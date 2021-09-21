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
#include "transport/get_ip.h"


/* Protobuf Libraries */
#include "transport/transport.pb.h"

#define MAX_TRANSPORTS 3

using namespace std;

static void tsc_usage_server(int argc, char **argv)
{
    fprintf(stderr, 
        "Invalid arguments provided.\n"
        "Usage: %s -c <capture type> -h <host> -p <port> --ifname <name> --rdma <portnum>\n",
        argv[0]
    );
}

static inline uint generate_seed()
{
    uint seed;
    FILE *f = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(uint), 1, f);
    fclose(f);
    return seed;
}

int get_transport_idx(struct tsc_transport *transports, int flag)
{
    for (int i = 0;;i++)
    {
        if (transports[i].flag == 0)
        {
            return -1;
        }
        if (transports[i].flag == flag)
        {
            return i;
        }
    }
}

int main(int argc, char **argv)
{

    //int ret;
    
    printf("Telescope test build (%s - GCC %d.%d.%d)\n", __DATE__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    srandom(generate_seed());

    // Can be passed to getaddrinfo
    char *host_str  = (char *) calloc(1, 256);
    char *port_str  = (char *) calloc(1, 16);
    if (!host_str || !port_str)
    {
        cerr << "Memory allocation failed.\n";
        return 0;
    }
    
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
        {"ifname",      required_argument,  0,  'i'},
        {0, 0, 0, 0}
    };

    // Getopt stuff
    int option_index    = 0;
    int ok_flag         = 0;
    int transport_index = 0;
    int transport_flags = 0;
    char *if_addr = nullptr;

    // Initial data exchange transport
    struct tsc_transport init_transport = {
        .flag = TSC_TRANSPORT_TCP,
        .host = host_str,
        .port = port_str,
        .attr = nullptr
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
                    .port = strdup(optarg),
                    .attr = nullptr
                };
                transport_flags |= TSC_TRANSPORT_RDMACM;
                ok_flag = ok_flag | 1;
                transport_index++;
                break;
            case 't':
                if (transport_index >= MAX_TRANSPORTS) { return 1; }
                transports[transport_index] = {
                    .flag = TSC_TRANSPORT_TCP,
                    .host = host_str,
                    .port = strdup(optarg),
                    .attr = nullptr
                };
                transport_flags |= TSC_TRANSPORT_TCP;
                ok_flag = ok_flag | 1;
                transport_index++;
                break;
            case 'u':
                if (transport_index >= MAX_TRANSPORTS) { return 1; }
                transports[transport_index] = {
                    .flag = TSC_TRANSPORT_UCX,
                    .host = host_str,
                    .port = strdup(optarg),
                    .attr = nullptr
                };
                transport_flags |= TSC_TRANSPORT_UCX;
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
            case 'i':
                ok_flag = ok_flag | 1 << 3;
                if_addr = find_addr(optarg);
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
                ok_flag = ok_flag | 1 << 4;
                break;
            default:
                printf("Invalid argument passed\n");
                return 1;
        }
    }

    if (ok_flag != 0b11111)
    {
        tsc_usage_server(argc, argv);
        return 1;
    }

    // Initialise screen capture system
    tsc_capture_context capture_ctx;
    tsc_screen scr;
    scr.capture_type = cap_method;
    capture_ctx.init(&scr);

    // Initialise server
    tsc_server server;
    server.init_servers(&init_transport, transports, 1);

    // Block until incoming client detected
    tsc_client_repr client = server.get_client();
    
    // Receive and parse client hello message
    tsc_mbuf client_msg = client.recv_msg();
    
    int client_flags = 0;
    int sel_transport = 0;
    
    {
        pb_wrapper wrapper;
        wrapper.ParseFromArray(client_msg.buf, client_msg.size);
        if (!wrapper.has_pb_whello()) cerr << "Client sent malformed data!\n";
        free(client_msg.buf);
        pb_hello *client_hello = wrapper.mutable_pb_whello();

        // Determine the requested client resolution
        // We only have X11 capture, which is usually BGRA...
        // This should be changed later to support different pixel formats
        if (client_hello->screens(0).pixmap() != pb_pixmap::z_bgra_888)
        {
            cerr << "Invalid client pixmap type.\n";
        }

        // Determine whether the client supports the server's transport types.
        for (int i = 0; i < client_hello->transports_size(); i++)
        {
            int flag = client_hello->transports(i).flag();
            cout << "Client transport type flag: " << flag << "\n";
            int can_support = client_flags & transport_flags;
            if (can_support && flag > client_flags)
            {
                sel_transport = i;
            }
            client_flags |= flag;
        }
    }

    std::cout << "Transport index: " << sel_transport << "\n";

    {
        // Flag order should be: UCX RDMACM TCP
        // For now, only RDMACM (ibverbs) is supported as a transport type.
        pb_wrapper wrapper;
        pb_hello_reply *reply = wrapper.mutable_pb_whello_reply();
        
        // Set transport type supported by server
        int rdma_idx = get_transport_idx(transports, TSC_TRANSPORT_RDMACM);
        reply->set_cookie(random());
        reply->mutable_transport()->set_flag(TSC_TRANSPORT_RDMACM);
        reply->mutable_transport()->set_port(transports[rdma_idx].port);
        reply->mutable_transport()->set_ip_addr(if_addr);
        reply->set_cookie(random());

        
        // Add screen data
        pb_screen *tscr = reply->add_screens();
        tscr->set_index(0);
        tscr->set_pixmap(pb_pixmap::z_bgra_888);
        tscr->set_res_x(scr.width);
        tscr->set_res_y(scr.height);
        tscr->set_hz(scr.fps);
        
        // Send this data back to the client...
        char *sndbuf = server.pack_msg(&wrapper);
        client.send_msg(sndbuf);
        free(sndbuf);
    }

    // Now we can wait for an incoming RDMA connection.
    // For now the cookie is not verified since we only support
    // one client, but it should be in future.
    cout << "Waiting for RDMA connection...\n";
    server.add_client_channel(&client, TSC_TRANSPORT_RDMACM);

    // Note that once the client has established a data channel
    // with the server, the client is expected to send ALL messages
    // over this new channel.
    cout << "hello client.\n";
    sleep(10);

}