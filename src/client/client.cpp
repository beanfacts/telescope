/*
    SPDX-License-Identifier: AGPL-3.0-or-later
    Telescope - Linux Client
    Copyright (c) 2021 Telescope Project
*/

#include "render/render.hpp"
#include <unistd.h>
#include <random>

#define _test_width 1280
#define _test_height 720

int main(int argc, char **argv)
{
    tsc_render_context render_ctx;
    int ret = render_ctx.init_x11_context(_test_width, _test_height);
    if (ret)
    {
        fprintf(stderr, "Init failed...\n");
        return 1;
    }

    /* Generate random noise */
    char *rand_px = (char *) malloc(_test_width * _test_height * 4);
    for (int i = 0; i < _test_width * _test_height; i++)
    {
        rand_px[i] = 0xFF;
    }

    for (int i = 0; i < 50; i++)
    {
        printf("%hhx", rand_px[i]);
    }
    printf("\n");

    printf("Generated random pixel map.\n");
    render_ctx.x11_render_image(rand_px, 1280, 720, 32);

    printf("Hello\n");
    sleep(100);
    return 0;
}