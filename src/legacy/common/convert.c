#include "convert.h"

// Perform a memory copy from the raw 32-bit buffer into a smaller 24-bit buffer
// for network transmission. For testing only
int bgra32_to_bgr24(uint8_t *restrict bgra32, uint8_t *restrict bgr24, int size)
{
    uint8_t *end = bgra32 + size;
    while (bgra32 < end)
    {
        *(bgr24++) = *(bgra32++);
        *(bgr24++) = *(bgra32++);
        *(bgr24++) = *(bgra32++);
        bgra32++;
    }
    return 0;
}

// Perform a memory copy from a 24-bit buffer into a 32-bit buffer for use by
// the windowing system. For testing only
int bgr24_to_bgra32(uint8_t *restrict bgr24, uint8_t *restrict bgra32, int size)
{
    uint8_t *end = bgra32 + size;
    while (bgra32 < end)
    {
        *(bgra32++) = *(bgr24++);
        *(bgra32++) = *(bgr24++);
        *(bgra32++) = *(bgr24++);
        *(bgra32++) = 0xFF;
    }
    return 0;
}