/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope - Linux Server Host
    Copyright (c) 2021 Telescope Project
*/

#include <iostream>
#include <getopt.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>

/* Internal Libraries */
#include "transport/transport.hpp"
#include "capture/capture.hpp"
#include "common.hpp"

#define MAX_TRANSPORTS 3

using namespace std;

static void tsc_usage_server(int argc, char **argv)
{
    fprintf(stderr, 
        "Invalid arguments provided.\n"
        "Usage: %s -c <capture type> -h <host> -p <port> [--rdma <portnum>] [--tcp <portnum>] [--ucx <portnum>]\n",
        argv[0]
    );
}


int main(int argc, char **argv)
{

    int ret;
    
    printf("Telescope build " TELESCOPE_VERSION " (GCC %d.%d.%d)\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    
    // --- Get arguments

    // Can be passed to getaddrinfo
    char *host_str  = (char *) calloc(1, 256);
    char *port_str  = (char *) calloc(1, 16);
    
    // Store desktop capture method
    tsc_capture_type cap_method = TSC_CAPTURE_NONE;
    
    // Valid command line arguments
    static struct option long_options[] =
    {
        {"rdma",        required_argument,  0,  'r'},
        {"tcp",         required_argument,  0,  't'},
        {"ucx",         required_argument,  0,  'u'},
        {"host",        required_argument,  0,  'h'},
        {"port",        required_argument,  0,  'p'},
        {"capture",     required_argument,  0,  'c'},
        {0, 0, 0, 0}
    };

    // Getopt stuff
    int option_index    = 0;
    int ok_flag         = 0;
    int transport_index = 0;

    // Initial data exchange transport
    struct tsc_transport init_transport = {
        .flag = TSC_TRANSPORT_TCP,
        .host = host_str,
        .port = port_str,
    };

    // Transport types
    struct tsc_transport *transports = 
        (struct tsc_transport *) calloc(1, sizeof(struct tsc_transport) * (MAX_TRANSPORTS + 1));

    while (1)
    {
        int c = getopt_long(argc, argv, "r:t:u:h:p:c:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                printf("???\n");
                break;
            case 'r':
                if (transport_index >= MAX_TRANSPORTS) { return 1; }
                transports[transport_index] = {
                    .flag = TSC_TRANSPORT_RDMACM,
                    .host = host_str,
                    .port = strdup(optarg)
                };
                ok_flag = ok_flag | 1;
                transport_index++;
                break;
            case 't':
                if (transport_index >= MAX_TRANSPORTS) { return 1; }
                transports[transport_index] = {
                    .flag = TSC_TRANSPORT_TCP,
                    .host = host_str,
                    .port = strdup(optarg)
                };
                ok_flag = ok_flag | 1;
                transport_index++;
                break;
            case 'u':
                if (transport_index >= MAX_TRANSPORTS) { return 1; }
                transports[transport_index] = {
                    .flag = TSC_TRANSPORT_UCX,
                    .host = host_str,
                    .port = strdup(optarg)
                };
                ok_flag = ok_flag | 1;
                transport_index++;
                break;
            case 'h':
                strncpy(host_str, optarg, 255);
                ok_flag = ok_flag | 1 << 1;
                break;
            case 'p':
                strncpy(port_str, optarg, 15);
                ok_flag = ok_flag | 1 << 2;
                break;
            case 'c':
                if (strncasecmp(optarg, "pw", 2) == 0)
                {
                    printf("Capture method: PipeWire DMA-BUF\n");
                    cap_method = TSC_CAPTURE_PW;
                }
                else if (strncasecmp(optarg, "x11", 3) == 0)
                {
                    printf("Capture method: X11 SHM\n");
                    cap_method = TSC_CAPTURE_X11;
                }
                else if (strncasecmp(optarg, "amf", 3) == 0)
                {
                    printf("Capture method: AMF RDMA\n");
                    cap_method = TSC_CAPTURE_AMF;
                }
                else if (strncasecmp(optarg, "lgshm", 5) == 0)
                {
                    printf("Capture method: Looking Glass SHM\n");
                    cap_method = TSC_CAPTURE_LGSHM;
                }
                else if (strncasecmp(optarg, "nvfbc", 5) == 0)
                {
                    printf("Capture method: NvFBC GPUDirect\n");
                    cap_method = TSC_CAPTURE_NVFBC;
                }
                else
                {
                    printf(
                        "Invalid capture method specified.\n"
                        "Valid options:\n"
                        " x11   : X11 Display Server Capture\n"
                        // " pw    : PipeWire DMA-BUF Capture\n"
                        // " amf   : AMD AMF Capture\n"
                        // " nvfbc : NVIDIA Framebuffer Capture\n" 
                        // " lgshm : Looking Glass SHM\n"
                    );
                    cap_method = TSC_CAPTURE_NONE;
                    return 1;
                }
                ok_flag = ok_flag | 1 << 3;
                break;
            default:
                printf("Invalid argument passed\n");
                return 1;
        }
    }

    if (ok_flag != 0b1111)
    {
        tsc_usage_server(argc, argv);
        return 1;
    }

    // --- Initialise capture system

    // Initialise the screen capture system.
    tsc_screen *screen = (tsc_screen *) calloc(1, sizeof(tsc_screen));
    screen->capture_type = cap_method;
    ret = tsc_init_capture(screen);
    if (ret)
    {
        fprintf(stderr, "Screen capture init failed.\n");
        return 1;
    }

    // --- Initialise server

    // Initialise the server. Telescope is only designed to handle
    // one client at a time.
    struct tsc_server *server;
    server = tsc_start_servers(&init_transport, transports, 1);
    if (!server)
    {
        fprintf(stderr, "Server init failed.\n");
        return 1;
    }

    // Wait for the client to request a connection
    ret = listen(server->init_server->sockfd, 0);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to listen on init channel.\n");
        return 1;
    }

    printf("Hello world.\n");

}