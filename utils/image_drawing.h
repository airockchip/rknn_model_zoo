#ifndef _RKNN_MODEL_ZOO_IMAGE_DRAWING_H_
#define _RKNN_MODEL_ZOO_IMAGE_DRAWING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

// Color Format ARGB8888
#define COLOR_GREEN     0xFF00FF00
#define COLOR_BLUE      0xFF0000FF
#define COLOR_RED       0xFFFF0000
#define COLOR_YELLOW    0xFFFFFF00
#define COLOR_ORANGE    0xFFFF4500
#define COLOR_BLACK     0xFF000000
#define COLOR_WHITE     0xFFFFFFFF

/**
 * @brief Draw rectangle
 * 
 * @param image [in] Image buffer
 * @param rx [in] Rectangle top left x
 * @param ry [in] Rectangle top left y
 * @param rw [in] Rectangle width
 * @param rh [in] Rectangle height
 * @param color [in] Rectangle line color
 * @param thickness [in] Rectangle line thickness
 */
void draw_rectangle(image_buffer_t* image, int rx, int ry, int rw, int rh, unsigned int color,
                      int thickness);

/**
 * @brief Draw line
 * 
 * @param image [in] Image buffer
 * @param x0 [in] Line begin point x
 * @param y0 [in] Line begin point y
 * @param x1 [in] Line end point x
 * @param y1 [in] Line end point y
 * @param color [in] Line color
 * @param thickness [in] Line thickness
 */
void draw_line(image_buffer_t* image, int x0, int y0, int x1, int y1, unsigned int color,
                 int thickness);

/**
 * @brief Draw text (only support ASCII char)
 * 
 * @param image [in] Image buffer
 * @param text [in] Text
 * @param x [in] Text position x
 * @param y [in] Text position y
 * @param color [in] Text color
 * @param fontsize [in] Text fontsize
 */
void draw_text(image_buffer_t* image, const char* text, int x, int y, unsigned int color,
                 int fontsize);

/**
 * @brief Draw circle
 * 
 * @param image [in] Image buffer
 * @param cx [in] Circle center x
 * @param cy [in] Circle center y
 * @param radius [in] Circle radius
 * @param color [in] Circle color
 * @param thickness [in] Circle thickness
 */
void draw_circle(image_buffer_t* image, int cx, int cy, int radius, unsigned int color,
                 int thickness);

/**
 * @brief Draw image
 * 
 * @param image [in] Target Image buffer
 * @param draw_img [in] Image for drawing
 * @param x [in] Target Image draw position x
 * @param y [in] Target Image draw position y
 * @param rw [in] Width of image for drawing
 * @param rh [in] Height of image for drawing
 */
void draw_image(image_buffer_t* image, unsigned char* draw_img, int x, int y, int rw, int rh);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _RKNN_MODEL_ZOO_IMAGE_DRAWING_H_