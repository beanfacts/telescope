#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <semaphore.h>
#include <unistd.h>

typedef struct {
    uint64_t addr;  // Remote memory buffer address
    uint64_t size;  // Remote memory buffer size
    uint32_t nbufs; // Number of buffers available
    uint32_t rkey;  // Remote key to access memory segment
} RemoteMR;


typedef struct
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    Display             *dsp;           // X11 display
    Window              win;            // X11 window
    XImage              *ximage;        // X11 image
    void                *rcv_buf;       // Receive buffer
    int                 rcv_bufsize;    // Receive buffer size
    int                 poll_usec;      // Polling delay
    sem_t               *semaphore;     // Semaphore to sync RDMA
} ControlArgs;


typedef struct 
{
    struct rdma_cm_id   *connid;        // RDMA connection identifier
    RemoteMR            *remote_mr;     // Remote MR information
    Display             *dsp;           // X11 display
    struct shmimage     *src;           // X11 shared memory segment information
    int                 screen_number;  // X11 screen index
    int                 width;          // Screen width
    sem_t               *semaphore;     // Semaphore to sync RDMA
} display_args;
