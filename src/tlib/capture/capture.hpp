/* Screen Capture Libraries */
#include <X11/Xlib.h>               // Base X11 library
#include <X11/extensions/XShm.h>    // Shared memory extensions
#include <X11/extensions/Xrandr.h>  // Get FPS

/* Internal Libraries */
#include "../common.hpp"

enum tsc_capture_type {
    TSC_CAPTURE_NONE        = 0,
    TSC_CAPTURE_X11         = 1,
    TSC_CAPTURE_PW          = 2,
    TSC_CAPTURE_NVFBC       = 3,
    TSC_CAPTURE_AMF         = 4,
    TSC_CAPTURE_LGSHM       = 5
};

int tsc_init_capture(tsc_screen *scr);