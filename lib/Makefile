INCLUDE= -I ../include

libhttp.a: syscall.o cpu_time.o threadpool.o locker.o
	ar rv  $@ syscall.o cpu_time.o threadpool.o locker.o 
	ranlib $@

syscall.o: syscall.c
	gcc -c syscall.c $(INCLUDE)

locker.o: locker.c
	gcc -c locker.c $(INCLUDE)

cpu_time.o: cpu_time.c
	gcc -c cpu_time.c $(INCLUDE) 

threadpool.o:  threadpool.c
	gcc -c threadpool.c $(INCLUDE) 

all: libhttp.a

clean:
	rm *.o  *.a 
