#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <GLFW/glfw3.h>

class tsc_client_ui {

public:

    void init(int width, int height);

    void loop();

private:

    GLFWwindow *window;
    const char* glsl_version = "#version 130";
    ImGuiIO io;

    const int host_max_len = 256;
    const int port_max_len = 6;

    typedef enum {
        _tsc_SETUP      = 0,    // Setting up
        _tsc_CONNREQ    = 1,    // User requested connection
        _tsc_RUNNING    = 2     // Running
    } _ui_enum;

    typedef struct {
        _ui_enum state;
        bool enable_rdma;
        bool enable_ucx;
        int scr_width;
        int scr_height;
        char *address_str;
        int port;
        int res_x;
        int res_y;
        int fps;
    } _ui_data;

    _ui_data ui_data;

    void set_default_values(int width, int height);

    void init_window(int width, int height);

    void init_imgui();

    void init_fonts();

    void refresh_ui();

    int refresh();

    void cleanup();

};
