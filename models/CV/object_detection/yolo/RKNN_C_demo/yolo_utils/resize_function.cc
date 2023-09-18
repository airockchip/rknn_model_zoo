#include <stdint.h>
#include <stdio.h>
// #include "rga_func.h"
#include <resize_function.h>

#define _BASETSD_H

// #ifndef STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb/stb_image.h"
// #endif

#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
#endif

// #ifndef STB_IMAGE_WRITE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb/stb_image_write.h"
// #endif

#ifdef ENABLE_RGA
#include "im2d.h"
#include "RgaUtils.h"
#include "im2d_common.h"
// #include "utils.h"
#include "dma_alloc.h"
#endif

int compute_letter_box(LETTER_BOX* lb){
    lb->img_wh_ratio = (float)lb->in_width/ (float)lb->in_height;
    lb->target_wh_ratio = (float)lb->target_width/ (float)lb->target_height;

    if (lb->img_wh_ratio >= lb->target_wh_ratio){
        //pad height dim
        lb->resize_scale_w = (float)lb->target_width / (float)lb->in_width;
        lb->resize_scale_h = lb->resize_scale_w;

        lb->resize_width = lb->target_width;
        lb->w_pad_left = 0;
        lb->w_pad_right = 0;

        lb->resize_height = (int)((float)lb->in_height * lb->resize_scale_h);
        lb->h_pad_top = (lb->target_height - lb->resize_height) / 2;
        if (((lb->target_height - lb->resize_height) % 2) == 0){
            lb->h_pad_bottom = lb->h_pad_top;
        }
        else{
            lb->h_pad_bottom = lb->h_pad_top + 1;
        }

    }
    else{
        //pad width dim
        lb->resize_scale_h = (float)lb->target_height / (float)lb->in_height;
        lb->resize_scale_w = lb->resize_scale_h;

        lb->resize_width = (int)((float)lb->in_width * lb->resize_scale_w);
        lb->w_pad_left = (lb->target_width - lb->resize_width) / 2;
        if (((lb->target_width - lb->resize_width) % 2) == 0){
            lb->w_pad_right = lb->w_pad_left;
        }
        else{
            lb->w_pad_right = lb->w_pad_left + 1;
        }        

        lb->resize_height = lb->target_height;
        lb->h_pad_top = 0;
        lb->h_pad_bottom = 0;
    }
    return 0;
}


void stb_letter_box_resize(unsigned char *input_buf, unsigned char *output_buf, LETTER_BOX lb){
    if ((lb.w_pad_left == 0) &&
        (lb.w_pad_right == 0) &&
        (lb.h_pad_top == 0) && 
        (lb.h_pad_bottom == 0)){
            printf("Needn't letter box resize. Doing direct resize\n");
            stbir_resize_uint8(input_buf, lb.in_width, lb.in_height, 0, (unsigned char*)output_buf, lb.target_width, lb.target_height, 0, 3);
        }
    else{
        
        unsigned char* temp_buf;
        temp_buf = (unsigned char* )malloc(lb.resize_width* lb.resize_height* 3);
        stbir_resize_uint8(input_buf, lb.in_width, lb.in_height, 0, temp_buf, lb.resize_width, lb.resize_height, 0, 3);
        
        // offset
        printf("SETTING OFFSET\n\n");
        printf("lb.in_height: %d\nlb.in_width: %d\n", lb.in_height, lb.in_width);
        printf("lb.resize_height: %d\nlb.resize_width: %d\n", lb.resize_height, lb.resize_width);        

        int ch = 3;
        int output_offset=0, temp_offset=0;
        for (int i=0; i < lb.resize_height; i++){
            for (int j=0; j<lb.resize_width; j++){
                output_offset = ((i+lb.h_pad_top)*lb.target_width + (j+lb.w_pad_left))*ch;
                temp_offset = (i*lb.resize_width + j)*ch;
                output_buf[output_offset ]    = temp_buf[temp_offset];
                output_buf[output_offset + 1] = temp_buf[temp_offset + 1];
                output_buf[output_offset + 2] = temp_buf[temp_offset + 2];
            }
        }

        free(temp_buf);
    }
    return;
}

#ifdef ENABLE_RGA
int _rga_resize(rga_buffer_handle_t src_handle, rga_buffer_handle_t dst_handle, LETTER_BOX* lb){
    int ret = 0;
    rga_buffer_t src_buf, dst_buf;

    src_buf = wrapbuffer_handle(src_handle, lb->in_width, lb->in_height, RK_FORMAT_BGR_888);
    dst_buf = wrapbuffer_handle(dst_handle, lb->target_width, lb->target_height, RK_FORMAT_BGR_888);

    im_rect dst_rect;
    dst_rect.x = 0 + lb->w_pad_left;
    dst_rect.y = 0 + lb->h_pad_top;
    dst_rect.width = lb->resize_width;
    dst_rect.height = lb->resize_height;

    ret = imcheck(src_buf, dst_buf, {}, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("rga letter box resize check error! \n");
        return -1;
    }

    ret = improcess(src_buf, dst_buf, {}, {}, dst_rect, {}, IM_SYNC);
    if (ret == IM_STATUS_SUCCESS) {
        printf("%s running success!\n", "rga letter box resize");
    } else {
        printf("%s running failed, %s\n", "rga letter box resize", imStrError((IM_STATUS)ret));
        return -1;
    }
    return 0;
}
#endif

int rga_letter_box_resize(void *src_buf, void *dst_buf, LETTER_BOX* lb){
    int ret = 0;
#ifdef ENABLE_RGA
    rga_buffer_handle_t src_handle, dst_handle;

    src_handle = importbuffer_virtualaddr(src_buf, lb->in_width* lb->in_height* lb->channel);
    dst_handle = importbuffer_virtualaddr(dst_buf, lb->target_width* lb->target_height* lb->channel);

    ret = _rga_resize(src_handle, dst_handle, lb);

    if (src_handle > 0){
        releasebuffer_handle(src_handle);}
    if (dst_handle > 0){
        releasebuffer_handle(dst_handle);}
#else
    ret = -1;
#endif

    return ret;
}


int rga_letter_box_resize(void *src_buf, int dst_fd, LETTER_BOX* lb){
    int ret = 0;

#ifdef ENABLE_RGA
    rga_buffer_handle_t src_handle, dst_handle;

#ifdef RV110X_DEMO
    int src_fd;
    char* tmp_buf;
    ret = dma_buf_alloc(RV1106_CMA_HEAP_PATH, lb->in_width* lb->in_height* lb->channel, &src_fd, (void **)&tmp_buf);
    memcpy(tmp_buf, src_buf, lb->in_width* lb->in_height* lb->channel);
    src_handle = importbuffer_fd(src_fd, lb->in_width* lb->in_height* lb->channel);
#else
    src_handle = importbuffer_virtualaddr(src_buf, lb->in_width* lb->in_height* lb->channel);
#endif

    dst_handle = importbuffer_fd(dst_fd, lb->target_width* lb->target_height* lb->channel);

    ret = _rga_resize(src_handle, dst_handle, lb);

    if (src_handle > 0){
        releasebuffer_handle(src_handle);}
    if (dst_handle > 0){
        releasebuffer_handle(dst_handle);}

#ifdef RV110X_DEMO
    dma_buf_free(lb->in_width* lb->in_height* lb->channel, &src_fd, tmp_buf);
#endif
#else
    ret = -1;
#endif
    return ret;
}


int rga_letter_box_resize(int src_fd, int dst_fd, LETTER_BOX* lb){
    int ret = 0;

#ifdef ENABLE_RGA
    rga_buffer_handle_t src_handle, dst_handle;

    src_handle = importbuffer_fd(src_fd, lb->in_width* lb->in_height* lb->channel);
    dst_handle = importbuffer_fd(dst_fd, lb->target_width* lb->target_height* lb->channel);

    ret = _rga_resize(src_handle, dst_handle, lb);

    if (src_handle > 0){
        releasebuffer_handle(src_handle);}
    if (dst_handle > 0){
        releasebuffer_handle(dst_handle);}
#else
    ret = -1;
#endif

    return ret;
}



inline static int clamp(float val, int min, int max)
{
    return val > min ? (val < max ? val : max) : min;
}

int h_reverse(int h, LETTER_BOX lb){
    if (not lb.reverse_available){
        return h;
    }
    int r_h = clamp(h, 0, lb.target_height) - lb.h_pad_top;
    r_h = clamp(r_h, 0, lb.target_height);
    r_h = r_h / lb.resize_scale_h;
    return r_h;
}

int w_reverse(int w, LETTER_BOX lb){
    if (not lb.reverse_available){
        return w;
    }
    int r_w = clamp(w, 0, lb.target_width) - lb.w_pad_left;
    r_w = clamp(r_w, 0, lb.target_width);
    r_w = r_w / lb.resize_scale_w;
    return r_w;
}


int print_letter_box_info(LETTER_BOX lb){
    printf("in_width: %d\n", lb.in_width);
    printf("in_height: %d\n", lb.in_height);
    printf("target_width: %d\n", lb.target_width);
    printf("target_height: %d\n", lb.target_height);
    printf("img_wh_ratio: %f\n", lb.img_wh_ratio);
    printf("target_wh_ratio: %f\n", lb.target_wh_ratio);
    printf("resize_scale_w: %f\n", lb.resize_scale_w);
    printf("resize_scale_h: %f\n", lb.resize_scale_h);
    printf("resize_width: %d\n", lb.resize_width);
    printf("resize_height: %d\n", lb.resize_height);
    printf("w_pad_left: %d\n", lb.w_pad_left);
    printf("w_pad_right: %d\n", lb.w_pad_right);
    printf("h_pad_top: %d\n", lb.h_pad_top);
    printf("h_pad_bottom: %d\n", lb.h_pad_bottom);
    printf("reverse_available: %d\n", lb.reverse_available);
    return 0;
}