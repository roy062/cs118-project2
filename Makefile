CC=g++
CCFLAGS=-std=c++11 -c -g
LFLAGS=

all: my-router.o read-file.o read-dv.o write-table.o
	$(CC) *.o -o my-router $(LFLAGS)
	$(CC) -std=c++11 -g inject-packet.cpp -o inject-packet $(LFLAGS)

clean:
	rm -f *.o routing-output*

my-router.o: my-router.cpp node-info.h read-file.h router.h
	$(CC) $(CCFLAGS) my-router.cpp -o my-router.o

read-file.o: read-file.cpp node-info.h
	$(CC) $(CCFLAGS) read-file.cpp -o read-file.o

read-dv.o: read-dv.cpp read-dv.h
	$(CC) $(CCFLAGS) read-dv.cpp -o read-dv.o

write-table.o: write-table.cpp write-table.h router.h
	$(CC) $(CCFLAGS) write-table.cpp -o write-table.o
