all:
	gcc telescope_client.c -o client.o -std=c99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lX11-xcb -lxcb-xfixes -lxcb -lpthread -lm -g
clean:
	rm client.o
