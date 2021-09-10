all:
	gcc -O1 -o client.o client_rdma.c receive_shm.c input_xcb.c -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lX11-xcb -lxcb-xfixes -lxcb -lpthread -lm
debug:
	gcc client_rdma.c -O0 -o client.o -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lX11-xcb -lxcb-xfixes -lxcb -lpthread -lm -g
clean:
	rm client.o