// Basic stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Networking + RDMA stuff
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rdma/rsocket.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>

// XCB stuff
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

// X11 stuff
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xatom.h>

#include "../common/rdma_common.h"


