/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Transport Library (TCP)
    Copyright (c) 2021 Telescope Project
*/

#include "transport_base.hpp"

/* ------------------ Common Transport Functions ------------------ */

/* ------------------ Server Transport Functions ------------------ */

class tsc_server_tcp : public tsc_server_base {

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

/* ------------------ Client Transport Functions ------------------ */

class tsc_client_tcp : public tsc_client_base {

public:

    int is_ready();

    void connect(const char *host, const char *port);

    int sockfd;

};