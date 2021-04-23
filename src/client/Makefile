all:
	gcc telescope_client.c -o client.o -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lX11-xcb -lxcb-xfixes -lxcb -lpthread -lm -g
clean:
	rm client.o
nox11:
	gcc client_no_x11.c -o client.o -std=gnu99 -libverbs -lrdmacm -lpthread -lm -g