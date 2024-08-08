// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "clip_text.h"
#include "yolo_world.h"
#include "file_utils.h"
#include "image_utils.h"
#include "image_drawing.h"


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 5)
    {
        printf("%s <text_model_path> <text_path> <yolo_world_model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char *text_model_path = argv[1];
    const char *text_path = argv[2];
    const char *yolo_world_path = argv[3];
    const char *img_path = argv[4];

    int ret;
    rknn_clip_context rknn_clip_ctx;
    rknn_app_context_t rknn_yolo_world_ctx;
    memset(&rknn_clip_ctx, 0, sizeof(rknn_clip_context));
    memset(&rknn_yolo_world_ctx, 0, sizeof(rknn_app_context_t));

    printf("--> init clip text model\n");
    ret = init_clip_text_model(&rknn_clip_ctx, text_model_path);
    if (ret != 0)
    {
        printf("init clip text model fail! ret=%d\n", ret);
        return -1;
    }

    init_post_process();

    printf("--> init yolo world model\n");
    ret = init_yolo_world_model(&rknn_yolo_world_ctx, yolo_world_path);
    if (ret != 0)
    {
        printf("init yolo world model fail! ret=%d\n", ret);
        return -1;
    }

    int text_lines;
    char** input_texts = read_lines_from_file(text_path, &text_lines);
    if (input_texts == NULL)
    {
        printf("read input texts fail! ret=%d text_path=%s\n", ret, text_path);
        return -1;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(img_path, &src_image);
    if (ret != 0)
    {
        printf("read image fail! ret=%d image_path=%s\n", ret, img_path);
        return -1;
    }

    int text_size = text_lines * rknn_clip_ctx.output_attrs[0].dims[1];
    float text_output[text_size];

    printf("--> inference clip text model\n");
    ret = inference_clip_text_model(&rknn_clip_ctx, input_texts, text_lines, text_output);
    if (ret != 0)
    {
        printf("inference_clip_model fail! ret=%d\n", ret);
        goto out;
    }

    object_detect_result_list od_results;

    printf("--> inference yolo world model\n");
    ret = inference_yolo_world_model(&rknn_yolo_world_ctx, &src_image, text_output, text_size, &od_results);
    if (ret != 0)
    {
        printf("init_yolo_world_model fail! ret=%d\n", ret);
        goto out;
    }

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
               det_result->box.left, det_result->box.top,
               det_result->box.right, det_result->box.bottom,
               det_result->prop);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);
    }

    write_image("out.png", &src_image);


out:
    ret = release_clip_text_model(&rknn_clip_ctx);
    if (ret != 0)
    {
        printf("release_clip_model fail! ret=%d\n", ret);
    }

    deinit_post_process();

    ret = release_yolo_world_model(&rknn_yolo_world_ctx);
    if (ret != 0)
    {
        printf("release_yolo_world_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL)
    {

        free(src_image.virt_addr);
    }

    if (input_texts != NULL)
    {
        free_lines(input_texts, text_lines);
    }


    return 0;
}
