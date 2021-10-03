/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (TCP)
    Copyright (c) 2021 Telescope Project
*/

#include "transport/base.hpp"
/* ------------------ Common Transport Functions ------------------ */

/*  Communicator implementation for TCP. */
class tsc_communicator_tcp : public tsc_communicator {

public:

    tsc_communicator_tcp(int _fd);

    /*  Communicator API functions */
    inline int get_transport_type();
    void send_msg(char *buf, size_t length);
    ssize_t recv_msg(char *buf, size_t max_len);

private:

    int connfd;

};

/* ------------------ Server Transport Functions ------------------ */


/*
class tsc_server_tcp : public tsc_server {

public:

    int is_ready();

    void init_server(const char *host, const char *port, int backlog);
    
    void init_server(struct sockaddr_in *sa, int backlog);
    
    int get_server(struct sockaddr_in *sa, int backlog);

    void attach_fd(int _fd);

    int accept();

private:
    
    int sockfd = -1;

};
*/

/* ------------------ Client Transport Functions ------------------ */
