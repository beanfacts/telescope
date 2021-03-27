all:
	gcc draw_display.c -o server -std=c99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm
	gcc screen_cast_receiver.c -o client -std=c99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm
clean:
	rm server