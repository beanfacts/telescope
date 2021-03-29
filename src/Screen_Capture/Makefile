all:
	gcc draw_display.c -o server.o -std=c99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm
	gcc screen_cast_receiver.c -o client.o -std=c99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm
clean:
	rm -f server.o
	rm -f client.o