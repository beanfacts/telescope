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
}

void tsc_client_ui::loop()
{
    int x = 0;
    while (!x)
    {
        x = tsc_client_ui::refresh();
    }
    printf("Cleaning up...\n");
    cleanup();
}


void tsc_client_ui::set_default_values(int width, int height)
{
    ui_data.address_str = (char *) calloc(1, host_max_len);
    ui_data.state = _tsc_SETUP;
    ui_data.port = 9999;
    ui_data.fps = 60;
    ui_data.res_x = width;
    ui_data.res_y = height;
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
            ImGui::InputTextWithHint("##host", "127.0.0.1:9999", ui_data.address_str, host_max_len);
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

void tsc_client_ui::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
