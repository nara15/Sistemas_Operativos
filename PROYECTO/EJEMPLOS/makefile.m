all: main

threadpool.o : threadpool.c threadpool.h
	gcc -g -S -Wall threadpool.c
	gcc -g -c threadpool.s	

example.o: example.c threadpool.h
	gcc -g -Wall -c example.c

main: threadpool.o example.o
	gcc -g -pthread threadpool.o example.o 
	rm -f threadpool.o example.o threadpool.s


