.PHONY: clean

CFLAGS  := -Wall -Werror -g
APPS    := client server

all: ${APPS}

client: common.o client.o
	gcc -o $@ $^ -lrdmacm -libverbs -lpthread

server: common.o server.o
	gcc -o $@ $^ -lrdmacm -libverbs -lpthread

clean:
	rm -f *.o ${APPS}