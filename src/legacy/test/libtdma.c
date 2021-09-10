/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Protocol Wrapper Libraries
    Copyright (c) 2021 Tim Dettmar
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtdma.h"

int td_create_server(in_addr_t host, uint16_t port, int socktype, SAI *config)
{
    
    int sockfd;
    
    sockfd = socket(AF_INET, socktype, 0);
    if (sockfd == -1)
    {
        printf("Could not create the socket.\n");
        return -1;
    }
    
    SAI *servaddr = malloc(sizeof(SAI));
    memset(servaddr, 0, sizeof(SAI)); 
    
    servaddr->sin_family        = AF_INET; 
    servaddr->sin_addr.s_addr   = htonl(host);
    servaddr->sin_port          = htons(port);

    if (bind(sockfd, (struct sockaddr *) servaddr, sizeof(servaddr)) != 0)
    {
        printf("Cannot bind to socket.\n");
        free(servaddr);
        return -1;
    }
    
    if (listen(sockfd, 1) == -1)
    { 
        printf("Cannot listen on socket.\n");
        free(servaddr);
        return -1;
    }

    config = servaddr;
    return sockfd;
}

int td_create_client(in_addr_t host, uint16_t port, int socktype, SAI *config)
{
    int sockfd = -1;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Failed to create the TCP socket!\n");
        return -1;
    }
    printf("Created TCP event notification socket.\n");

    int flag = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    if (ret < 0)
    {
        fprintf(stderr, "Cannot set socket options.\n");
        return -1;
    }

    SAI *servaddr = malloc(sizeof(SAI));
    memset(servaddr, 0, sizeof(SAI)); 

    servaddr->sin_family = AF_INET; 
    servaddr->sin_addr.s_addr = host;
    servaddr->sin_port = htons(port);

    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(SAI));
    if (ret != 0)
    {
        fprintf(stderr, "Could not connect to the server.\n");
        free(servaddr);
        return -1;
    }

    config = servaddr;
    return sockfd;
}