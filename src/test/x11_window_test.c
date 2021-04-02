#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    
    Display* dsp = XOpenDisplay(NULL);
    Window* root_win;
    int screen = XDefaultScreen(dsp);
    /*
    int disp_loc_x, disp_loc_y = 0;
    unsigned int disp_width, disp_height, disp_bpp, disp_border = 0;
    /* Get the X11 display parameters to allocate the required amount of memory
    XGetGeometry(dsp, XDefaultRootWindow(dsp), root_win, &disp_loc_x, &disp_loc_y,
        &disp_width, &disp_height, &disp_border, &disp_bpp);

    printf( "Display parameters:\n"
            " X Position   : %u\n Y Position   : %u\n"
            " Disp. Width  : %u\n Disp. Height : %u\n Disp. Depth  : %u\n"
            " Disp. Border : %u\n",
            disp_loc_x, disp_loc_y, disp_width, disp_height, disp_bpp, disp_border
        );
    */

    int i, j;

    XFixesCursorImage *cursor = XFixesGetCursorImage(dsp);
    char* px = cursor->pixels;
    long unsigned int data;

    for (int i = 0; i < (cursor->height * cursor->width) * 4; i+=4) {
        data = *(cursor->pixels + i);
        if (data) { printf("X"); } else { printf("."); }
        if (i % (24 * 4) == 0)
        {
            printf("\n");
        }
    }
    
}

