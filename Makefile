CC = gcc
CFLAGS = -g -Wall -lpthread
INCLUDE = -I include/ 
LIBS = lib/libhttp.a
webserver: main.c http_conn.c server.c hand.c get_config.c
	$(CC)  $(CFLAGS) -o $@ main.c server.c hand.c http_conn.c get_config.c $(LIBS) $(INCLUDE)

all: webserver

clean:
	rm webserver
