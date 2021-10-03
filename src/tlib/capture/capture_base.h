//
// Created by maomao on 03/10/2021.
//

#ifndef TELESCOPE_CAPTURE_H
#define TELESCOPE_CAPTURE_H
# include "../common.hpp"
#include <vector>
//typedef enum {
//    TSC_CAPTURE_NONE        = 0,
//    TSC_CAPTURE_X11         = 1,
//    TSC_CAPTURE_PW          = 2,
//    TSC_CAPTURE_NVFBC       = 3,
//    TSC_CAPTURE_AMF         = 4,
//    TSC_CAPTURE_LGSHM       = 5
//} tsc_capture_type;

struct tsc_display {
    int                 index;
    int                 width;
    int                 height;
    int                 fps;
};

struct tsc_frame_buffer {
    struct tsc_display  display;
    int                 buffer_id;
    int                 pixmap;
    void                *buffer;
    size_t              buffer_length;
    void                *context;
};

class tsc_capture{
    /*  Constructor.
    Initialise the capture system. */
    virtual void init() = 0;

/*  Get the screen capture type in use by this class. */
    virtual int get_capture_type() = 0;

/*  Get a list of displays that can be captured. */
    virtual struct std::vector<tsc_display> get_displays() = 0;

/*  Free the list of displays returned by get_displays() */
    virtual void free_displays(struct tsc_display *) = 0;

/*  Allocate memory to store one frame for a specific display */
    virtual struct tsc_frame_buffer *alloc_frame(int index) = 0;

/*  Update the memory for a specific framebuffer */
    virtual void update_frame(struct tsc_frame_buffer *buffer) = 0;

/*  Free the memory used by the frame buffer. */
    virtual void free_frame(struct tsc_frame_buffer *frame_buffer) = 0;
};

#endif //TELESCOPE_CAPTURE_H
