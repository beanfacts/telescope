#include <unistd.h>

#include "transport/transport.hpp"
#include "transport/transport.pb.h"
#include "transport/get_ip.h"
#include "ui/ui.hpp"

int main(int argc, char **argv)
{
    int ret;
    tsc_client_ui ui;
    tsc_client client;
    
    ui.init(1280, 720);
    ret = ui.wait_request();
    if (ret != 0) {
        std::cout << "Exiting due to close.\n";
        return 0;
    }
    _ui_data config = ui.get_config();
    
    client.init_control_channel(config.host_str, config.port_str);
    
    // Which connection types I support
    int transport_flags = 0;

    if (config.enable_rdma)
        transport_flags |= TSC_TRANSPORT_RDMACM;

    char *my_ip = find_addr(config.ifname_str);

    // Populate, wrap, and serialize metadata.
    {
        std::cout << "Wrapping message...\n";
        pb_wrapper wrapper;
        pb_hello *hello = wrapper.mutable_pb_whello();

        // Configure transport type
        hello->add_transports();
        hello->mutable_transports(0)->set_flag(transport_flags);
        hello->mutable_transports(0)->set_ip_addr(config.ifname_str);

        // Configure screen information
        hello->add_screens();
        hello->mutable_screens(0)->set_index(0);
        hello->mutable_screens(0)->set_res_x(config.res_x);
        hello->mutable_screens(0)->set_res_y(config.res_y);
        hello->mutable_screens(0)->set_pixmap(pb_pixmap::z_bgra_888);
        hello->mutable_screens(0)->set_hz(config.fps);

        // We can add acceptable resolutions but for this version
        // it is not populated.

        // Allocate memory to send the message and .. send it
        std::cout << "packing message...\n";
        char *mpkt = client.pack_msg(&wrapper);
        client.send_msg(mpkt);
        free(mpkt);
    }

    // Wait for the server to reply (over control channel)
    std::cout << "Reading message...\n";
    pb_wrapper wrapper;
    tsc_mbuf buf = client.recv_msg();
    bool stat = wrapper.ParseFromArray(buf.buf, buf.size);
    std::cout << "stat: " << stat << "\n";
    if (!wrapper.has_pb_whello_reply() || !stat)
        throw std::runtime_error("Server sent malformed data!");
    free(buf.buf);
    pb_hello_reply *reply = wrapper.mutable_pb_whello_reply();

    // Establish data channel connection
    tsc_transport data_channel;
    data_channel.flag = (tsc_transport_type) reply->mutable_transport()->flag();
    data_channel.host = (char *) reply->mutable_transport()->ip_addr().c_str();
    data_channel.port = (char *) reply->mutable_transport()->port().c_str();

    std::cout << "Data channel: " << data_channel.host << ":" << data_channel.port << "\n";

    // Establish connection.
    client.init_transport(&data_channel);
    std::cout << "Connected\n";


}