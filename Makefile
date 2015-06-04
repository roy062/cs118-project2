CC=g++
CCFLAGS=-std=c++0x -c -g
LFLAGS= -lpthread

all: my-router.o read.o write.o
	$(CC) *.o -o my-router $(LFLAGS)
	$(CC) -std=c++0x -g inject-packet.cpp -o inject-packet $(LFLAGS)

clean:
	rm -f *.o routing-output*

clean-log:
	rm -f routing-output*

my-router.o: my-router.cpp read.h write.h router.h
	$(CC) $(CCFLAGS) my-router.cpp -o my-router.o

read.o: read.cpp read.h
	$(CC) $(CCFLAGS) read.cpp -o read.o

write.o: write.cpp write.h router.h
	$(CC) $(CCFLAGS) write.cpp -o write.o
