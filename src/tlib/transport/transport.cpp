#include "transport/transport.hpp"

ssize_t tsc_pb::pack_msg(pb_wrapper *msg, char *buf, size_t maxbufsize)
{
    /*  We do not have messages larger than 2GB. But we check it
        anyway in case the message data is malformed */
    long lbufsize = msg->ByteSizeLong();
    if (lbufsize >= maxbufsize)
        throw std::runtime_error("Message buffer too small to contain message.");
    else if (lbufsize >= INT32_MAX)
        throw std::runtime_error("Message too large for transmission.");
    
    /*  Both sides expect to decode 4 bytes at the start of the message to get
        the message size. */
    static_assert(sizeof(uint32_t) == 4);
    
    uint32_t bufsize = (uint32_t) lbufsize;
    
    /*  Pack the message size, in network byte order, into the buffer. */
    bool status = msg->SerializeToArray(buf + 4, bufsize);
    if (!status) throw std::runtime_error("Serialize failed.");
    * (uint32_t *) buf = htonl(bufsize);

    return lbufsize;
}

pb_wrapper unpack_msg(char *buf, size_t max_len)
{
    uint32_t mlen;
    mlen = * (uint32_t *) buf;
    if (((size_t) mlen) > max_len)
        throw std::runtime_error("Buffer size too small to contain message.");

    pb_wrapper w;
    if (!w.ParseFromArray((void *) (buf + 4), max_len))
        throw std::runtime_error("Decode failed.");

    return w;
}

/*  ---------------------- Telescope Communication API ---------------------- */