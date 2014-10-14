CC = gcc
CFLAGS = -g -Wall
INCLUDE = -I include/ 
LIBS = lib/libhttp.a -lpthread
webserver: main.c http_conn.c server.c lib/libhttp.a
	$(CC) -o $@ main.c server.c http_conn.c $(LIBS) $(INCLUDE)

all: webserver

clean:
	rm webserver
