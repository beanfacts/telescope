IMGUI_DIR = tlib/ui/imgui

CFLAGS = -Wall -Wextra -O1 -g
CFLAGS += -Itlib

COMMON_LIBS = -lrdmacm -libverbs -lpthread
COMMON_LIBS += `pkg-config --static --libs protobuf`

SRV_LIBS = -lX11 -lXrandr -lXext

CLI_LIBS = -lGL
CLI_LIBS += `pkg-config --static --libs glfw3`

COMMON_SRC = tlib/transport/transport.cpp tlib/transport/transport_tcp.cpp tlib/transport/transport_base.cpp tlib/transport/transport_rdmacm.cpp
COMMON_SRC += tlib/transport/transport.pb.cc tlib/transport/get_ip.c

SRV_SRC += tlib/capture/capture.cpp

CLI_SRC += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
CLI_SRC += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
CLI_SRC += tlib/ui/ui.cpp

CSTD = -std=c++17

CLI_FLAGS = -Itlib/ui -Itlib/ui/imgui

all:
	make server
	make client
server:
	$(CXX) $(CFLAGS) $(COMMON_SRC) $(SRV_SRC) $(SRV_LIBS) $(COMMON_LIBS) server/server.cpp -o ../bin/server $(CSTD)
client:
	$(CXX) $(CFLAGS) $(COMMON_SRC) $(CLI_SRC) $(CLI_LIBS) $(COMMON_LIBS) $(CLI_FLAGS) client/client.cpp -o ../bin/client $(CSTD) 