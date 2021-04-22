        /*
        // Create an RDMA connection.
        int rdma_fd;
        new(struct sockaddr_in, rdma_addr);
        new(struct sockaddr_in, cli);

        // Configure the socket to not buffer outgoing data, as well as allow remote
        // I/O operations, with a maximum of 1 I/O mapped buffer.
        rdma_fd = rsocket(AF_INET, SOCK_STREAM, 0);
        int optval = 1;
        rsetsockopt(rdma_fd, SOL_TCP, TCP_NODELAY, (void *) &optval, sizeof(optval));
        rsetsockopt(rdma_fd, SOL_RDMA, RDMA_IOMAPSIZE, (void *) &optval, sizeof(optval));
        if (rdma_fd == -1) { 
            printf("Could not create RDMA socket.\n"); 
            exit(1); 
        } 
        printf("Created RDMA Socket - FD: %d\n", rdma_fd);

        // Attempt a connection to the server.
        printf("Trying to connect to %s:%d (RDMA)\n", argv[1], atoi(argv[2]));
        rdma_addr.sin_family = AF_INET; 
        rdma_addr.sin_addr.s_addr = inet_addr(argv[1]); 
        rdma_addr.sin_port = htons(atoi(argv[2])); 
        if (rconnect(rdma_fd, (SA*)&rdma_addr, sizeof(rdma_addr)) != 0)
        { 
            printf("RDMA connection with the server failed.\n"); 
            exit(1); 
        }
        */