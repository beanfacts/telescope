#include <capture.hpp>

/*
    Setup the capture system.
    tsc_screen *screen -> should have capture_type populated.
*/
int tsc_init_capture(tsc_screen *scr)
{

    if (scr->capture_type == TSC_CAPTURE_X11)
    {
        // Prepare X server connection for multithreading
        ret = XInitThreads();
        if (!ret){
            fprintf(stderr, "X11 threading error: %d\n", ret);
            return 1;
        }
        else
        {
            printf("X11 MT ready (%d) \n", ret);
        }

        // Grab the root display
        // On some X servers and Xwayland, this will return a black screen.
        scr->x_display = XOpenDisplay(NULL);
        if(!scr->x_display)
        {
            fprintf(stderr, "Could not open a connection to the X server\n");
            return 1;
        }

        scr->x_window = XDefaultRootWindow(scr->x_display);
        scr->x_screen_no = (scr->x_display);

        // Check the X server supports shared memory extensions
        if(!XShmQueryExtension(scr->x_display))
        {
            XCloseDisplay(scr->x_display);
            fprintf(stderr, "Your X server does not support shared memory extensions.\n");
            return 1;
        }

        scr->width  = XDisplayWidth(scr->x_display, scr->x_screen_no);
        scr->height = XDisplayHeight(scr->x_display, scr->x_screen_no);
        
        XRRScreenConfiguration *xrr = XRRGetScreenInfo(scr->x_display, scr->x_window);
        scr->fps = (uint32_t) XRRConfigCurrentRate(xrr);

        printf("Capturing X Screen %d @ %d x %d \n", scr->x_screen_no, scr->width, scr->height);
        return 0;
    }

}