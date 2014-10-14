#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <stdbool.h>

#define  FILENAME_LEN     200
#define  READ_BUFFER_SIZE 2048
#define  WRITE_BUFFER_SIZE 1024

enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
struct request_file
{
    char c_real_file[ FILENAME_LEN ];
    int file_d;
    char* c_url;
    char* c_version;
    char* c_host;
    off_t f_sent;
    struct stat c_file_stat;
};


struct request_info
{
    char c_read_buf[ READ_BUFFER_SIZE ];
    int c_read_idx;
    int c_checked_idx;
    int c_start_line;
    char c_write_buf[ WRITE_BUFFER_SIZE ];
    int c_write_idx;
};

struct http_conn
{
    enum METHOD c_method;
    enum CHECK_STATE c_check_stat;
    enum HTTP_CODE c_http_code;
    enum LINE_STATUS c_line_stat;

    bool keep_live;
    bool first_write;
    char *c_host;
    char *c_agent; 
    int  c_sockfd;
    int  c_content_length;
    struct sockaddr_in c_address;
    struct request_file r_file;
    struct request_info r_info;
};

extern int epollfd;
extern int user_count;
void http_close_conn(struct http_conn * cli );
void http_init(struct http_conn * users, int http_connfd,\
			struct sockaddr_in client_addr);
bool http_read(struct http_conn * cli);
bool http_Write(struct http_conn *cli);
void http_process(struct http_conn * cli);

bool process_write( struct http_conn *cli, enum HTTP_CODE ret );
enum HTTP_CODE process_read(struct http_conn * cli);
enum HTTP_CODE parse_request_line( char* text ,struct http_conn *cli);
enum HTTP_CODE parse_headers( char* text ,struct http_conn *cli);
enum HTTP_CODE parse_content( char* text ,struct http_conn * cli);
enum HTTP_CODE do_request(struct http_conn * cli);
enum LINE_STATUS parse_line(struct http_conn * cli);
bool add_response( struct http_conn *cli,const char* format, ... );
bool add_status_line( struct http_conn *cli,int status, const char* title );
bool add_headers( struct http_conn *cli, int content_len );
bool add_content( const char* content );

#endif
