#include "http_conn.h"

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
const char* doc_root = "/var/www/html";

int epollfd = -1;
int user_count = 0;

char *get_line(struct http_conn * cli){
	return cli->r_info.c_read_buf + cli->r_info.c_start_line;
}

int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    Fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void addfd( int epollfd, int fd, bool one_shot )
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if( one_shot )
    {
        event.events |= EPOLLONESHOT;
    }
    Epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
    return;
}

void removefd( int epollfd, int fd )
{
    Epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    return;
}

void modfd( int epollfd, int fd, int ev )
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    Epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
    return;
}

void http_close_conn(struct http_conn *cli )
{
    int sockfd = cli->c_sockfd;
    int file_d  = cli->r_file.file_d;
    if(file_d)
	Close(file_d);

    removefd( epollfd, sockfd );
    Close(sockfd);
    user_count--;
    return;
}


void http_init(struct http_conn * users, int http_connfd,\
			 struct sockaddr_in client_addr)
{
    int socket_fd = http_connfd;
    memset(users,'\0',sizeof(struct http_conn));

    users -> c_sockfd = http_connfd;
    users -> c_address = client_addr;

    users->c_method = GET;
    users->c_check_stat = CHECK_STATE_REQUESTLINE;
    users->c_line_stat = LINE_OK;
    users->c_http_code = NO_REQUEST;
    users->first_write = true;
    user_count ++;
    addfd(epollfd, socket_fd, true); 
    return;
}

enum LINE_STATUS parse_line(struct http_conn * cli)
{
    char temp;
    int checked_idx = cli->r_info.c_checked_idx;
    int read_idx = cli->r_info.c_read_idx;
    char * read_buf = cli->r_info.c_read_buf;
    cli->r_info.c_start_line = cli->r_info.c_checked_idx;
    for ( ; checked_idx < read_idx; ++checked_idx )
    {
        temp = read_buf[ checked_idx ];
        if ( temp == '\r' )
        {
            if ( ( checked_idx + 1 ) == read_idx )
            {
		cli->r_info.c_checked_idx = checked_idx;
                return LINE_OPEN;
            }
            else if ( read_buf[ checked_idx + 1 ] == '\n' )
            {
                read_buf[ checked_idx++ ] = '\0';
                read_buf[ checked_idx++ ] = '\0';
		cli->r_info.c_checked_idx = checked_idx;
                return LINE_OK;
            }

	    cli->r_info.c_checked_idx = checked_idx;
            return LINE_BAD;
        }
        else if( temp == '\n' )
        {
            if( ( checked_idx > 1 ) && ( read_buf[ checked_idx - 1 ] == '\r' ) )
            {
                read_buf[ checked_idx-1 ] = '\0';
                read_buf[ checked_idx++ ] = '\0';
		cli->r_info.c_checked_idx = checked_idx;
                return LINE_OK;
            }
	    cli->r_info.c_checked_idx = checked_idx;
            return LINE_BAD;
        }
    }
    cli->r_info.c_checked_idx = checked_idx;
    return LINE_OPEN;
}

bool http_read(struct http_conn * cli)
{
    int read_idx = cli -> r_info.c_read_idx;
    int sockfd = cli->c_sockfd;
    char * read_buf = cli->r_info.c_read_buf;
    if( read_idx >= READ_BUFFER_SIZE )
    {
        return false;
    }

    int bytes_read = 0;
    while( true )
    {
        bytes_read = recv( sockfd, read_buf + read_idx, READ_BUFFER_SIZE - read_idx, 0 );
        if ( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        else if ( bytes_read == 0 )
        {
            return false;
        }
	read_idx += bytes_read;
    }
    cli -> r_info.c_read_idx = read_idx;
    return true;
}

enum HTTP_CODE parse_request_line( char* text ,struct http_conn *cli)
{
    char *p_url;
    char *c_version;
    p_url = strpbrk( text, " \t" );
    if ( ! p_url )
    {
        return BAD_REQUEST;
    }
    *p_url++ = '\0';

    char* method = text;
    if ( strcasecmp( method, "GET" ) == 0 )
    {
        cli->c_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    p_url += strspn( p_url, " \t" );
    c_version = strpbrk( p_url, " \t" );
    if ( ! c_version )
    {
        return BAD_REQUEST;
    }
    *c_version++ = '\0';
    c_version += strspn( c_version, " \t" );
    if ( strncasecmp( c_version, "HTTP/1.1", 8 ) != 0 && \
    		strncasecmp( c_version, "HTTP/1.0", 8 ) != 0)
    {
        return BAD_REQUEST;
    }

    if ( strncasecmp( p_url, "http://", 7 ) == 0 )
    {
        p_url += 7;
        p_url = strchr( p_url, '/' );
    }
    
    if ( ! p_url || p_url[ 0 ] != '/' )
    {
        return BAD_REQUEST;
    }

    cli->c_check_stat = CHECK_STATE_HEADER;
    cli->r_file.c_url = p_url;
    return NO_REQUEST;
}

enum HTTP_CODE parse_headers( char *text, struct http_conn *cli)
{
    if( text[ 0 ] == '\0' )
    {
        if( cli->c_method == HEAD )
        {
            return GET_REQUEST;
        }

        if ( cli->c_content_length != 0 )
        {
            cli->c_check_stat = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }else{
            cli->c_check_stat = CHECK_STATE_REQUESTLINE;
            return GET_REQUEST;
	}
    }
    else if ( strncasecmp( text, "Connection:", 11 ) == 0 )
    {
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 )
        {
            cli->keep_live = true;
        }
    }
    else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 )
    {
        text += 15;
        text += strspn( text, " \t" );
        cli->c_content_length = atol( text );
    }
    else if ( strncasecmp( text, "Host:", 5 ) == 0 )
    {
        text += 5;
        text += strspn( text, " \t" );
        cli->c_host = text;
    }
    else if ( strncasecmp( text, "User-agent:", 11 ) == 0 )
    {
        text += 11;
        text += strspn( text, " \t" );
        cli->c_agent = text;
    }
    else if ( strncasecmp( text, "Accept:", 7 ) == 0 )
    {
        text += 7;
        text += strspn( text, " \t" );
        cli->c_host = text;
    }
    else if ( strncasecmp( text, "Range:", 6 ) == 0 )
    {
        text += 6;
        text += strspn( text, " \t" );
	text = strchr (text, '=');
        cli->r_file.f_sent = atoi(text+1);
	if(cli->r_file.f_sent)
		cli->first_write = false;
    }
    else
    {
        printf( "oop! unknow header %s\n", text );
    }

    return NO_REQUEST;

}

enum HTTP_CODE parse_content( char* text ,struct http_conn * cli)
{
    int read_idx = cli->r_info.c_read_idx;
    int checked_idx = cli->r_info.c_checked_idx;
    int content_length = cli->c_content_length;
    if ( read_idx >= ( content_length + checked_idx ) )
    {
        text[ content_length ] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

enum HTTP_CODE process_read(struct http_conn * cli)
{
    enum HTTP_CODE ret = NO_REQUEST;
    enum LINE_STATUS line_stat = LINE_OPEN;
    char* text = NULL;

    while ( ( ( cli->c_check_stat == CHECK_STATE_CONTENT ) && ( cli->c_line_stat == LINE_OK  ) )
                || ( ( cli->c_line_stat = parse_line(cli) ) == LINE_OK ) )
    {
        text = get_line(cli);
        printf( "got 1 http line: %s\n", text );

        switch (cli->c_check_stat )
        {
            case CHECK_STATE_REQUESTLINE:
            {
		//printf("check_state_requestline\n");
                ret = parse_request_line( text ,cli);
                if ( ret == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
		//printf("check_state_header %s\n",text);
                ret = parse_headers( text, cli );
                if ( ret == GET_REQUEST )
                {
                    return do_request(cli);
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
		//printf("check_state_content\n");
                ret = parse_content( text, cli);
                if ( ret == GET_REQUEST )
                {
                    return do_request(cli);
                }
                line_stat = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }

    return NO_REQUEST;
}

enum HTTP_CODE do_request(struct http_conn * cli)
{
    
    char * real_file = cli-> r_file.c_real_file;
    char * url = cli-> r_file.c_url;
    struct stat file_stat = cli->r_file.c_file_stat;
    int *file_d = &cli->r_file.file_d;
    strcpy( real_file, doc_root );
    int len = strlen( doc_root );

    strncpy( real_file + len, url, FILENAME_LEN - len - 1 );
    if(strlen(url)==1){
    	strncpy( real_file + len, "/index.html", FILENAME_LEN - len - 1 );
    }
    if ( stat( real_file, &file_stat ) < 0 )
    {
        return NO_RESOURCE;
    }
    
    if ( ! ( file_stat.st_mode & S_IROTH ) )
    {
        return FORBIDDEN_REQUEST;
    }

    if ( S_ISDIR( file_stat.st_mode ) )
    {
        return BAD_REQUEST;
    }
    *file_d = Open (real_file,O_RDONLY|O_NONBLOCK,0);

    cli->r_file.c_file_stat = file_stat;
    return FILE_REQUEST;
}

bool http_Write(struct http_conn *cli)
{
    char* write_buf = cli->r_info.c_write_buf;
    int write_idx = cli->r_info.c_write_idx;
    int sockfd = cli->c_sockfd;
    int file_d = cli->r_file.file_d;
    struct stat file_stat = cli->r_file.c_file_stat;
    bool keep_live = cli->keep_live;

    /*
     * If it is first send message to client,we should send the http head first;
    */
    if(cli->first_write)
	Write(sockfd,write_buf,write_idx);

    if(file_d  <= 0 )
	return keep_live?true:false;

    while( 1 )
    {
	//printf("Sendfile %d size %d offset %d\n",file_d,file_stat.st_size,cli->r_file.f_sent);
        if( !Sendfile( sockfd, file_d, &(cli->r_file.f_sent), file_stat.st_size)){
		modfd(epollfd,sockfd,EPOLLOUT);
		return true;	
	}

        if ( cli->r_file.f_sent >= file_stat.st_size )
        {
            if( !keep_live )
            {
		struct linger linger = {1,3};
		Setsockopt(cli->c_sockfd, SOL_SOCKET, SO_LINGER, \
				&linger, sizeof(struct linger));
            	return false;
            }
	    close(file_d);
	    removefd(epollfd, sockfd);
            http_init(cli, sockfd, cli->c_address);
            return true;
        }
    }
}

bool add_response( struct http_conn *cli,const char* format, ... )
{
    int write_idx = cli->r_info.c_write_idx;
    char* write_buf = cli->r_info.c_write_buf;

    if( write_idx >= WRITE_BUFFER_SIZE )
    {
        return false;
    }
    va_list arg_list;
    va_start( arg_list, format );
    int len = vsnprintf( write_buf + write_idx,\
	 WRITE_BUFFER_SIZE - 1 - write_idx, format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - write_idx ) )
    {
        return false;
    }
    cli->r_info.c_write_idx += len;
    va_end( arg_list );
    return true;
}

bool add_status_line( struct http_conn *cli, int status, const char* title )
{
    return add_response( cli, "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool add_headers( struct http_conn *cli, int content_len )
{
	int range = cli->r_file.f_sent;
	int file_size = cli->r_file.c_file_stat.st_size;
	bool keep_live = cli->keep_live;
	add_response(cli, "Content-Length: %d\r\n", content_len - cli->r_file.f_sent );
	add_response(cli, "Range: bytes%d-%d\r\n", range,file_size);
	add_response(cli, "Connection: %s\r\n", ( keep_live == true ) ? "keep-alive" : "close" );
	add_response(cli, "Content-Type: text/html; charset=UTF-8\r\n");
	add_response(cli, "%s", "\r\n" );
	return true;
}

bool process_write( struct http_conn *cli, enum HTTP_CODE ret )
{
    struct stat file_stat = cli->r_file.c_file_stat;
    switch ( ret )
    {
        case INTERNAL_ERROR:
        {
            add_status_line(cli, 500, error_500_title );
            add_headers( cli,strlen( error_500_form ) );
            if ( ! add_response( cli, error_500_form ) )
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(cli, 400, error_400_title );
            add_headers( cli, strlen( error_400_form ) );
            if ( ! add_response( cli, error_400_form ) )
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
	    cli->keep_live = false;
            add_status_line(cli, 404, error_404_title );
            add_headers( cli, strlen( error_404_form ) );
            if ( ! add_response( cli, error_404_form ) )
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line(cli, 403, error_403_title );
            add_headers( cli, strlen( error_403_form ) );
            if ( ! add_response( cli, error_403_form ) )
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line(cli, 200, ok_200_title );
            if ( file_stat.st_size != 0 )
            {
                add_headers( cli, file_stat.st_size );
                return true;
            }
            else
            {
                const char* ok_string = "<html><body></body></html>";
                add_headers( cli, strlen( ok_string ) );
                if ( ! add_response( cli, ok_string ) )
                {
                    return false;
                }
            }
	    break;
        }
        default:
        {
            return false;
        }
    }
    return true;
}

void http_process(struct http_conn * cli)
{
    enum HTTP_CODE read_ret;
    read_ret = process_read(cli);
    int sockfd = cli->c_sockfd;
    if ( read_ret == NO_REQUEST )
    {
        modfd( epollfd, sockfd, EPOLLIN );
        return;
    }
    bool write_ret = process_write( cli, read_ret );
    if ( ! write_ret )
    {
        http_close_conn(cli);
    }

    modfd( epollfd, sockfd, EPOLLOUT );
}
