/*
    SPDX-License-Identifier: MIT
    Telescope X Input Interfaces
    Copyright (C) 2021 Telescope Project
    Based on documentation from https://xcb.freedesktop.org/tutorial/
*/

#include <stdio.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>

static void testCookie(xcb_void_cookie_t, xcb_connection_t*, char *);
void get_center(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny);
void get_param(xcb_connection_t *c, xcb_window_t window, int *ret_cenx, int *ret_ceny, int *ret_left, int *ret_right, int *ret_upper, int *ret_bottom);