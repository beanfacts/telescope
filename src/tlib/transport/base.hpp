/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (Base Classes)
    Copyright (c) 2021 Telescope Project
*/

#pragma once

#include <cerrno>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "smalloc/smalloc.h"

/*  ---------------------- Transport Type Handles ---------------------- */

/*  Opaque handles for different transport types.
    Allows internal functions to be overloaded. */

/*  Wrapper for [struct ibv_mr *] */
typedef struct { void* addr; } tsc_rdma_mr_h;

/*  Wrapper for [rdma_cm_id *] */
typedef struct { void* addr; } tsc_rdma_id_h;

/*  Wrapper for TCP FD. */
typedef struct { int fd; } tsc_tcp_id_h;

/*  Wrapper for all connection types. */
typedef struct {
    int type;
    union {
        tsc_tcp_id_h    tcp_id;
        tsc_rdma_id_h   rdma_id;
    };
} tsc_conn_h;

/*  Wrapper for all MR types. */
typedef struct {
    /*  Native context type. See tsc_transport_type enum. */
    int type;
    /*  Pointer to the native context, e.g. ibv_mr * to be cast by functions
        that use this MR. */
    void *native;
    /*  Device address pointer. */
    void *addr;
    /*  Protection flags. See tsc_access_flags. */
    int access_flags;
} tsc_mr_h;

/*  Telescope shared memory segment handle. */
typedef struct {
    /*  SysV shared memory identifier. */
    int shm_id;
    /*  Memory address. */
    void *shm_addr;
    /*  Buffer length. */
    size_t size;
    /*  User-defined context. */
    void *context;
} tsc_shm_h;

/*  Protection flags across all transport types without requiring ibverbs. */
enum tsc_access_flags {
	TSC_ACCESS_LOCAL_WRITE      = 1,
	TSC_ACCESS_REMOTE_WRITE     = (1 << 1),
	TSC_ACCESS_REMOTE_READ      = (1 << 2),
	TSC_ACCESS_REMOTE_ATOMIC    = (1 << 3)
};

/*  Telescope transport type list. */
typedef enum {
    TSC_TRANSPORT_NONE      = 1,        // Dummy server
    TSC_TRANSPORT_TCP       = (1 << 1), // TCP
    TSC_TRANSPORT_RDMACM    = (1 << 2), // RDMA_CM + InfiniBand Verbs
    TSC_TRANSPORT_UCX       = (1 << 3)  // Unified Communication X (UCX)
} tsc_transport_type;

/*  ---------------------- Control Structs ---------------------- */

/*  Mouse button flags. */
enum tsc_hid_mouse_buttons {
    TSC_HID_MOUSE_LBUTTON       = 1,
    TSC_HID_MOUSE_RBUTTON       = (1 << 1),
    TSC_HID_MOUSE_MBUTTON       = (1 << 2),
    TSC_HID_MOUSE_XBUTTON1      = (1 << 3),     // Back
    TSC_HID_MOUSE_XBUTTON2      = (1 << 4),     // Fwd
    TSC_HID_MOUSE_WHEELDOWN     = (1 << 5),
    TSC_HID_MOUSE_WHEELUP       = (1 << 6),
    TSC_HID_MOUSE_WHEELLEFT     = (1 << 7),
    TSC_HID_MOUSE_WHEELRIGHT    = (1 << 8)
};

/*  Information needed to initialize transports. */
struct tsc_transport {
    tsc_transport_type  flag;
    char                *host;
    char                *port;
    void                *attr;
};

/*  ---------------------- Internal Classes ---------------------- */

namespace tsc_base {

    /*  Allocate memory aligned to system page size.
        length:     Minimum length.
        out_length: Actual allocated length.
        Returns:    Pointer to memory region. */
    void *alloc_aligned(size_t length, size_t *out_length);

    /*  Construct a sockaddr struct from address and port information. 
        address_str:    IP address or hostname string.
        port:           Port number string.
        out_addr:       Sockaddr struct to populate. */
    void set_sockaddr(const char *address_str, const char *port,
            struct sockaddr_in *out_addr);

};

/*  Telescope static memory pool manager. A wrapper around smalloc. */
class tsc_malloc {

public:

    /*  Create a new memory pool.
        length:     Buffer length.
        alignment:  Memory alignment. */
    tsc_malloc(size_t length, size_t alignment);

    /*  Destroy the memory pool. */
    ~tsc_malloc();

    /*  Set the thread-safety mode for the memory allocator. 
        mode:   true ->  thread-safe, lower performance
                false -> thread-unsafe, higher performance */
    void set_thread_mode(bool mode);

    /*  Get the thread-safety mode for the memory allocator. */
    bool get_thread_mode();

    /*  Allocate memory from this class's memory pool. 
        size:       Number of bytes to allocate.
        Returns:    Pointer to memory region. 0 on failure. */
    void *tmalloc(size_t size);

    /*  Free memory allocated from this class only. 
        addr:       Pointer to memory region. 
        Returns:    nothing, like free(). */
    void tfree(void *addr);

    /*  Get the pointer to the allocated memory region. */
    void *get_addr();

private:

    void *addr = nullptr;
    size_t oom_h(smalloc_pool *pptr, size_t n);
    bool thread_mode = false;
    pthread_mutex_t thread_lock;
    struct smalloc_pool *pool;
};

/*  ---------------------- Telescope Communication API ---------------------- */

/*  Communicator class supporting only blocking calls. 
    This is intended for the control channel, and as such should be cross-
    platform. Platform-specific functions, such as epoll or kqueue, should
    not be used here. */
class tsc_communicator {

public:

    /*  Get the active transport type of this class. 
        This value must be set by derived classes. */
    virtual int get_transport_type() = 0;

    /*  Send a message. This function blocks until the message is sent.
        buf:    Buffer to send messages from.
        length: Number of bytes to send. */
    virtual void send_msg(char *buf, size_t length) = 0;

    /*  Receive a message. This function blocks until the message is sent.
        buf:        Buffer to receive messages into.
        max_len:    Maximum message size.
        Returns:    Actual number of bytes received. */
    virtual ssize_t recv_msg(char *buf, size_t max_len) = 0;

};


/*  Communicator class supporting non-blocking calls.
    Libraries such as ibverbs support non-blocking calls. The wrapper for such
    transport types should be implemented as non-blocking for maximum
    performance. */
class tsc_communicator_nb {

public:

    /*  Communicator init functions. */
    virtual void init(tsc_rdma_mr_h id) {
        throw std::runtime_error("Not implemented or incorrect class.");
    }
    
    /*  Get the active transport type of this class. 
        This value must be set by derived classes. */
    virtual int get_transport_type() = 0;

    /*  -------------------------------------------------------------------  */

    /*  Register the memory region for RDMA operations.
        buf:    Buffer address.
        length: Buffer length.
        flags:  Memory protection flags.
        If the transport type in use does not support RDMA operations, the MR
        should be returned, and RDMA emulation performed internally. */
    virtual tsc_mr_h register_mem(void *buf, size_t length, int flags) = 0;

    /*  Deregister the memory region. */
    virtual void deregister_mem(tsc_mr_h mr) = 0;

    /*  -------------------------------------------------------------------  */
    
    /*  Send a message from buf.
        The transport type must support inline messages, and length must be
        less than the maximum inline message size. If the message size exceeds
        the inline message size and pre-registered memory is available, that
        memory will be used instead.
        The buffer can be reused immediately after calling this function.
        Returns: cookie on successful operation. 0 on failure. */
    virtual uint64_t send_msg(void *buf, size_t length) = 0;

    /*  Send a message from a registered buffer.
        mr:     Registered memory region details.
        buf:    If the data is not stored at the start of the MR, the start
                address within the local MR can be specified. 
        len:    Number of bytes to send. 
        The buffer must remain registered until the send completes. */
    virtual uint64_t send_msg(tsc_mr_h mr, void *buf, size_t len) = 0;

    /*  Receive a message into a registered buffer.
        mr:     Registered memory region details.
        buf:    If the data should not be stored at the start of the MR, the
                start address within the local MR can be specified. 
        len:    Maximum number of bytes to receive.  
        The buffer must remain registered until the receive completes. */
    virtual uint64_t recv_msg(tsc_mr_h mr, void *buf, size_t len) = 0;

    /*  -------------------------------------------------------------------  */

    /*  Perform a write directly from local to remote memory.
        local_mr:       Local MR handle.
        remote_mr:      Remote MR handle.
        local_addr:     If the data is not stored at the start of the MR, the
                        start address within the local MR can be specified.
        remote_addr:    Same as local_addr, but for the remote side. 
        length:         Number of bytes to write.
        Returns:        cookie on success, 0 on failure. */
    virtual uint64_t write_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
            void *local_addr, void *remote_addr, size_t length) = 0;

    /*  Perform a read directly from remote to local memory.
        local_mr:       Local MR handle.
        remote_mr:      Remote MR handle.
        local_addr:     If the data is not stored at the start of the MR, the
                        start address within the local MR can be specified.
        remote_addr:    Same as local_addr, but for the remote side.
        length:         Number of bytes to read.
        Returns:        cookie on success, 0 on failure. */
    virtual uint64_t read_msg(tsc_mr_h local_mr, tsc_mr_h remote_mr,
            void *local_addr, void *remote_addr, size_t length) = 0;

    /*  -------------------------------------------------------------------  */

    /*  Poll the active channel for outgoing message completions.
        This includes send, write, and read operations!
        block: Whether to block until an event.
        Returns nonzero if there are any new messages in the queue. 
        Does not insert the message into the completion queue.
        Sets the message identifier rid if retval > 0. */
    virtual int poll_sq(uint64_t *out, bool block) = 0;

    /*  Poll the active channel for incoming messages.
        block: Whether to block until an event.
        Returns nonzero if there are any new messages in the queue. 
        Does not insert the message into the completion queue.
        Sets the message identifier rid if retval > 0. */
    virtual int poll_rq(uint64_t *out, bool block) = 0;

    /*  -------------------------------------------------------------------  */

    /*  Increment the send counter, skipping zero. */
    inline uint64_t increment_sc();

    /*  Increment the receive counter, skipping zero. */
    inline uint64_t increment_rc();

    /*  Pending message counters */
    uint64_t send_pending = 0;
    uint64_t recv_pending = 0;
    
    /*  Unique message ID counter */
    uint64_t send_counter = 0;
    uint64_t recv_counter = 0;

    /*  Currently active transport used by this client. */
    tsc_transport_type active_transport = TSC_TRANSPORT_NONE;

};


/*  Server class supporting only blocking calls. */
class tsc_server_base {

public:
  
    /*  Start a server and listen on host:port.
        host:       IP address or hostname to listen on.
        port:       Port number.
        type:       Protocol family e.g. SOCK_STREAM
        backlog:    Maximum pending connections. */
    virtual void init_server(const char *host, const char *port, int type,
            int backlog) = 0;

    /*  Get the active transport type of this class. */
    virtual int get_transport_type() = 0;

    /*  Accept a new connection. */
    virtual tsc_conn_h accept();

};