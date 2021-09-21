/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope Desktop Capture Library
    Copyright (c) 2021 Telescope Project
*/

#include <cstring>
#include <cstdio>
#include <stdexcept>
#include "capture.hpp"
#include <vector>

#include <sys/shm.h>

void tsc_capture_context::init(tsc_screen *scr)
{
    // Copy screen info into internal context
    memcpy(&screen, scr, sizeof(tsc_screen));
    
    // Initialise capture context and allocate memory to store frames
    init_context();
}

void tsc_capture_context::start_capture(uint32_t width, uint32_t height, uint32_t bpp, uint32_t fps, uint32_t buffers)
{
    uint64_t imbytes = width * height * (bpp / 8);

    if (screen.width != width || screen.height != height)
    {
        // resolution change to be implemented later
        throw std::runtime_error("Requested resolution does not match screen.");
    }

    if (screen.capture_type == TSC_CAPTURE_X11)
    {
        for (uint32_t i = 0; i < buffers; i++)
        {
            int ret;
            XShmSegmentInfo this_shm;
            
            this_shm.shmid = shmget(IPC_PRIVATE, imbytes, IPC_CREAT | 0777);
            if (!this_shm.shmid)
                throw std::runtime_error("Could not get SHM region.");
            
            this_shm.shmaddr = (char *) shmat(this_shm.shmid, 0, 0);
            if (!this_shm.shmaddr)
                throw std::runtime_error("SHM region alloc failed.");
            
            ret = shmctl(this_shm.shmid, IPC_RMID, 0);
            if (ret < 0)
                throw std::runtime_error(
                        std::string("SHM configuration failed: ") 
                        + std::string(strerror(errno)));
            
            ret = XShmAttach(screen.x_display, &this_shm);
            if (!ret)
                throw std::runtime_error("SHM attach to X server failed.");

            XImage *img = XShmCreateImage(screen.x_display, 
                    DefaultVisual(screen.x_display, screen.x_screen_no), 
                    bpp, ZPixmap, NULL, &this_shm, width, height);
            if (!img)
                throw std::runtime_error("SHM pixmap creation failed.");

            tsc_shminfo     _info;
            _info.type      = TSC_CAPTURE_X11;
            _info.x_image   = img;
            _info.shmid     = this_shm.shmid;
            _info.addr      = this_shm.shmaddr;
            _info.length    = imbytes;
            shminfo.insert(shminfo.end(), _info);
        }
    }
}

void tsc_capture_context::update_frame(int index)
{
    int ret;
    
    if (screen.capture_type == TSC_CAPTURE_X11)
    {
        ret = XShmGetImage(screen.x_display, screen.x_window, 
                shminfo[index].x_image, screen.x_offset_x, screen.x_offset_y,
                AllPlanes );
        if (!ret)
            throw std::runtime_error("Capture failed.");
    }
}

int tsc_capture_context::get_buf_count()
{
    return shminfo.size();
}
    
std::vector<tsc_shminfo> tsc_capture_context::get_shminfo()
{
    return shminfo;
}

void tsc_capture_context::init_context()
{

    int ret;
    
    if (screen.capture_type == TSC_CAPTURE_X11)
    {
        // Prepare X server connection for multithreading
        ret = XInitThreads();
        if (!ret){
            throw std::runtime_error(std::string("X11 threading error: ") + std::to_string(ret));
        }

        // Grab the root display
        // On some X servers and Xwayland, this will return a black screen.
        screen.x_display = XOpenDisplay(NULL);
        if(!screen.x_display)
        {
            throw std::runtime_error("Could not open a connection to the X server.");
        }

        screen.x_window = XDefaultRootWindow(screen.x_display);
        screen.x_screen_no = 0;

        // Check the X server supports shared memory extensions
        if(!XShmQueryExtension(screen.x_display))
        {
            XCloseDisplay(screen.x_display);
            throw std::runtime_error("Your X server does not support shared memory extensions.");
        }

        screen.width  = XDisplayWidth(screen.x_display, screen.x_screen_no);
        screen.height = XDisplayHeight(screen.x_display, screen.x_screen_no);

        XRRScreenConfiguration *xrr = XRRGetScreenInfo(screen.x_display, screen.x_window);
        screen.fps = (uint32_t) XRRConfigCurrentRate(xrr);

        double bw_frame = (double) screen.width * (double) screen.height * (double) screen.fps * (double) 32.0;

        printf( "Capturing X Screen %d (%d x %d @ %d Hz)\n"
                "Estimated bandwidth: %.3f Gbps\n",
                screen.x_screen_no, screen.width, screen.height, screen.fps,
                (bw_frame / (double) (1 << 30)));

        return;
    }

    throw std::runtime_error("Supplied capture function invalid or unimplemented.");

}

void *tsc_capture_context::get_frame_addr(int index, int *size)
{
    *size = shminfo[index].length;
    return shminfo[index].addr;
}