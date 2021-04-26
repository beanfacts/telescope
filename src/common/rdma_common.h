#include <stdio.h>
#include <stdlib.h>
#include <rdma/rsocket.h>

/* Message type definitions */

#define T_MSG_INVALID           0x00        // Invalid message, can be used to signal that an received message was invalid
#define T_MSG_KEEP_ALIVE        0x02        // Keep the connection alive (not used)

#define T_REQ_SCREEN_DATA       0x10        // Request the screen data, such as resolution and # of screens, from the server.
#define T_REQ_SERVER_PREPARE    0x11        // Request the server prepare resources needed to send an image to the client.
#define T_REQ_BUFFER_WRITE      0x12        // Request the server write the screen data into the client's buffer.
#define T_REQ_CLIENT_BUF_DATA   0x13        // Request the buffer data of the client, such as the RDMA rkey.

#define T_META_SCREEN_DATA      0x20        // Server response to a client request for server screen data
#define T_META_BUFFER_DATA      0x21        // Client response to a server request for client buffer data

#define T_NOTIFY_XFER_DONE      0x30        // Server notification to client that transfer of data is done
#define T_NOTIFY_XFER_FAIL      0x31        // Server notification to client that transfer of data has failed

#define T_INLINE_DATA_HID       0x40        // Inline data containing mouse and keyboard information
#define T_INLINE_DATA_CLI_MR    0x41        // Inline data containing the client memory information

/* Chroma subsampling types */

#define CHR_ASIS                0x00        // Used for client request, send the image as it is on the server

#define CHR_BGRA_32             0x01        // BGRA packed pixels, either Z or XY, with 32 bits per pixel.
                                            // Note that if the image is in XY format, the alpha channel resides
                                            // in memory but is never actually sent from the server.
#define CHR_BGR_24              0x03        // BGRA packed pixels, either Z or XY, with 24 bits per pixel.

#define CHR_YUV_444             0x10        // 
#define CHR_YUV_422             0x11
#define CHR_YUV_420             0x12

/* Transfer type definitions and timeouts */

#define XFER_READ   0
#define XFER_WRITE  1
#define msec(x) (x * 1000)

/* Helper macros for common RDMACM operations */

#define clear(x) memset(&x, 0, sizeof(x))                   // Clear fields of an existing variable
#define new(type, x) type x; memset(&x, 0, sizeof(x))       // Create a new zeroed item (on stack)
#define heapnew(type, x) type x = calloc(1, sizeof(type))   // Create a new zeroed item (on heap)
#define println(x) printf(x "\n");

void printb(char *buff, int n)
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

/* Screen metadata */

typedef struct {
    unsigned int screen_index;          // Screen number
    unsigned int screen_offset_x;       // X Screen offset (x)
    unsigned int screen_offset_y;       // X Screen offset (y)
    unsigned int width;                 // Screen width in pixels
    unsigned int height;                // Screen height in pixels
    unsigned int bits_per_pixel;        // Screen bit depth
    unsigned int framerate;             // Vertical sync framerate
} T_ScreenData;

/* Screen header for transferred image */

typedef struct {
    unsigned int screen_index;          // Screen number
    unsigned int width;                 // Screen width in pixels
    unsigned int height;                // Screen height in pixels
    unsigned int chroma;                // Chroma subsampling type if any
    unsigned int ximage_type;           // X Image type (XYPixmap, ZPixmap)
} T_ScreenMeta;

/*  Returns the number of displays available on the server.
    The data must be freed when done!
*/
T_List *get_displays(int connfd);

/*  Perform a complete read (mode 0), or write (mode 1), of len bytes into buffer *data.

    Returns when all the data has been written, or more than max_attempts
    attempts have been made to transfer the data to the other side.

    A failed attempt is counted as a read/write which returned fewer than
    or equal to min_xfer_threshold bytes, and counted sequentially.

    Returns 1 if successful, 0 if not.
*/
int full_xfer(int mode, int connfd, void *data, int len, int min_xfer_threshold, float timeout, int usleep_duration);

/*  Request the server draw its display contents for the client.
    
    The server must know which display to send, which memory region to write
    the data to, the index of the display as returned by get_displays(), and
    if the data is double buffered, the number of the buffer to write to. In
    the case that the client does not have a double buffer, the index of the
    buffer is always 0.
*/
int req_image(int connfd, int dpy_index, off_t offset, int buf_index);

/*  Request the server prepare its image capture backend for transmission given
    the requested image parameters.

    Future support for the transmission of multiple displays is planned.
*/
int prepare_image(int connfd, int dpy_index, int chroma);