//
// Created by maomao on 03/10/2021.
//

#include "capture_x11.hpp"

#include <iostream>
//x11
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

//xrandr
#include <X11/extensions/Xrandr.h>

#define BPP    4

void tsc_capture_x11::init()
{
    // Initialise X11
    XInitThreads();
    // define the global display variable
    display = XOpenDisplay(nullptr);
    root = XDefaultRootWindow( display );
    if(display == nullptr) {
        std::cout << "Error: Failed to open display\n";
        exit(0); }
}

struct std::vector<tsc_display> tsc_capture_x11::get_displays()
{
    // n_screens is the screen counter
    int n_screen = 0;
    XRRScreenResources *screen;
    XRRCrtcInfo *crtc_info;
    screen = XRRGetScreenResources (display, root);
    while (true){
        // get the offset width height and other stuff
        tsc_display display_info;
        crtc_info = XRRGetCrtcInfo (display, screen, screen->crtcs[n_screen]);
        display_info = {
        .index = n_screen,
        .width = (int) crtc_info->width,
        .height = (int) crtc_info->height,
        .fps = 60
        };
        tsc_offset offset;
        offset.x_offset = crtc_info->x;
        offset.y_offset = crtc_info->y;
        offset_vec.push_back(offset);
        display_info_vec.push_back(display_info);
        //if it is a valid display
        // the crtc_ifo->width will be > 0
        if (crtc_info->width == 0){
            break;
        }
        n_screen++;
    }
    std::cout << "You Have " << n_screen << " screen(s) available\n";
    std::cout << "select using 0 to " << n_screen - 1 << "\n";
    return display_info_vec;
    
}

struct tsc_frame_buffer *tsc_capture_x11::alloc_frame(int index)
{   tsc_frame_buffer *buf = new tsc_frame_buffer;
    shmimage *temp;
    temp = tsc_capture_x11::init_x11(display,display_info_vec[index].width,display_info_vec[index].height);
    buf->display = display_info_vec[index];
    // figure out how to deal with this later
    buf->buffer_id = 0;
    buf->pixmap = 0;
    buf->buffer = temp->shminfo.shmaddr;
    buf->buffer_length=0;
    // needed to update the ximage
    buf->context = temp;
    return buf;
    
}

void tsc_capture_x11::update_frame(struct tsc_frame_buffer *frame_buffer){
    int display_index = frame_buffer->display.index;
    get_frame((shmimage *) frame_buffer->context,offset_vec[display_index].x_offset,offset_vec[display_index].y_offset);
}

void tsc_capture_x11::initimage( struct shmimage * image )
{
    image->ximage = nullptr ;
    image->shminfo.shmaddr = (char *) -1 ;
}

void tsc_capture_x11::destroyimage( Display * dsp, struct shmimage * image )
{
    if( image->ximage )
    {
        XShmDetach( dsp, &image->shminfo ) ;
        XDestroyImage( image->ximage ) ;
        image->ximage = nullptr ;
    }

    if( image->shminfo.shmaddr != ( char * ) -1 )
    {
        shmdt( image->shminfo.shmaddr ) ;
        image->shminfo.shmaddr = ( char * ) -1 ;
    }
}

int tsc_capture_x11::init_xshm(Display * dsp, struct shmimage * image , int width, int height){
    // Create a shared memory area
    image->shminfo.shmid = shmget( IPC_PRIVATE, width * height * BPP, IPC_CREAT | 0600 ) ;
    if( image->shminfo.shmid == -1 )
    {
        return false ;
    }

    // Map the shared memory segment into the address space of this process
    image->shminfo.shmaddr = (char *) shmat( image->shminfo.shmid, nullptr, 0 ) ;
    if( image->shminfo.shmaddr == (char *) -1 )
    {
        return false ;
    }

    image->data = (unsigned int*) image->shminfo.shmaddr ;
    image->shminfo.readOnly = false ;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl( image->shminfo.shmid, IPC_RMID, nullptr ) ;

    // Allocate the memory needed for the XImage structure
    image->ximage = XShmCreateImage( dsp, XDefaultVisual( dsp, XDefaultScreen( dsp ) ),
                                     DefaultDepth( dsp, XDefaultScreen( dsp ) ), ZPixmap, 0,
                                     &image->shminfo, 0, 0 ) ;
    if( !image->ximage )
    {
        destroyimage( dsp, image ) ;
        printf( "Error: could not allocate the XImage structure\n" ) ;
        return false ;
    }

    image->ximage->data = (char *)image->data ;
    image->ximage->width = width ;
    image->ximage->height = height ;

    // Ask the X server to attach the shared memory segment and sync
    XShmAttach( dsp, &image->shminfo ) ;
    XSync( dsp, false ) ;
    return true ;

}

tsc_capture_x11::shmimage *tsc_capture_x11::init_x11(Display *dsp, int dsp_width, int dsp_height){
    shmimage *src = new shmimage;

    // initalise the xshm image
    initimage(src);
    // init xshm for to capture
    if( !init_xshm( dsp, src, dsp_width, dsp_height ) )
    {
        std::cout << "Error: Unable to initialise XShm";
        XCloseDisplay( dsp ) ;
    }
    return src;
}

const void *tsc_capture_x11::get_frame(shmimage *src,int x_offset,int y_offset)
    {
        XShmGetImage( display, XDefaultRootWindow( display ), src->ximage, x_offset, y_offset, AllPlanes ) ;
        return reinterpret_cast<void *>(src->ximage->data);
    }

void tsc_capture_x11::free_displays(struct tsc_display *display){
    free(display);
}

void tsc_capture_x11::free_frame(struct tsc_frame_buffer *frame_buffer){
    free(frame_buffer->buffer);
    free(frame_buffer->context);

}