#include <sys/types.h>
#include "transport/base.hpp"
#include "protobuf/transport.pb.h"

/*  ---------------------- Telescope Protobuf Library ---------------------- */

namespace tsc_pb {

    /*  Pack a message from protobuf format into an existing buffer.
        This message includes the 4-byte delimiter.
        msg:        Protobuf message wrapper.
        buf:        Pointer to allocated buffer.
        max_len:    Buffer length.
        Returns:    Message length. */
    ssize_t pack_msg(pb_wrapper *msg, char *buf, size_t max_len);
    
    /*  Unpack a wrapped protobuf message. 
        buf:        Pointer to message buffer, including 4-byte delimiter.
        len:        Maximum number of bytes to unpack.
        Returns:    Protobuf message wrapper class. */
    pb_wrapper unpack_msg(char *buf, size_t max_len);

};

/*  ---------------------- Telescope Client/Server API ---------------------- */

/*  Connection class.
    This class can be used both by the client and server to exchange messages. */
class tsc_conn {
    
public:

    /*  Callback executed on new connection. */
    void connect_cb(tsc_communicator_nb *new_conn)
    {
        pthread_mutex_lock(&pending_lock);
        pending_conn.insert(pending_conn.end(), new_conn);
        pthread_mutex_unlock(&pending_lock);
    }

    tsc_communicator    *ctrl_ch        = nullptr;
    tsc_communicator_nb *data_ch        = nullptr;

    /*  Pending connection vector. */
    std::vector<tsc_communicator_nb *>  pending_conn;
    pthread_mutex_t                     pending_lock;

};

/*  Server class.
    The sole purpose of the server class is to wait and accept clients. Once a
    client has been accepted, resources associated with that client should be
    migrated to a tsc_client class. */
class tsc_server {

public:

    /*  Establish a control channel on host:port with protocol family 'type'.
        Currently supported:
        - Sockets + SOCK_STREAM => TCP */
    void start_cc(const char *host, const char *port, int type);
    
    /*  Establish a data channel on host:port with protocol family 'type'.
        Currently supported:
        - RDMA_CM + SOCK_STREAM => IBV_QPT_RC; RDMA_PS_TCP 
        Note that IBV_QPT_RC behaves like SOCK_SEQPACKET, not SOCK_STREAM. */
    void start_dc(const char *host, const char *port, int type);

    /*  Accept a client on the control channel.
        Returns a connection class that can be used for further communication. */
    tsc_conn accept();

    /*  Accept a client on the data channel, and add it to the connection
        class. 
        conn:   Connection identifier. The */
    void accept_dc(tsc_conn *conn, int type);
};