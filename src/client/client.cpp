#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <GLFW/glfw3.h>

#include <unistd.h>

#include "ui.hpp"

int main(int argc, char **argv)
{
    tsc_client_ui ui;
    ui.init(1280, 720);
    sleep(100);
}