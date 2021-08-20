/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Common RDMA Functions
    Copyright (C) 2021 Telescope Project
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CM_X11                  1

/*  Message type definitions (32-bit)
    These type definitions should be sent as immediate data to allow the
    receiver to understand the message type and decode it appropriately.
*/

#define T_MSG_INVALID           htonl(0x0000)   // Invalid message, can be used to signal that an received message was invalid
#define T_MSG_KEEP_ALIVE        htonl(0x0002)   // Keep the connection alive (not used)

#define T_NEG_SERVER_HELLO      htonl(0x0010)   // Server Hello Packet
#define T_NEG_CLIENT_HELLO      htonl(0x0011)   // Client Hello Packet
#define T_NEG_SERVER_BUF_DATA   htonl(0x0012)   // Server Buffer Data

/*  Pixel format types (32-bit)
    Both the server and client must understand each others' pixel formats,
    either natively or using conversion, in order for the connection to be
    established.
*/

#define T_PF_NULL               0x0000      // Placeholder for "no capture method available"

#define T_PF_ARGB_LE_32         0x0010      // BGRA packed pixels (little endian ARGB), with 32 bits per pixel.
#define T_PF_RGB_LE_24          0x0020      // BGR packed pixels (little endian RGB), with 24 bits per pixel.

#define CHR_YUV_444             0x0100      // YCbCr chroma with no subsampling
#define CHR_YUV_422             0x0110      // YCbCr 4:2:2 subsampled image
#define CHR_YUV_420             0x0120      // YCbCr 4:2:0 subsampled image

/* Helper macros for common operations */

#define clear(x) memset(&x, 0, sizeof(x))                       // Clear fields of an existing variable
#define new(type, x) type x; memset(&x, 0, sizeof(x))           // Create a new zeroed item (on stack)
#define heapnew(type, x) type *x = calloc(1, sizeof(type))      // Create a new zeroed item (on heap)
#define println(x) printf(x "\n");                              // Print with new line...

/* Print a string as a series of hex bytes */

static inline void printb(char *buff, int n)
{
    for(int i=0; i < n; i++)
    {
        printf("%hhx", *(buff + i));
    }
    printf("\n");
}

/* Message header */

typedef struct {
    int msg_type;       // Message type (see Message Type Definitions)
    int length;         // Message length
    int msg_id;         // Message identifier for asynchronous operation
    int num_msgs;       // Number of messages following of this type
} T_MsgHeader;

/* Server-side buffer preparation request structure */

typedef struct {
    int dpy_index;      // Requested display index
    int chroma;         // Requested subsampling of the image contents
} T_PrepareReq;

/* Client write request structure */

typedef struct {
    int dpy_index;      // Requested display index
    int chroma;         // Requested subsampling of the image contents
    off_t offset;       // Memory region on the client side
    int buf_index;      // Buffer index to write to
} T_WriteReq;

/* Memory buffer information */

typedef struct {
    long bufsize;       // Buffer size (bytes)
    off_t offset;       // Memory location offset
} T_BufferData;

/* Basic array structure */

typedef struct {
    int elements;       // Number of available elements
    void *data;         // Pointer to actual data, convert to appropriate type
} T_List;

/* Inline buffer data */
typedef struct {
    uint32_t    data_type;
    void        *addr;
    uint64_t    size;
    uint32_t    rkey;
    uint32_t    numbufs;
} T_InlineBuff;

/* Screen Capture Metadata */

typedef struct {
    uint32_t    capture_type;   // Screen capture provider (X11 etc.)
    uint32_t    width;          // Native width
    uint32_t    height;         // Native height
    uint32_t    fps;            // Native framerate
    uint16_t    bpp;            // Screen bits per pixel
    uint16_t    format;         // Screen pixel format
    union
    {
        struct {
            Display     *display;
            Window      window;
            int         screen_no;
        };
    };
} T_Screen;

/* Initial Data Exchange Packets */

/*  The server first sends this hello packet to inform the client about its
    native pixel map and egress bandwidth available to service the user's 
    request.
*/
typedef struct {
    uint64_t    avail_bw;       // Bandwidth limit (bits/second)
    uint64_t    avail_mem;      // Memory limit (bytes)
    uint32_t    s_width;        // Native width
    uint32_t    s_height;       // Native height
    uint32_t    s_fps;          // Native framerate
    uint16_t    s_format;       // Screen pixel format
} __attribute__((__packed__)) T_ServerHello;

/*  The client then replies back with the format it wants from the server.
    If the client wants the server to send the native pixmap for the highest
    performance, the client can simply echo back the parameters it received 
    from the server.  
    The client is responsible for ensuring there is 
    enough available downstream bandwidth to handle the RDMA reads, as the
    server is not responsible for performing or throttling reads.
*/
typedef struct {
    uint32_t    s_width;        // Requested width
    uint32_t    s_height;       // Requested height
    uint32_t    s_fps;          // Requested framerate
    uint16_t    s_bpp;          // Screen bits per pixel
    uint16_t    s_format;       // Screen pixel format
    uint16_t    rdma_bufs;      // Number of buffers to use for page flipping.
} __attribute__((__packed__)) T_ClientHello;

/*  The server then performs memory allocation according to the client's
    request. If the request is invalid, it's indicated as such.
*/
typedef struct {
    uint32_t    ok_flag;        // If everything is ok, indicate 0xffffffff
    uint64_t    fb_addr;        // Frame buffer address
    uint64_t    fb_size;        // Frame buffer size
    uint64_t    st_addr;        // State buffer address
    uint64_t    st_size;        // State buffer size
    uint32_t    fb_rkey;        // Frame buffer RDMA key
    uint32_t    st_rkey;        // State buffer RDMA key
    uint32_t    fb_bufs;        // Frame buffer number of sub-buffers
} __attribute__((__packed__)) T_ServerBuffData;

/*  At this point, the connection is established. The client is responsible
    for telling the server which buffers need to be read.
*/

// Get the page-aligned size for a single frame
inline int paligned_fsize(int width, int height, int bpp)
{
    int ps = getpagesize();
    int raw_fsize = (width * height * (bpp / 8));
    return raw_fsize + (ps - (raw_fsize % ps));
}

/* Linked list containing information about every buffer */
typedef struct {
    union {
        struct {
            XShmSegmentInfo     shminfo;        // Shared memory segment info
            uint32_t            bufid;          // Item number in buffer
            XImage              *ximage;        // X Image containing metadata
        };
    };
    T_ImageList *next;  // Next item in buffer
} T_ImageList;