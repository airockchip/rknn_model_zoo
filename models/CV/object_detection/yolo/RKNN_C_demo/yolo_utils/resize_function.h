#ifndef _MZ_RESIZE_FUNCTION
#define _MZ_RESIZE_FUNCTION

#include <stdint.h>

#ifndef _MZ_LETTER_BOX
#define _MZ_LETTER_BOX
typedef struct _LETTER_BOX{
    int in_width=0, in_height=0;
    int target_width=0, target_height=0;
    int channel=3;

    float img_wh_ratio=1, target_wh_ratio=1;
    float resize_scale_w=0, resize_scale_h=0;
    int resize_width=0, resize_height=0;
    int h_pad_top=0, h_pad_bottom=0;
    int w_pad_left=0, w_pad_right=0;

    bool reverse_available=false;
} LETTER_BOX;
#endif

int compute_letter_box(LETTER_BOX* lb);

void stb_letter_box_resize(unsigned char *input_buf, unsigned char *output_buf, LETTER_BOX lb);

int rga_letter_box_resize(int src_fd, int dst_fd, LETTER_BOX* lb);
int rga_letter_box_resize(void *src_buf, void *dst_buf, LETTER_BOX* lb);
int rga_letter_box_resize(void *src_buf, int dst_fd, LETTER_BOX* lb);

int h_reverse(int h, LETTER_BOX lb);

int w_reverse(int w, LETTER_BOX lb);

int print_letter_box_info(LETTER_BOX lb);

/*
void rga_resize(uint8_t *input_buf, int input_buf_type,
                uint8_t *output_buf, int output_buf_type, LETTER_BOX lb)
*/


#endif