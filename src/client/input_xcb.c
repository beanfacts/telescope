/*
    SPDX-License-Identifier: MIT
    Telescope X Input Interfaces
    Copyright (C) 2021 Telescope Project
    Based on documentation from https://xcb.freedesktop.org/tutorial/
*/

#include <stdlib.h>
#include "input_xcb.h"

static void testCookie (xcb_void_cookie_t cookie, xcb_connection_t *connection, char *errMessage)
{
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
        fprintf (stderr, "ERROR: %s : %d\n", errMessage , error->error_code);
        xcb_disconnect (connection);
        exit (-1);
    }
}

void get_center(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny)
{
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry_unchecked(c, window);
    if ((reply = xcb_get_geometry_reply(c, cookie, NULL))) {
        *ret_cenx = (reply->width)/2;
        *ret_ceny = (reply->height)/2;   
    }
    free(reply);
}

void get_param(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny, int *ret_left, int *ret_right, int *ret_upper, int *ret_bottom) {
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry(c, window);
    
    if ((reply = xcb_get_geometry_reply(c, cookie, NULL)))
    {
		*ret_cenx =     (reply->width) / 2;
		*ret_ceny =     (reply->height) / 2;
        *ret_left =     100;
        *ret_right =    (reply->width) - 100;
        *ret_upper =    50;
        *ret_bottom =   (reply->height) - 50;        
    }
    free(reply);
}