
#include <stdio.h>
#include <getopt.h>
#include <rdma/rdma_cma.h>

int main(int argc, char **argv)
{
    struct addrinfo *addr;
    struct rdma_cm_event *event = NULL;
    struct rdma_cm_id *conn= NULL;
    struct rdma_event_channel *ec = NULL;
    
    // Process command line arguments.
    static struct option long_options[] =
    {
        {"host",        required_argument,  0,  'h'},
        {"nc_port",     required_argument,  0,  'n'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int ok_flag = 0;

    while (1)
    {
        int c = getopt_long_only(argc, argv, "h:n:", long_options, &option_index);
        printf("[%s]\n", optarg);

        if (c == -1)
            break;

        switch (c)
        {
                
            case 'h':
                if (!inet_aton(optarg, &host))
                {
                    fprintf(stderr, "Could not convert IP address...\n");
                    break;
                }
                ok_flag = ok_flag | 1;
                break;
            case 'n':
                port = (uint16_t) atoi(optarg);
                ok_flag = ok_flag | 2;
                break;
        }
    }


}