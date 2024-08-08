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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yolov8-obb.h"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("%s <model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char *model_path = argv[1];
    const char *image_path = argv[2];

    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    init_post_process();

    ret = init_yolov8_obb_model(model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_yolov8_obb_model fail! ret=%d model_path=%s\n", ret, model_path);
        goto out;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);

    if (ret != 0)
    {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        goto out;
    }

    object_detect_result_list od_results;

    ret = inference_yolov8_obb_model(&rknn_app_ctx, &src_image, &od_results);
    if (ret != 0)
    {
        printf("inference_yolov8_obb_model fail! ret=%d\n", ret);
        goto out;
    }

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        printf("%s @ (%d %d %d %d angle=%f) %.3f\n", coco_cls_to_name(det_result->cls_id),
               det_result->box.x, det_result->box.y,
               det_result->box.w, det_result->box.h,
               det_result->box.angle,det_result->prop);
        int x1 = det_result->box.x;
        int y1 = det_result->box.y;
        int w = det_result->box.w;
        int h = det_result->box.h;
        float angle = det_result->box.angle;

        draw_obb_rectangle(&src_image, x1, y1, w, h, angle, COLOR_BLUE, 3);

        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 8);
    }

    write_image("out.png", &src_image);

out:
    deinit_post_process();

    ret = release_yolov8_obb_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov5_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL)
    {

        free(src_image.virt_addr);
    }

    return 0;
}
