INCLUDE= -I ../include

libhttp.a: csapp.o cpu_time.o threadpool.o locker.o
	ar rv  $@ csapp.o cpu_time.o threadpool.o locker.o
	ranlib $@

csapp.o: csapp.c
	gcc -c csapp.c $(INCLUDE)

locker.o: locker.c
	gcc -c locker.c $(INCLUDE)

cpu_time.o: cpu_time.c
	gcc -c cpu_time.c $(INCLUDE) 

threadpool.o:  threadpool.c
	gcc -c threadpool.c $(INCLUDE) 

all: libhttp.a

clean:
	rm *.o  *.a 
