CC=g++
CCFLAGS=-std=c++11 -c -g
LFLAGS=

all: my-router.o read-file.o
	$(CC) *.o -o my-router $(LFLAGS)

clean:
	rm -f *.o

my-router.o: my-router.cpp node-info.h read-file.h
	$(CC) $(CCFLAGS) my-router.cpp -o my-router.o

read-file.o: read-file.cpp node-info.h
	$(CC) $(CCFLAGS) read-file.cpp -o read-file.o
