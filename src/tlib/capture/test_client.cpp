//
// Created by maomao on 03/10/2021.
//

// cout and stuff
#include <iostream>

//x11
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

//opengl stuff
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "capture_base.hpp"
#include "capture_x11.hpp"


//Display         *dpy;
GLFWwindow      *win;
int             dsp_width;
int             dsp_height;
XImage			*xim;
GLuint			texture_id;
// these are used for scaling
int             width;
int             height;
float aspect_ratio;
bool switcher;

void reshapeScene(GLint width, GLint height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    int w = height * aspect_ratio;           // w is width adjusted for aspect ratio
    int left = (width - w) / 2;
    // moves the viewport around to get the screen size scaled
    glViewport(left, 0, w, height);       // fix up the viewport to maintain aspect ratio
    glMatrixMode(GL_MODELVIEW);

    // glutPostRedisplay();
}

void Redraw() {
    // evrytime we redraw the scene we get the window size and reshape accordingly
    glfwGetWindowSize(win, &width, &height);
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
    glfwSwapBuffers(win);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        switcher= ! switcher;
}

/*                */
/*  MAIN PROGRAM  */
/*                */
int main(int argc, char *argv[]) {
    if (argc != 4){
        std::cout<<"Usage: ./client <display number> <width> <height> \n";
        return 1;
    }
    else {
        int dsp = atoi(argv[1]);
        dsp_width = atoi(argv[2]);
        dsp_height = atoi(argv[3]);
        aspect_ratio = float(dsp_width)/dsp_height;

        /* Initialize the library */
        if (!glfwInit())
            return -1;

        /* Create a windowed mode window and its OpenGL context */
        win = glfwCreateWindow(dsp_width, dsp_height, "Telescope", nullptr, nullptr);
        if (!win) {
            std::cout << "Error: Failed to Create glfw Window \n";
            glfwTerminate();
            return -1;
        }


        /* Make the window's context current */
        glfwMakeContextCurrent(win);


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

        // define and init the cap class
        tsc_capture_x11 cap;
        cap.init();
        std::vector<tsc_display> screens = cap.get_displays();
        tsc_frame_buffer *buf = cap.alloc_frame(dsp);
        tsc_frame_buffer *buf2 = cap.alloc_frame(1);
        glfwSetKeyCallback(win, key_callback);
        while (!glfwWindowShouldClose(win)) {
            // render and call the xshmget
            if (switcher){
                cap.update_frame(buf);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screens[dsp].width, screens[dsp].height, 0, GL_BGRA, GL_UNSIGNED_BYTE, buf->buffer);
            }else{
                cap.update_frame(buf2);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screens[dsp].width, screens[dsp].height, 0, GL_BGRA, GL_UNSIGNED_BYTE, buf2->buffer);
            }

            Redraw();
            glfwPollEvents();

        } /* while(1) */
    }
} /* int main(...) */