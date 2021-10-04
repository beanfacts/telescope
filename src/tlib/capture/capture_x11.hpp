//
// Created by maomao on 03/10/2021.
//


#include "capture_base.hpp"
#ifndef TELESCOPE_CAPTURE_X11_H
#define TELESCOPE_CAPTURE_X11_H
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

struct tsc_offset{
    int x_offset;
    int y_offset;
};

class tsc_capture_x11 : public tsc_capture {
    private:
        struct shmimage
        {
            XShmSegmentInfo shminfo ;
            XImage * ximage ;
            unsigned int * data ; // will point to the image's BGRA packed pixels
        } ;
        // class storage variables
        Display *display;
        
        int x_offset{}, y_offset{};
        Window root;
        std::vector<tsc_display> display_info_vec;
        std::vector<tsc_frame_buffer> frame_buffer_vec;
        std::vector<tsc_offset> offset_vec;
        // private class methods 
        static void initimage( struct shmimage * image );
        static void destroyimage( Display * dsp, struct shmimage * image );
        static int init_xshm(Display * dsp, struct shmimage * image , int width, int height);

        shmimage *init_x11(Display *dsp, int dsp_width, int dsp_height,int x_offset,int y_offset);
        const void *get_frame(shmimage *src);

    public:
            void init();

            int get_capture_type() {
                return TSC_CAPTURE_X11;
            }

            struct std::vector<tsc_display> get_displays();

            struct tsc_frame_buffer *alloc_frame(int index);

            void update_frame(struct tsc_frame_buffer *frame_buffer);
            void free_displays(struct tsc_display *display);
            void free_frame(struct tsc_frame_buffer *frame_buffer);
};
#endif //TELESCOPE_CAPTURE_X11_H
