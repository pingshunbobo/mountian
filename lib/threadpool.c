#include "threadpool.h"

pthread_mutex_t request_chain_mutex;
sem_t 		have_request_sem;

struct queue_node
{
        struct queue_node *prive;
        struct queue_node *next;
        struct http_conn  *http_data;
};
struct threadpool
{
    int thread_number;
    int max_requests;
    int now_requests;
    pthread_t* m_threads;
    struct queue_node * conn_queue_front;
    struct queue_node * conn_queue_rear;
} threadpool_data;

void push_queue(struct http_conn * one_http)
{
	struct queue_node * new_node;
	new_node = (struct queue_node *)malloc(sizeof(struct queue_node));
	new_node->http_data = one_http;
	
	if(threadpool_data.conn_queue_front == NULL){
		threadpool_data.conn_queue_front = new_node;
		new_node -> prive = NULL;
	}else{
		threadpool_data.conn_queue_rear->next = new_node;
		new_node -> prive = threadpool_data.conn_queue_rear;
	}
	new_node -> next = NULL;
	threadpool_data.conn_queue_rear = new_node;
}

void pop_queue()
{
	struct queue_node * first_node;
	first_node = threadpool_data.conn_queue_front;
	threadpool_data.conn_queue_front = first_node -> next;
	if(first_node->next)
		first_node->next->prive = NULL;
	free(first_node);
}

void *run()
{
    struct queue_node *first_node;
    struct http_conn *request;
    while ( 1 )
    {
        wait_get(&have_request_sem);
        lock(&request_chain_mutex);
        first_node = threadpool_data.conn_queue_front;

        if ( first_node == NULL )
        {
            unlock(&request_chain_mutex);
            continue;
        }
        request = first_node->http_data;
	pop_queue();
        unlock(&request_chain_mutex);
       
 	if ( ! request )
        {
            continue;
        }
        http_process(request);
     }
}

int make_threadpool( int thread_number ) 
{
    int i;
    threadpool_data.thread_number = thread_number;
    threadpool_data.conn_queue_front == NULL;
    threadpool_data.conn_queue_rear == NULL;

    pthread_t m_threads[ thread_number ];
    struct http_conn *request_pool = NULL;
    init_locker(&request_chain_mutex);
    init_sem (&have_request_sem);

    for ( i = 0; i < thread_number; ++i )
    {
        printf( "create the %dth thread\n", i );
        Pthread_create( m_threads + i, NULL, run, NULL );
        
	Pthread_detach( m_threads[i] ); 
    }
    return i;
}


int threadpool_append( struct http_conn* request )
{
     
    lock(&request_chain_mutex);
    if ( threadpool_data.now_requests > threadpool_data.max_requests )
    {
        unlock(&request_chain_mutex);
        return false;
    }
    push_queue( request );
    unlock(&request_chain_mutex);
    post(&have_request_sem);	//post signal say that add a http_connect!
    return true;
}
