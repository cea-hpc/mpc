#include <mpi.h>
#include <mpc_lowcomm_monitor.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int set_callback(mpc_lowcomm_monitor_set_t set, void * arg)
{
    int * verbose = (int *)arg;

    mpc_lowcomm_set_uid_t uid = mpc_lowcomm_monitor_set_gid(set);
    char * name = mpc_lowcomm_monitor_set_name(set);
    uint64_t size = mpc_lowcomm_monitor_set_peer_count(set);

    printf("-------------------------\n");
    printf("SET ID : %u\n", uid);
    printf("SIZE   : %lu\n", size);
    printf("CMD    : %s\n", name);
    if(*verbose)
    {
        printf("PEER LIST:\n");
        mpc_lowcomm_monitor_peer_info_t * peers = malloc(size *sizeof(mpc_lowcomm_monitor_peer_info_t));
        mpc_lowcomm_monitor_set_peers(set, peers, size);

        uint64_t i;

        for( i = 0 ; i < size; i++)
        {
            printf(" - %lu : %s @ %s\n", peers[i].uid, mpc_lowcomm_peer_format(peers[i].uid), peers[i].uri);
        }

        free(peers);
    }
    printf("-------------------------\n");

}



void print_sets(int argc, char *argv[])
{
    int verbose = 0;
    if(argc > 2)
    {
        if(!strcmp(argv[2], "-v"))
        {
            verbose = 1;
        }
    }
    mpc_lowcomm_monitor_set_iterate(set_callback, &verbose);

}



int conn_callback(mpc_lowcomm_monitor_set_t set, void * arg)
{
    uint64_t size = mpc_lowcomm_monitor_set_peer_count(set);

    mpc_lowcomm_monitor_peer_info_t * peers = malloc(size *sizeof(mpc_lowcomm_monitor_peer_info_t));
    mpc_lowcomm_monitor_set_peers(set, peers, size);

    uint64_t i;

    for( i = 0 ; i < size; i++)
    {
        mpc_lowcomm_monitor_retcode_t ret;
        mpc_lowcomm_peer_uid_t peer_uid = peers[i].uid;

        mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_connectivity(peer_uid, &ret);

        if(ret == MPC_LOWCOMM_MONITOR_RET_SUCCESS)
        {
            mpc_lowcomm_monitor_args_t *args = mpc_lowcomm_monitor_response_get_content(resp);

            int j;

            printf("%lu [label=\"%lu %s\"]\n", peer_uid, peer_uid, mpc_lowcomm_peer_format(peer_uid));


            for(j = 0 ; j < args->connectivity.peers_count; j++)
            {
                char buff[128];
                printf("//%s -- %s\n", mpc_lowcomm_peer_format_r(peer_uid, buff, 128), mpc_lowcomm_peer_format(args->connectivity.peers[j]));
                printf("%lu -- %lu\n", peer_uid, args->connectivity.peers[j]);
            }

            mpc_lowcomm_monitor_response_free(resp);
        }

    }

    free(peers);

}



void get_connectivity(int argc, char ** argv)
{
    printf("graph G{\n");
    printf("graph[concentrate=true]\n");

    mpc_lowcomm_monitor_set_iterate(conn_callback, NULL);

    printf("}\n");


}


void help(int argc, char *argv[])
{
    fprintf(stderr, "Usage of %s:\n", argv[0]);
    fprintf(stderr, "%s [sets,help] [EXTRA ARGS]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "- sets          : print running sets (default)\n");
    fprintf(stderr, "     -v be verbose and print peers\n");
    fprintf(stderr, "- conn          : output overlay network map in Graphviz\n");
    fprintf(stderr, "- help/-h/--help: print this help\n");
    fprintf(stderr, "Advanced debug:\n");
    fprintf(stderr, "- syncconn      : output own program map (only current set)\n");


}

void syncdump(int argc, char *argv[])
{
    mpc_lowcomm_monitor_synchronous_connectivity_dump();
}


int main( int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    /* Set default and get mode if present */

    char * mode = "sets";

    if(argc > 1)
    {
        mode = argv[1];
    }


    /* Alternate modes */

    if(!strcmp(mode, "sets"))
    {
        print_sets(argc, argv);
    }
    else if(!strcmp(mode, "conn"))
    {
        get_connectivity(argc, argv);
    }
    else if(!strcmp(mode, "syncconn"))
    {
        syncdump(argc, argv);
    }
    else if(!strcmp(mode, "help") || !strcmp(mode, "-h") || !strcmp(mode, "--help"))
    {
        help(argc, argv);
    }
    else
    {
        fprintf(stderr, "No such argument %s\n", argv[1]);
        return 1;
    }


    MPI_Finalize();

    return 0;
}