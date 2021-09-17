.PHONY: all server client

CFLAGS = -Wall -O0 -g
CFLAGS += -Itlib

LIBS = -lrdmacm -libverbs
LIBS += -lX11 -lXrandr -lXext

SRV_LIBS = tlib/transport/transport.cpp tlib/transport/transport_base.cpp tlib/transport/transport_rdmacm.cpp
SRV_LIBS += tlib/transport/transport.pb.cc
SRV_LIBS += tlib/capture/capture.cpp

CLI_LIBS = tlib/render/render.cpp

all:
	make server
	make client
server:
	g++ $(CFLAGS) $(SRV_LIBS) $(LIBS) server/server.cpp -o ../bin/server.o -std=c++17
client:
	g++ $(CFLAGS) $(CLI_LIBS) $(LIBS) client/client.cpp -o ../bin/client.o -std=c++17