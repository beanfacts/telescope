all:
	gcc server_rdma.c -o server.o -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm -lpthread -lXtst -g3 -O0
clean:
	rm -f server.o
nox11:
	gcc server_no_x11.c -o server.o -std=gnu99 -libverbs -lrdmacm -lpthread -lm -g -O0
test:
	gcc screencap.h -o screencap.o -std=gnu99 -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lm
	# gcc draw_display_server_no_c.c -o server_no_c.o -std=gnu99 -libverbs -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lrdmacm -lm -lpthread -lXtst -g -O0