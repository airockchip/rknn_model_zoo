#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "image_drawing.h"
#include "font.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

// src color format(ARGB888) To dest format color
static unsigned int convert_color(unsigned int src_color, image_format_t dst_fmt)
{
    // printf("sizeof(int)=%d\n", sizeof(int));
    unsigned int dst_color = 0x0;
    unsigned char* p_src_color = (unsigned char*)&src_color;
    unsigned char* p_dst_color = (unsigned char*)&dst_color;
    char r = p_src_color[2];
    char g = p_src_color[1];
    char b = p_src_color[0];
    char a = p_src_color[3];

    switch (dst_fmt)
    {
    case IMAGE_FORMAT_GRAY8:
        p_dst_color[0] = a;
        break;
    case IMAGE_FORMAT_RGB888:
        p_dst_color[0] = r;
        p_dst_color[1] = g;
        p_dst_color[2] = b;
        break;
    case IMAGE_FORMAT_RGBA8888:
        p_dst_color[0] = r;
        p_dst_color[1] = g;
        p_dst_color[2] = b;
        p_dst_color[3] = a;
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
        p_dst_color[0] = 0.299 * r + 0.587 * g + 0.114 * b;
        p_dst_color[1] = 0.492 * (b - p_dst_color[0]);
        p_dst_color[2] = 0.877 * (r - p_dst_color[0]);
        break;
    case IMAGE_FORMAT_YUV420SP_NV21:
        p_dst_color[0] = 0.299 * r + 0.587 * g + 0.114 * b;
        p_dst_color[1] = 0.877 * (r - p_dst_color[0]);
        p_dst_color[2] = 0.492 * (b - p_dst_color[0]);
        break;
    default:
        break;
    }
    return dst_color;
}

static void draw_rectangle_c1(unsigned char* pixels, int w, int h, int rx, int ry, int rw, int rh, unsigned int color,
                              int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w;

    if (thickness == -1) {
        // filled
        for (int y = ry; y < ry + rh; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx; x < rx + rw; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x] = pen_color[0];
            }
        }

        return;
    }

    const int t0 = thickness / 2;
    const int t1 = thickness - t0;

    // draw top
    {
        for (int y = ry - t0; y < ry + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x] = pen_color[0];
            }
        }
    }

    // draw bottom
    {
        for (int y = ry + rh - t0; y < ry + rh + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x] = pen_color[0];
            }
        }
    }

    // draw left
    for (int x = rx - t0; x < rx + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x] = pen_color[0];
        }
    }

    // draw right
    for (int x = rx + rw - t0; x < rx + rw + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x] = pen_color[0];
        }
    }
}

static void draw_rectangle_c2(unsigned char* pixels, int w, int h, int rx, int ry, int rw, int rh, unsigned int color,
                              int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 2;

    if (thickness == -1) {
        // filled
        for (int y = ry; y < ry + rh; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx; x < rx + rw; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 2 + 0] = pen_color[0];
                p[x * 2 + 1] = pen_color[1];
            }
        }

        return;
    }

    const int t0 = thickness / 2;
    const int t1 = thickness - t0;

    // draw top
    {
        for (int y = ry - t0; y < ry + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 2 + 0] = pen_color[0];
                p[x * 2 + 1] = pen_color[1];
            }
        }
    }

    // draw bottom
    {
        for (int y = ry + rh - t0; y < ry + rh + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 2 + 0] = pen_color[0];
                p[x * 2 + 1] = pen_color[1];
            }
        }
    }

    // draw left
    for (int x = rx - t0; x < rx + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 2 + 0] = pen_color[0];
            p[x * 2 + 1] = pen_color[1];
        }
    }

    // draw right
    for (int x = rx + rw - t0; x < rx + rw + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 2 + 0] = pen_color[0];
            p[x * 2 + 1] = pen_color[1];
        }
    }
}

static void draw_rectangle_c3(unsigned char* pixels, int w, int h, int rx, int ry, int rw, int rh, unsigned int color,
                              int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 3;

    if (thickness == -1) {
        // filled
        for (int y = ry; y < ry + rh; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx; x < rx + rw; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 3 + 0] = pen_color[0];
                p[x * 3 + 1] = pen_color[1];
                p[x * 3 + 2] = pen_color[2];
            }
        }

        return;
    }

    const int t0 = thickness / 2;
    const int t1 = thickness - t0;

    // draw top
    {
        for (int y = ry - t0; y < ry + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 3 + 0] = pen_color[0];
                p[x * 3 + 1] = pen_color[1];
                p[x * 3 + 2] = pen_color[2];
            }
        }
    }

    // draw bottom
    {
        for (int y = ry + rh - t0; y < ry + rh + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 3 + 0] = pen_color[0];
                p[x * 3 + 1] = pen_color[1];
                p[x * 3 + 2] = pen_color[2];
            }
        }
    }

    // draw left
    for (int x = rx - t0; x < rx + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 3 + 0] = pen_color[0];
            p[x * 3 + 1] = pen_color[1];
            p[x * 3 + 2] = pen_color[2];
        }
    }

    // draw right
    for (int x = rx + rw - t0; x < rx + rw + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 3 + 0] = pen_color[0];
            p[x * 3 + 1] = pen_color[1];
            p[x * 3 + 2] = pen_color[2];
        }
    }
}

static void draw_rectangle_c4(unsigned char* pixels, int w, int h, int rx, int ry, int rw, int rh, unsigned int color,
                              int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 4;

    if (thickness == -1) {
        // filled
        for (int y = ry; y < ry + rh; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx; x < rx + rw; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 4 + 0] = pen_color[0];
                p[x * 4 + 1] = pen_color[1];
                p[x * 4 + 2] = pen_color[2];
                p[x * 4 + 3] = pen_color[3];
            }
        }

        return;
    }

    const int t0 = thickness / 2;
    const int t1 = thickness - t0;

    // draw top
    {
        for (int y = ry - t0; y < ry + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 4 + 0] = pen_color[0];
                p[x * 4 + 1] = pen_color[1];
                p[x * 4 + 2] = pen_color[2];
                p[x * 4 + 3] = pen_color[3];
            }
        }
    }

    // draw bottom
    {
        for (int y = ry + rh - t0; y < ry + rh + t1; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = rx - t0; x < rx + rw + t1; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                p[x * 4 + 0] = pen_color[0];
                p[x * 4 + 1] = pen_color[1];
                p[x * 4 + 2] = pen_color[2];
                p[x * 4 + 3] = pen_color[3];
            }
        }
    }

    // draw left
    for (int x = rx - t0; x < rx + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 4 + 0] = pen_color[0];
            p[x * 4 + 1] = pen_color[1];
            p[x * 4 + 2] = pen_color[2];
            p[x * 4 + 3] = pen_color[3];
        }
    }

    // draw right
    for (int x = rx + rw - t0; x < rx + rw + t1; x++) {
        if (x < 0)
            continue;

        if (x >= w)
            break;

        for (int y = ry + t1; y < ry + rh - t0; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            p[x * 4 + 0] = pen_color[0];
            p[x * 4 + 1] = pen_color[1];
            p[x * 4 + 2] = pen_color[2];
            p[x * 4 + 3] = pen_color[3];
        }
    }
}

static void draw_rectangle_yuv420sp(unsigned char* yuv420sp, int w, int h, int rx, int ry, int rw, int rh,
                                    unsigned int color, int thickness)
{
    // assert w % 2 == 0
    // assert h % 2 == 0
    // assert rx % 2 == 0
    // assert ry % 2 == 0
    // assert rw % 2 == 0
    // assert rh % 2 == 0
    // assert thickness % 2 == 0

    const unsigned char* pen_color = (const unsigned char*)&color;

    unsigned int v_y;
    unsigned int v_uv;
    unsigned char* pen_color_y = (unsigned char*)&v_y;
    unsigned char* pen_color_uv = (unsigned char*)&v_uv;
    pen_color_y[0] = pen_color[0];
    pen_color_uv[0] = pen_color[1];
    pen_color_uv[1] = pen_color[2];

    unsigned char* Y = yuv420sp;
    draw_rectangle_c1(Y, w, h, rx, ry, rw, rh, v_y, thickness);

    unsigned char* UV = yuv420sp + w * h;
    int thickness_uv = thickness == -1 ? thickness : max(thickness / 2, 1);
    draw_rectangle_c2(UV, w / 2, h / 2, rx / 2, ry / 2, rw / 2, rh / 2, v_uv, thickness_uv);
}

static inline int distance_lessequal(int x0, int y0, int x1, int y1, float r)
{
    int dx = x0 - x1;
    int dy = y0 - y1;
    int q = dx * dx + dy * dy;
    return q <= r * r;
}

static inline int distance_inrange(int x0, int y0, int x1, int y1, float r0, float r1)
{
    int dx = x0 - x1;
    int dy = y0 - y1;
    int q = dx * dx + dy * dy;
    return q >= r0 * r0 && q < r1 * r1;
}

static void draw_circle_c1(unsigned char* pixels, int w, int h, int cx, int cy, int radius, unsigned int color,
                           int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w;

    if (thickness == -1) {
        // filled
        for (int y = cy - (radius - 1); y < cy + radius; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = cx - (radius - 1); x < cx + radius; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                // distance from cx cy
                if (distance_lessequal(x, y, cx, cy, radius)) {
                    p[x] = pen_color[0];
                }
            }
        }

        return;
    }

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    for (int y = cy - (radius - 1) - t0; y < cy + radius + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = cx - (radius - 1) - t0; x < cx + radius + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from cx cy
            if (distance_inrange(x, y, cx, cy, radius - t0, radius + t1)) {
                p[x] = pen_color[0];
            }
        }
    }
}

static void draw_circle_c2(unsigned char* pixels, int w, int h, int cx, int cy, int radius, unsigned int color,
                           int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 2;

    if (thickness == -1) {
        // filled
        for (int y = cy - (radius - 1); y < cy + radius; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = cx - (radius - 1); x < cx + radius; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                // distance from cx cy
                if (distance_lessequal(x, y, cx, cy, radius)) {
                    p[x * 2 + 0] = pen_color[0];
                    p[x * 2 + 1] = pen_color[1];
                }
            }
        }

        return;
    }

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    for (int y = cy - radius - t0; y < cy + radius + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = cx - radius - t0; x < cx + radius + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from cx cy
            if (distance_inrange(x, y, cx, cy, radius - t0, radius + t1)) {
                p[x * 2 + 0] = pen_color[0];
                p[x * 2 + 1] = pen_color[1];
            }
        }
    }
}

static void draw_circle_c3(unsigned char* pixels, int w, int h, int cx, int cy, int radius, unsigned int color,
                           int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 3;

    if (thickness == -1) {
        // filled
        for (int y = cy - (radius - 1); y < cy + radius; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = cx - (radius - 1); x < cx + radius; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                // distance from cx cy
                if (distance_lessequal(x, y, cx, cy, radius)) {
                    p[x * 3 + 0] = pen_color[0];
                    p[x * 3 + 1] = pen_color[1];
                    p[x * 3 + 2] = pen_color[2];
                }
            }
        }

        return;
    }

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    for (int y = cy - radius - t0; y < cy + radius + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = cx - radius - t0; x < cx + radius + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from cx cy
            if (distance_inrange(x, y, cx, cy, radius - t0, radius + t1)) {
                p[x * 3 + 0] = pen_color[0];
                p[x * 3 + 1] = pen_color[1];
                p[x * 3 + 2] = pen_color[2];
            }
        }
    }
}

static void draw_circle_c4(unsigned char* pixels, int w, int h, int cx, int cy, int radius, unsigned int color,
                           int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 4;

    if (thickness == -1) {
        // filled
        for (int y = cy - (radius - 1); y < cy + radius; y++) {
            if (y < 0)
                continue;

            if (y >= h)
                break;

            unsigned char* p = pixels + stride * y;

            for (int x = cx - (radius - 1); x < cx + radius; x++) {
                if (x < 0)
                    continue;

                if (x >= w)
                    break;

                // distance from cx cy
                if (distance_lessequal(x, y, cx, cy, radius)) {
                    p[x * 4 + 0] = pen_color[0];
                    p[x * 4 + 1] = pen_color[1];
                    p[x * 4 + 2] = pen_color[2];
                    p[x * 4 + 3] = pen_color[3];
                }
            }
        }

        return;
    }

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    for (int y = cy - (radius - 1) - t0; y < cy + radius + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = cx - (radius - 1) - t0; x < cx + radius + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from cx cy
            if (distance_inrange(x, y, cx, cy, radius - t0, radius + t1)) {
                p[x * 4 + 0] = pen_color[0];
                p[x * 4 + 1] = pen_color[1];
                p[x * 4 + 2] = pen_color[2];
                p[x * 4 + 3] = pen_color[3];
            }
        }
    }
}

static void draw_circle_yuv420sp(unsigned char* yuv420sp, int w, int h, int cx, int cy, int radius, unsigned int color,
                                 int thickness)
{
    // assert w % 2 == 0
    // assert h % 2 == 0
    // assert cx % 2 == 0
    // assert cy % 2 == 0
    // assert radius % 2 == 0
    // assert thickness % 2 == 0

    const unsigned char* pen_color = (const unsigned char*)&color;

    unsigned int v_y;
    unsigned int v_uv;
    unsigned char* pen_color_y = (unsigned char*)&v_y;
    unsigned char* pen_color_uv = (unsigned char*)&v_uv;
    pen_color_y[0] = pen_color[0];
    pen_color_uv[0] = pen_color[1];
    pen_color_uv[1] = pen_color[2];

    unsigned char* Y = yuv420sp;
    draw_circle_c1(Y, w, h, cx, cy, radius, v_y, thickness);

    unsigned char* UV = yuv420sp + w * h;
    int thickness_uv = thickness == -1 ? thickness : max(thickness / 2, 1);
    draw_circle_c2(UV, w / 2, h / 2, cx / 2, cy / 2, radius / 2, v_uv, thickness_uv);
}

static inline int distance_lessthan(int x, int y, int x0, int y0, int x1, int y1, float t)
{
    int dx01 = x1 - x0;
    int dy01 = y1 - y0;
    int dx0 = x - x0;
    int dy0 = y - y0;

    float r = (float)(dx0 * dx01 + dy0 * dy01) / (dx01 * dx01 + dy01 * dy01);

    if (r < 0 || r > 1)
        return 0;

    float px = x0 + dx01 * r;
    float py = y0 + dy01 * r;
    float dx = x - px;
    float dy = y - py;
    float p = dx * dx + dy * dy;
    return p < t;
}

static void draw_line_c1(unsigned char* pixels, int w, int h, int x0, int y0, int x1, int y1, unsigned int color,
                         int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w;

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    int x_min = min(x0, x1);
    int x_max = max(x0, x1);
    int y_min = min(y0, y1);
    int y_max = max(y0, y1);

    for (int y = y_min - t0; y < y_max + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = x_min - t0; x < x_max + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from line
            if (distance_lessthan(x, y, x0, y0, x1, y1, t1)) {
                p[x] = pen_color[0];
            }
        }
    }
}

static void draw_line_c2(unsigned char* pixels, int w, int h, int x0, int y0, int x1, int y1, unsigned int color,
                         int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 2;

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    int x_min = min(x0, x1);
    int x_max = max(x0, x1);
    int y_min = min(y0, y1);
    int y_max = max(y0, y1);

    for (int y = y_min - t0; y < y_max + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = x_min - t0; x < x_max + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from line
            if (distance_lessthan(x, y, x0, y0, x1, y1, t1)) {
                p[x * 2 + 0] = pen_color[0];
                p[x * 2 + 1] = pen_color[1];
            }
        }
    }
}

static void draw_line_c3(unsigned char* pixels, int w, int h, int x0, int y0, int x1, int y1, unsigned int color,
                         int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 3;

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    int x_min = min(x0, x1);
    int x_max = max(x0, x1);
    int y_min = min(y0, y1);
    int y_max = max(y0, y1);

    for (int y = y_min - t0; y < y_max + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = x_min - t0; x < x_max + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from line
            if (distance_lessthan(x, y, x0, y0, x1, y1, t1)) {
                p[x * 3 + 0] = pen_color[0];
                p[x * 3 + 1] = pen_color[1];
                p[x * 3 + 2] = pen_color[2];
            }
        }
    }
}

static void draw_line_c4(unsigned char* pixels, int w, int h, int x0, int y0, int x1, int y1, unsigned int color,
                         int thickness)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 4;

    const float t0 = thickness / 2.f;
    const float t1 = thickness - t0;

    int x_min = min(x0, x1);
    int x_max = max(x0, x1);
    int y_min = min(y0, y1);
    int y_max = max(y0, y1);

    for (int y = y_min - t0; y < y_max + t1; y++) {
        if (y < 0)
            continue;

        if (y >= h)
            break;

        unsigned char* p = pixels + stride * y;

        for (int x = x_min - t0; x < x_max + t1; x++) {
            if (x < 0)
                continue;

            if (x >= w)
                break;

            // distance from line
            if (distance_lessthan(x, y, x0, y0, x1, y1, t1)) {
                p[x * 4 + 0] = pen_color[0];
                p[x * 4 + 1] = pen_color[1];
                p[x * 4 + 2] = pen_color[2];
                p[x * 4 + 3] = pen_color[3];
            }
        }
    }
}

static void draw_line_yuv420sp(unsigned char* yuv420sp, int w, int h, int x0, int y0, int x1, int y1,
                               unsigned int color, int thickness)
{
    // assert w % 2 == 0
    // assert h % 2 == 0
    // assert x0 % 2 == 0
    // assert y0 % 2 == 0
    // assert x1 % 2 == 0
    // assert y1 % 2 == 0
    // assert thickness % 2 == 0

    const unsigned char* pen_color = (const unsigned char*)&color;

    unsigned int v_y;
    unsigned int v_uv;
    unsigned char* pen_color_y = (unsigned char*)&v_y;
    unsigned char* pen_color_uv = (unsigned char*)&v_uv;
    pen_color_y[0] = pen_color[0];
    pen_color_uv[0] = pen_color[1];
    pen_color_uv[1] = pen_color[2];

    unsigned char* Y = yuv420sp;
    draw_line_c1(Y, w, h, x0, y0, x1, y1, v_y, thickness);

    unsigned char* UV = yuv420sp + w * h;
    int thickness_uv = thickness == -1 ? thickness : max(thickness / 2, 1);
    draw_line_c2(UV, w / 2, h / 2, x0 / 2, y0 / 2, x1 / 2, y1 / 2, v_uv, thickness_uv);
}

static void get_text_drawing_size(const char* text, int fontpixelsize, int* w, int* h)
{
    *w = 0;
    *h = 0;

    const int n = strlen(text);

    int line_w = 0;
    for (int i = 0; i < n; i++) {
        char ch = text[i];

        if (ch == '\n') {
            // newline
            *w = max(*w, line_w);
            *h += fontpixelsize * 2;
            line_w = 0;
        }

        if (isprint(ch) != 0) {
            line_w += fontpixelsize;
        }
    }

    *w = max(*w, line_w);
    *h += fontpixelsize * 2;
}

static int resize_bilinear_c1(const unsigned char* src_pixels, int w, int h, unsigned char* dst_pixels, int w2, int h2)
{
    int A, B, C, D, x, y, index, gray;
    float x_ratio = ((float)(w - 1)) / w2;
    float y_ratio = ((float)(h - 1)) / h2;
    float x_diff, y_diff, ya, yb;
    int offset = 0;
    for (int i = 0; i < h2; i++) {
        for (int j = 0; j < w2; j++) {
            x = (int)(x_ratio * j);
            y = (int)(y_ratio * i);
            x_diff = (x_ratio * j) - x;
            y_diff = (y_ratio * i) - y;
            index = y * w + x;

            // range is 0 to 255 thus bitwise AND with 0xff
            A = src_pixels[index] & 0xff;
            B = src_pixels[index + 1] & 0xff;
            C = src_pixels[index + w] & 0xff;
            D = src_pixels[index + w + 1] & 0xff;

            // Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
            gray = (int)(A * (1 - x_diff) * (1 - y_diff) + B * (x_diff) * (1 - y_diff) + C * (y_diff) * (1 - x_diff) +
                         D * (x_diff * y_diff));

            dst_pixels[offset++] = gray;
        }
    }
    return 0;
}

static void draw_text_c1(unsigned char* pixels, int w, int h, const char* text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w;

    unsigned char* resized_font_bitmap = malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++) {
        char ch = text[i];

        if (ch == '\n') {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0) {
            const unsigned char* font_bitmap = mono_font_data[ch - ' '];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++) {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char* palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char* p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++) {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k] = (p[k] * (255 - alpha) + pen_color[0] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_c2(unsigned char* pixels, int w, int h, const char* text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 2;

    unsigned char* resized_font_bitmap = malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++) {
        char ch = text[i];

        if (ch == '\n') {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0) {
            int font_bitmap_index = ch - ' ';
            const unsigned char* font_bitmap = mono_font_data[font_bitmap_index];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++) {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char* palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char* p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++) {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k * 2 + 0] = (p[k * 2 + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                    p[k * 2 + 1] = (p[k * 2 + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_c3(unsigned char* pixels, int w, int h, const char* text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 3;

    unsigned char* resized_font_bitmap = malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++) {
        char ch = text[i];

        if (ch == '\n') {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0) {
            int font_bitmap_index = ch - ' ';
            const unsigned char* font_bitmap = mono_font_data[font_bitmap_index];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++) {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char* palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char* p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++) {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k * 3 + 0] = (p[k * 3 + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                    p[k * 3 + 1] = (p[k * 3 + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
                    p[k * 3 + 2] = (p[k * 3 + 2] * (255 - alpha) + pen_color[2] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_c4(unsigned char* pixels, int w, int h, const char* text, int x, int y, int fontpixelsize,
                         unsigned int color)
{
    const unsigned char* pen_color = (const unsigned char*)&color;
    int stride = w * 4;

    unsigned char* resized_font_bitmap = malloc(fontpixelsize * fontpixelsize * 2);

    const int n = strlen(text);

    int cursor_x = x;
    int cursor_y = y;
    for (int i = 0; i < n; i++) {
        char ch = text[i];

        if (ch == '\n') {
            // newline
            cursor_x = x;
            cursor_y += fontpixelsize * 2;
        }

        if (isprint(ch) != 0) {
            const unsigned char* font_bitmap = mono_font_data[ch - ' '];

            // draw resized character
            resize_bilinear_c1(font_bitmap, 20, 40, resized_font_bitmap, fontpixelsize, fontpixelsize * 2);

            for (int j = cursor_y; j < cursor_y + fontpixelsize * 2; j++) {
                if (j < 0)
                    continue;

                if (j >= h)
                    break;

                const unsigned char* palpha = resized_font_bitmap + (j - cursor_y) * fontpixelsize;
                unsigned char* p = pixels + stride * j;

                for (int k = cursor_x; k < cursor_x + fontpixelsize; k++) {
                    if (k < 0)
                        continue;

                    if (k >= w)
                        break;

                    unsigned char alpha = palpha[k - cursor_x];

                    p[k * 4 + 0] = (p[k * 4 + 0] * (255 - alpha) + pen_color[0] * alpha) / 255;
                    p[k * 4 + 1] = (p[k * 4 + 1] * (255 - alpha) + pen_color[1] * alpha) / 255;
                    p[k * 4 + 2] = (p[k * 4 + 2] * (255 - alpha) + pen_color[2] * alpha) / 255;
                    p[k * 4 + 3] = (p[k * 4 + 3] * (255 - alpha) + pen_color[3] * alpha) / 255;
                }
            }

            cursor_x += fontpixelsize;
        }
    }

    free(resized_font_bitmap);
}

static void draw_text_yuv420sp(unsigned char* yuv420sp, int w, int h, const char* text, int x, int y, int fontpixelsize,
                               unsigned int color)
{
    // assert w % 2 == 0
    // assert h % 2 == 0
    // assert x % 2 == 0
    // assert y % 2 == 0
    // assert fontpixelsize % 2 == 0

    const unsigned char* pen_color = (const unsigned char*)&color;

    unsigned int v_y;
    unsigned int v_uv;
    unsigned char* pen_color_y = (unsigned char*)&v_y;
    unsigned char* pen_color_uv = (unsigned char*)&v_uv;
    pen_color_y[0] = pen_color[0];
    pen_color_uv[0] = pen_color[1];
    pen_color_uv[1] = pen_color[2];

    unsigned char* Y = yuv420sp;
    draw_text_c1(Y, w, h, text, x, y, fontpixelsize, v_y);

    unsigned char* UV = yuv420sp + w * h;
    draw_text_c2(UV, w / 2, h / 2, text, x / 2, y / 2, max(fontpixelsize / 2, 1), v_uv);
}

static void draw_image_c1(unsigned char* pixels, int w, int h, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    for (int i = 0; i < rh; i++) {
        memcpy(pixels + (y + i) * w + x,  draw_img + i * rw,  rw);
    }
}

static void draw_image_c2(unsigned char* pixels, int w, int h, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    for (int i = 0; i < rh; i++) {
        memcpy(pixels + ((y + i) * w + x) * 2,  draw_img + i * rw * 2,  rw * 2);
    }
}

static void draw_image_c3(unsigned char* pixels, int w, int h, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    printf("draw_image_c3 pixels=%p wxh=%dx%d draw_img=%p pos=(%d %d) rwxrh=%dx%d\n", pixels, w, h, draw_img, x, y, rw, rh);
    for (int i = 0; i < rh; i++) {
        memcpy(pixels + ((y + i) * w + x) * 3,  draw_img + i * rw * 3,  rw * 3);
    }
}

static void draw_image_c4(unsigned char* pixels, int w, int h, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    for (int i = 0; i < rh; i++) {
        memcpy(pixels + ((y + i) * w + x) * 4,  draw_img + i * rw * 4,  rw * 4);
    }
}

static void draw_image_yuv420sp(unsigned char* pixels, int w, int h, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    draw_image_c1(pixels, w, h, draw_img, x, y, rw, rh);
    draw_image_c2(pixels, w, h / 2, draw_img + rw * rh, x, y/2, rw, rh/2);
}

void draw_rectangle(image_buffer_t* image, int rx, int ry, int rw, int rh, unsigned int color,
                      int thickness)
{
    image_format_t format = image->format;
    unsigned char* pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;

    unsigned int draw_color = convert_color(color, format);
    // printf("draw_color=%x\n", draw_color);

    // printf("draw_rectangle format=%d rx=%d ry=%d rw=%d rh=%d color=0x%x thickness=%d\n",
    //     format, rx, ry, rw, rh, color, thickness);
    switch (format)
    {
    case IMAGE_FORMAT_RGB888:
        draw_rectangle_c3(pixels, w, h, rx, ry, rw, rh, draw_color, thickness);
        break;
    case IMAGE_FORMAT_RGBA8888:
        draw_rectangle_c4(pixels, w, h, rx, ry, rw, rh, draw_color, thickness);
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        draw_rectangle_yuv420sp(pixels, w, h, rx, ry, rw, rh, draw_color, thickness);
        break;
    default:
        printf("no support format %d", format);
        break;
    }
}

void draw_line(image_buffer_t* image, int x0, int y0, int x1, int y1, unsigned int color,
                 int thickness)
{
    image_format_t format = image->format;
    unsigned char* pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;

    unsigned draw_color = convert_color(color, format);

    switch (format)
    {
    case IMAGE_FORMAT_RGB888:
        draw_line_c3(pixels, w, h, x0, y0, x1, y1, draw_color, thickness);
        break;
    case IMAGE_FORMAT_RGBA8888:
        draw_line_c4(pixels, w, h, x0, y0, x1, y1, draw_color, thickness);
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        draw_line_yuv420sp(pixels, w, h, x0, y0, x1, y1, draw_color, thickness);
        break;
    default:
        printf("no support format %d", format);
        break;
    }
}

void draw_text(image_buffer_t* image, const char* text, int x, int y, unsigned int color,
                 int fontsize)
{
    image_format_t format = image->format;
    unsigned char* pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;
    unsigned draw_color = convert_color(color, format);

    switch (format)
    {
    case IMAGE_FORMAT_RGB888:
        draw_text_c3(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    case IMAGE_FORMAT_RGBA8888:
        draw_text_c4(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        draw_text_yuv420sp(pixels, w, h, text, x, y, fontsize, draw_color);
        break;
    default:
        printf("no support format %d", format);
        break;
    }
}

void draw_circle(image_buffer_t* image, int cx, int cy, int radius, unsigned int color,
                 int thickness)
{
    image_format_t format = image->format;
    unsigned char* pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;
    unsigned draw_color = convert_color(color, format);

    switch (format)
    {
    case IMAGE_FORMAT_RGB888:
        draw_circle_c3(pixels, w, h, cx, cy, radius, draw_color, thickness);
        break;
    case IMAGE_FORMAT_RGBA8888:
        draw_circle_c4(pixels, w, h, cx, cy, radius, draw_color, thickness);
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        draw_circle_yuv420sp(pixels, w, h, cx, cy, radius, draw_color, thickness);
        break;
    default:
        printf("no support format %d", format);
        break;
    }
}

void draw_image(image_buffer_t* image, unsigned char* draw_img, int x, int y, int rw, int rh)
{
    image_format_t format = image->format;
    unsigned char* pixels = image->virt_addr;
    int w = image->width;
    int h = image->height;

    switch (format)
    {
    case IMAGE_FORMAT_RGB888:
        draw_image_c3(pixels, w, h, draw_img, x, y, rw, rh);
        break;
    case IMAGE_FORMAT_RGBA8888:
        draw_image_c4(pixels, w, h, draw_img, x, y, rw, rh);
        break;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        draw_image_yuv420sp(pixels, w, h, draw_img, x, y, rw, rh);
        break;
    default:
        printf("no support format %d", format);
        break;
    }
}
