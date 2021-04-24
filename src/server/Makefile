all:
	gcc server_rdma.c -o server.o -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm -lpthread -lXtst -g
clean:
	rm -f server.o
nox11:
	gcc server_no_x11.c -o server.o -std=gnu99 -libverbs -lrdmacm -lpthread -lm -g
