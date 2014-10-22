#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <libgen.h>

#include "http_conn.h"
#include "threadpool.h"

#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000

void show_error( int connfd, const char* info )
{
	printf( "%s", info );
	send( connfd, info, strlen( info ), 0 );
	Close( connfd );
}

/*The main tree of server*/
int server(int listenfd)
{
	static int i;
	static int number;
	struct epoll_event events[MAX_EVENT_NUMBER];
	struct http_conn * users = malloc(sizeof(struct http_conn));
    

	epollfd = Epoll_create( 5 );
	addfd(epollfd,listenfd,false);
	while( true ){
		/* We call the epoll_wait to block untial a request done*/
		if((number = Epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 )) <= -1)
			continue;
		/*loop scan the epoll event*/
		for ( i = 0; i < number; i++ ){
			int sockfd = events[i].data.fd;
			
			if( sockfd == listenfd ){
			/*Here is a connect request*/
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof( client_address );
				int connfd = Accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
				if ( connfd < 0 )
					continue;

				if( user_count >= MAX_FD ){
					show_error( connfd, "Internal server busy" );
					continue;
				}
				/*connect success ,now go to initial the http struct*/
				http_init( users + connfd, connfd, client_address );
            		
			}else if( events[i].events & EPOLLIN ){
			/*The socket could be read. */
				if( http_read(users + sockfd) ){
					threadpool_append( users + sockfd );
				}else{
					http_close_conn(users + sockfd);
				}
			
			}else if( events[i].events & EPOLLOUT ){
			/*The socket could be write. */
				if( !http_Write(users + sockfd)){
					http_close_conn(users + sockfd);
				}
			
			}else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ){
			/*The socket had error. */
				http_close_conn(users + sockfd);
			
			}else{
				/*anything else happend*/
			}
		}	/*Loop scan epoll event end*/

	}
	
	close( epollfd );
	close( listenfd );
	return 0;
}
