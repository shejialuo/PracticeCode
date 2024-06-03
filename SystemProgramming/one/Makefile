objs = a2p1.o a2p2.o
exetuables = a2p1 a2p2
fifos = fifo-0-1 fifo-0-2 fifo-0-3 fifo-1-0 fifo-2-0 fifo-3-0

all: a2p1 a2p2

a2p1.o: a2p1.c
	cc -c a2p1.c

a2p1: a2p1.o
	cc $< -o a2p1 -lpthread

a2p2.o: a2p2.cpp
	c++ -c a2p2.cpp

fifo:
	mkfifo $(fifos)

a2p2: a2p2.o fifo
	c++ $< -o a2p2

test_a2p1: a2p1 fmt.man
	./a2p1 4 fmt.man 3500

.PHONY: tar
tar:
	tar -czvf file.tar.gz $(files)

.PHONY: clean
clean:
	rm $(objs) $(exetuables) $(fifos)
