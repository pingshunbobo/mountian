#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdbool.h>

#include "threadpool.h"
#include "daemon.h"

extern int server(int);
extern void daemon_init();
extern int get_config(char* ip, int* port, int* thread_num);

int main (  )
{
	int ret;
	static char ip[20];
	static int port;
	static int num_thread;
	if(get_config(ip,&port,&num_thread)){
		printf("get config error\n");
		exit(1);
	}
	/* Let the process run in daemon*/
	daemon_init();
	ret = make_threadpool( num_thread );
	if( ret != num_thread){
		printf("make %d thread\n",ret);
	}

	struct sockaddr_in address;
	bzero( &address, sizeof( address ) );
	address.sin_family = AF_INET;
	inet_pton( AF_INET, ip, &address.sin_addr );
	address.sin_port = htons( port );

	int listenfd = Socket( PF_INET, SOCK_STREAM, 0 );
	Bind( listenfd, ( struct sockaddr* )&address, sizeof( address ));
	Listen( listenfd, 5 );

	/*Now we just run the serve funtion*/
	server(listenfd);
	
	/*It should not return to here normaly!*/
	exit(-1);
}
