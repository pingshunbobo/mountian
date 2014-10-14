#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdbool.h>

#include "pr_cpu_time.h"

void addsig( int sig, void( handler )(int), bool restart)
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    if(sigaction( sig, &sa, NULL ) <= -1){
	perror("sigaction");
    }
}


int main( int argc, char* argv[] )
{
    int ret = 0;
    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        exit(1);
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
    int num_thread = atoi(argv[3]);
    addsig( SIGPIPE, SIG_IGN ,false);
    addsig(SIGINT,pr_cpu_time,false);
    ret = make_threadpool( num_thread );
    if( ret != num_thread){
	printf("make %d thread\n");
    }

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int listenfd = Socket( PF_INET, SOCK_STREAM, 0 );
    Bind( listenfd, ( struct sockaddr* )&address, sizeof( address ));
    Listen( listenfd, 5 );

    server(listenfd);
    exit(-1);
}
