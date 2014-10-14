#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "locker.h"
#include "http_conn.h"
#include "csapp.h"

void push_queue(struct http_conn * one_http);
void pop_queue();
void *run();
int make_threadpool( int thread_number );
int threadpool_append( struct http_conn *);
#endif
