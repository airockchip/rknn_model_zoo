// Copyright (c) 2021 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <vector>
#include <string>
#include <stdbool.h>
#include <timer.h>

#define _BASETSD_H

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif 

// #ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
// #include <stb/stb_image_resize.h>
// #endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#endif

#undef cimg_display
#define cimg_display 0
#undef cimg_use_jpeg
#define cimg_use_jpeg 1
#undef cimg_use_png
#define cimg_use_png 1
#include "CImg/CImg.h"

#include "rknn_api.h"
#include "yolo.h"
#include "resize_function.h"
#include "rknn_demo_utils.h"

#define PERF_WITH_POST 1
#define COCO_IMG_NUMBER 5000
#define INDENT "    "

using namespace cimg_library;
/*-------------------------------------------
                  Functions
-------------------------------------------*/

static int saveFloat(const char *file_name, float *output, int element_size)
{
    FILE *fp;
    fp = fopen(file_name, "w");
    for (int i = 0; i < element_size; i++)
    {
        fprintf(fp, "%.6f\n", output[i]);
    }
    fclose(fp);
    return 0;
}

static unsigned char *load_image(const char *image_path, int *org_height, int *org_width, int *org_ch, rknn_tensor_attr *input_attr)
{
    int req_height = 0;
    int req_width = 0;
    int req_channel = 0;

    switch (input_attr->fmt)
    {
    case RKNN_TENSOR_NHWC:
        req_height = input_attr->dims[2];
        req_width = input_attr->dims[1];
        req_channel = input_attr->dims[0];
        break;
    case RKNN_TENSOR_NCHW:
        //Need to double check dims!!!!!
        req_height = input_attr->dims[1];
        req_width = input_attr->dims[0];
        req_channel = input_attr->dims[2];
        break;
    default:
        printf("meet unsupported layout\n");
        return NULL;
    }


    int height = 0;
    int width = 0;
    int channel = 0;

    unsigned char *image_data = stbi_load(image_path, &width, &height, &channel, req_channel);
    if (image_data == NULL)
    {
        printf("load image-%s failed!\n", image_path);
        return NULL;
    }

    if (channel == 1){
        printf("image is grey, convert to RGB");
        void* rgb_data = malloc(width* height* 3);
        for(int i=0; i<height; i++){
            for(int j=0; j<width; j++){
                    int offset = (i*width + j)*3;
                    ((unsigned char*)rgb_data)[offset] = ((unsigned char*)image_data)[offset];
                    ((unsigned char*)rgb_data)[offset + 1] = ((unsigned char*)image_data)[offset];
                    ((unsigned char*)rgb_data)[offset + 2] = ((unsigned char*)image_data)[offset];
            }
        }
        free(image_data);
        image_data = (unsigned char*)rgb_data;
        channel = 3;
    }

//     int align_width = 4;

// #ifdef PLATFORM_RK3588
//     align_width = 16;    
// #endif

//     if (width % align_width != 0){
//         int new_width = width+ (align_width - width % align_width);
//         printf("%d is not pixel align at %d, which RGA REQUIRE. Using stb resize to %d, this will make the result shift slightly\n",width, align_width, new_width);
//         void* resize_data = malloc(new_width* height* channel);
//         stbir_resize_uint8(image_data, width, height, 0, (unsigned char*)resize_data, new_width, height, 0, channel);
//         free(image_data);
//         image_data = (unsigned char*)resize_data;
//         *org_width = new_width;
//     }
//     else{
//         *org_width = width;
//     }

    *org_width = width;
    *org_height = height;
    *org_ch = channel;

    return image_data;
}

int load_hyperm_param(YOLO_INFO *m, int argc, char** argv){
    if (argc != 5)
    {
        // printf("Usage: %s Model_type [fp/q8] [single_img/multi_imgs] <rknn model path> <anchor file path> <input_path>\n", argv[0]);
        printf("Usage: %s Model_type [fp/q8] <rknn model path> <input_path>\n", argv[0]);
        printf("  -- [1] Model type, select from v5,v6,v7,v8, or yolov5,yolov6,yolov7,yolov8,yolox,ppyoloe_plus\n");
        printf("  -- [2] Post process type, select from fp, q8. Only quantize-8bit model could use q8\n");
        printf("  -- [3] RKNN model path\n");
        printf("  -- [4] input path\n");
        return -1;
    }

    printf("MODEL HYPERPARAM:\n");
    int ret=0;
    // m->m_path = (char *)argv[4];
    m->in_path = (char *)argv[4];

    m->m_type = string_to_model_type(argv[1]);
    printf("%sModel type: %s, %d\n", INDENT, argv[1], m->m_type);
    // m->color_expect = RK_FORMAT_RGB_888;

    const char* anchor_path;
    switch (m->m_type)
    {
    case YOLOV5:
        m->anchor_per_branch = 3;
        anchor_path = "./model/anchors_yolov5.txt";
        break;
    case YOLOV7:
        m->anchor_per_branch = 3;
        anchor_path = "./model/anchors_yolov7.txt";
        break;
    case YOLOX:
        m->anchor_per_branch = 1;
        /*
            RK_FORMAT_RGB_888 if normal api
            RK_FORMAT_BGR_888 if pass_through/ zero_copy
        */
        // m->color_expect = RK_FORMAT_RGB_888;
        break;
    default:
        m->anchor_per_branch = 1;
        break;
    }

    if ((m->m_type == YOLOV5) || (m->m_type == YOLOV7)){
        int n = 2* 3* m->anchor_per_branch;
        printf("%sAnchors: ", INDENT);
        float result[n];
        int valid_number;
        ret = readFloats(anchor_path, &result[0], n, &valid_number);
        for (int i=0; i<valid_number; i++){
            m->anchors[i] = (int)result[i];
            printf("%d ", m->anchors[i]);
        }
        printf("\n");
    }
    else {
        printf("%sAnchor free\n", INDENT);
    }

    if (strcmp(argv[2], "fp") == 0){
        m->post_type = FP;
        printf("%sPost process with: fp\n", INDENT);
    }
    else if (strcmp(argv[2], "q8") == 0){
        m->post_type = Q8;
        printf("%sPost process with: q8\n", INDENT);
    }
    else{
        printf("Post process type not support: %s\nPlease select from [fp/q8]\n", argv[2]);
        return -1;
    }

    m->in_source = SINGLE_IMG;

    return 0;
}

void query_dfl_len(MODEL_INFO *m, YOLO_INFO *y_info){
    // set dfl_len
    if ((y_info->m_type == YOLOV8) || (y_info->m_type == PPYOLOE_PLUS) || (y_info->m_type == YOLOV6)){
        if (m->n_output>6){
            y_info->score_sum_available = true;
        }
        else{
            y_info->score_sum_available = false;
        }

        if ((y_info->m_type == YOLOV8) || (y_info->m_type == PPYOLOE_PLUS)){
            y_info->dfl_len = (int)(m->out_attr[0].dims[2]/4);
        }

        if (y_info->m_type == YOLOV6){
            // dump_tensor_attr(&m->out_attr[0]);
            if (m->out_attr[0].dims[2] != 4){
                y_info->dfl_len = (int)(m->out_attr[0].dims[2]/4);
            }
        }
    }
}


/*-------------------------------------------
                  Main Functions
-------------------------------------------*/
int main(int argc, char **argv)
{
    int status = 0;

    MODEL_INFO m_info;
    YOLO_INFO y_info;
    LETTER_BOX letter_box;

    TIMER timer;
    timer.indent_set("    ");
    int ret;

    ret = load_hyperm_param(&y_info, argc, argv);
    if (ret < 0) return -1;

    m_info.m_path = argv[3];
    // m_info.verbose_log = true;
    rkdemo_init(&m_info);
    rkdemo_init_input_buffer_all(&m_info, ZERO_COPY_API, RKNN_TENSOR_UINT8);
    for (int i=0; i< m_info.n_input; i++){
        rknn_set_io_mem(m_info.ctx, m_info.input_mem[i], &m_info.in_attr[i]);
    }

    if (y_info.post_type == Q8){
        rkdemo_init_output_buffer_all(&m_info, ZERO_COPY_API, 0);
    }
    else{
        printf("zero copy not support fp post process\n");
        return -1;
    }
    for (int i=0; i< m_info.n_output; i++){
        rknn_set_io_mem(m_info.ctx, m_info.output_mem[i], &m_info.out_attr[i]);
    }

    query_dfl_len(&m_info, &y_info);

    void* output_buf_list[m_info.n_output];

    switch (m_info.in_attr[0].fmt)
    {
        case RKNN_TENSOR_NHWC:
            letter_box.target_height = m_info.in_attr[0].dims[2];
            letter_box.target_width = m_info.in_attr[0].dims[1];
            break;
        case RKNN_TENSOR_NCHW:
            letter_box.target_height = m_info.in_attr[0].dims[1];
            letter_box.target_width = m_info.in_attr[0].dims[0];
            break;
        default:
            printf("meet unsupported layout\n");
            return NULL;
    }
    unsigned char *input_data = NULL;
    unsigned char *resize_buf = (unsigned char *)malloc(letter_box.target_height* letter_box.target_width* 3);
    if (resize_buf == NULL){
        printf("resize buf alloc failed\n");
        return -1;
    }

    /* Single img input */
    /* Due to different input img size, multi img method has to calculate letterbox param each time*/
    if (y_info.in_source == SINGLE_IMG)
    {
        /* Input preprocess */
        // Load image
        CImg<unsigned char> img(y_info.in_path);
        input_data = load_image(y_info.in_path, &letter_box.in_height, &letter_box.in_width, &letter_box.channel, &m_info.in_attr[0]);
        if (!input_data){
            fprintf(stderr, "Error in loading input image\n");
            return -1;
        }

        printf("img_height: %d, img_width: %d, img_channel: %d\n", letter_box.in_height, letter_box.in_width, letter_box.channel);

        // Letter box resize
        if ((letter_box.in_height == letter_box.target_height) && (letter_box.in_width == letter_box.target_width)){
            memcpy(m_info.input_mem[0]->logical_addr, input_data, m_info.in_attr[0].size);
        }
        else{
            compute_letter_box(&letter_box);
            // ret = rga_letter_box_resize(input_data, resize_buf, &letter_box);
            ret = -1;
            if (ret != 0){
                printf("RGA letter box resize failed, use stb to resize\n");
                stb_letter_box_resize(input_data, (unsigned char*)m_info.input_mem[0]->logical_addr, letter_box);
            }
            letter_box.reverse_available = true;
        }
        // stbi_write_bmp("./demo_c_input_hwc_rgb.bmp", letter_box.target_width, letter_box.target_height, 3, m_info.inputs[0].buf);

        printf("RUN MODEL ONE TIME FOR TIME DETAIL\n");
        // input set
        timer.tik();
        // rknn_inputs_set(m_info.ctx, m_info.n_input, m_info.inputs);
        timer.tok();
        timer.print_time("inputs_set");

        // rknn run
        timer.tik();
        ret = rknn_run(m_info.ctx, NULL);
        timer.tok();
        timer.print_time("rknn_run");

        // output get
        timer.tik();
        // ret = rknn_outputs_get(m_info.ctx, m_info.n_output, m_info.outputs, NULL);
        timer.tok();
        timer.print_time("outputs_get");

        /* Post process */
        detect_result_group_t detect_result_group;
        timer.tik();
        for (int i=0; i< m_info.n_output; i++){
            output_buf_list[i] = m_info.output_mem[i]->logical_addr;
        }
        post_process(output_buf_list, &m_info, &y_info, &detect_result_group);
        timer.tok();
        timer.print_time("cpu_post_process");

        // Draw Objects
        const unsigned char blue[] = {0, 0, 255};
        char score_result[64];
        printf("DRAWING OBJECT\n");
        for (int i = 0; i < detect_result_group.count; i++)
        {
            detect_result_group.results[i].box.left = w_reverse(detect_result_group.results[i].box.left, letter_box);
            detect_result_group.results[i].box.right = w_reverse(detect_result_group.results[i].box.right, letter_box);
            detect_result_group.results[i].box.top = h_reverse(detect_result_group.results[i].box.top, letter_box);
            detect_result_group.results[i].box.bottom = h_reverse(detect_result_group.results[i].box.bottom, letter_box);

            detect_result_t *det_result = &(detect_result_group.results[i]);
            printf("%s%s @ (%d %d %d %d) %f\n", INDENT,
                det_result->name,
                det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                det_result->prop);
            int x1 = det_result->box.left;
            int y1 = det_result->box.top;
            int x2 = det_result->box.right;
            int y2 = det_result->box.bottom;
            int ret = snprintf(score_result, sizeof score_result, "%f", det_result->prop);
            //draw box
            img.draw_rectangle(x1, y1, x2, y2, blue, 1, ~0U);
            img.draw_text(x1, y1 - 24, det_result->name, blue);
            img.draw_text(x1, y1 - 12, score_result, blue);
        }
        printf("    SAVE TO ./out.bmp\n");
        img.save("./out.bmp");

        // loop test without preprocess, postprocess
        int test_count = 10;
        timer.tik();
        for (int i = 0; i < test_count; ++i)
        {
            ret = rknn_run(m_info.ctx, NULL);
        }
        timer.tok();
        printf("WITHOUT POST_PROCESS\n    full run for %d loops, average time: %f ms\n", test_count,
            timer.get_time() / test_count);

        // loop test with postprocess
        timer.tik();
        for (int i = 0; i < test_count; ++i)
        {
            ret = rknn_run(m_info.ctx, NULL);
            for (int i=0; i< m_info.n_output; i++){
                output_buf_list[i] = m_info.output_mem[i]->logical_addr;
            }
            post_process(output_buf_list, &m_info, &y_info, &detect_result_group);
        }
        timer.tok();
        printf("WITH POST_PROCESS\n    full run for %d loops, average time: %f ms\n", test_count,
            timer.get_time() / test_count);

        free(input_data);
    }


    rkdemo_release(&m_info);

    if (resize_buf)
    {
        free(resize_buf);
    }

    return 0;
}
