#include "ui.hpp"
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


void tsc_client_ui::init(int width, int height)
{
    init_window(width, height);
    set_default_values(width, height);
    init_imgui();
    init_fonts();
    init_stream();
}


int tsc_client_ui::wait_request()
{
    while (ui_data.state == _tsc_SETUP)
    {
        if (glfwWindowShouldClose(window)) return -1;
        refresh();
        if (ui_data.state == _tsc_CONNREQ)
        {
            return 0;
        }
    }
    return 1;
}


void tsc_client_ui::set_default_values(int width, int height)
{
    ui_data.host_str    = (char *) calloc(1, host_max_len);
    ui_data.port_str    = (char *) calloc(1, port_max_len);
    ui_data.ifname_str  = (char *) calloc(1, ifname_max_len);
    ui_data.state       = _tsc_SETUP;
    ui_data.fps         = 60;
    ui_data.res_x       = width;
    ui_data.res_y       = height;
}

void tsc_client_ui::init_window(int width, int height)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        throw std::runtime_error("GLFW init failed.");

    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    window = glfwCreateWindow(width, height, "Telescope", NULL, NULL);
    if (window == NULL)
        throw std::runtime_error("GLFW init failed.");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
}

void tsc_client_ui::init_imgui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}


void tsc_client_ui::init_fonts()
{
    //io.Fonts->AddFontFromFileTTF("InterVariable.ttf", 14);
}


void tsc_client_ui::refresh_ui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    if (ui_data.state == _tsc_SETUP)
    {
        ImGui::Begin("Telescope Configuration");
        
        if (ImGui::TreeNode("Transport"))
        {
            ImGui::Checkbox("RDMACM", &ui_data.enable_rdma);
            ImGui::TreePop();
        }
        
        if (ImGui::TreeNode("Connection"))
        {
            ImGui::InputTextWithHint("RDMA Interface", "ib0_mlx5",
                    ui_data.ifname_str, ifname_max_len);
            ImGui::InputTextWithHint("Hostname", "192.168.1.123",
                    ui_data.host_str, host_max_len);
            ImGui::InputTextWithHint("Port", "9999", 
                    ui_data.port_str, port_max_len);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Streaming"))
        {
            ImGui::InputInt("X", &ui_data.res_x, 0);
            ImGui::InputInt("Y", &ui_data.res_y, 0);
            ImGui::InputInt("FPS", &ui_data.fps, 0);
            ImGui::TreePop();
        }
        
        ImGui::Separator();

        if (ImGui::Button("Connect"))
        {
            ui_data.state = _tsc_CONNREQ;
        }

        ImGui::SameLine();

        ImGui::Text("Estimated bandwidth: %.2f Gbit/s", 
                (double) ui_data.res_x * (double) ui_data.res_y 
                * (double) ui_data.fps * 32 / (double) (1 << 30));

        if (ui_data.state == _tsc_CONNREQ)
        {
            ImGui::Text("Connecting...");
        }

        ImGui::End();
    }
    
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}


int tsc_client_ui::refresh()
{
    int out = glfwWindowShouldClose(window);
    if (!out)
    {
        glfwPollEvents();
        refresh_ui();
    }
    return out;
}


int tsc_client_ui::refresh(tsc_frame_buffer *buf) {
    int out = glfwWindowShouldClose(window);
    if (!out)
    {
        glfwPollEvents();
        stream(buf);
    }
    return out;
}


void tsc_client_ui::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}


void tsc_client_ui::lock_cursor()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!glfwRawMouseMotionSupported())
        throw std::runtime_error("Raw mouse motion unsupported.");
    
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}


void tsc_client_ui::unlock_cursor()
{
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void tsc_client_ui::stream(tsc_frame_buffer *buf) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buf->display.width, buf->display.height, 0, buf->pixmap, GL_UNSIGNED_BYTE, buf->buffer);
    redraw();
}


void tsc_client_ui::redraw(){
    int width,height;
    // evrytime we redraw the scene we get the window size and reshape accordingly
    glfwGetWindowSize(window, &width, &height);
    reshapeScene(width,height);
    // clear the screen so we can draw another frame
    glClearColor(0.3, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw a rectangle
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(-1.0,1.0);
    glTexCoord2f(1.0, 0.0); glVertex2f( 1.0,1.0);
    glTexCoord2f(1.0, 1.0); glVertex2f( 1.0,-1.0);
    glTexCoord2f(0.0, 1.0); glVertex2f(-1.0,-1.0);
    glEnd();

    // draw onto screen
    glfwSwapBuffers(window);
}


void tsc_client_ui::reshapeScene(GLint width, GLint height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    int w = height * aspect_ratio;           // w is width adjusted for aspect ratio
    int left = (width - w) / 2;
    // moves the viewport around to get the screen size scaled
    glViewport(left, 0, w, height);       // fix up the viewport to maintain aspect ratio
    glMatrixMode(GL_MODELVIEW);
}


void tsc_client_ui::init_stream() {
    //init glew
    // glew gets the fuctions working for "new" opengl
    if (glewInit() != GLEW_OK) {
        std::cout << "Error: Failed to initialise Glew";
    }

    // enable features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // create a texture and set the parameters
    // idk if the parameters are needed but its there
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


}

