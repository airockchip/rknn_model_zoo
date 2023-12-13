// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#include "retinaface.h"
#include "image_utils.h"
#include "image_drawing.h"
#include "file_utils.h"

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv) {
    if (argc != 3) {
        printf("%s <model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char *model_path = argv[1];
    const char *image_path = argv[2];
    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    ret = init_retinaface_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_retinaface_model fail! ret=%d model_path=%s\n", ret, model_path);
        return -1;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        return -1;
    }

    retinaface_result result;
    ret = inference_retinaface_model(&rknn_app_ctx, &src_image, &result);
    if (ret != 0) {
        printf("init_retinaface_model fail! ret=%d\n", ret);
        goto out;
    }

    for (int i = 0; i < result.count; ++i) {
        int rx = result.object[i].box.left;
        int ry = result.object[i].box.top;
        int rw = result.object[i].box.right - result.object[i].box.left;
        int rh = result.object[i].box.bottom - result.object[i].box.top;
        draw_rectangle(&src_image, rx, ry, rw, rh, COLOR_GREEN, 3);
        char score_text[20];
        snprintf(score_text, 20, "%0.2f", result.object[i].score);
        printf("face @(%d %d %d %d) score=%f\n", result.object[i].box.left, result.object[i].box.top,
               result.object[i].box.right, result.object[i].box.bottom, result.object[i].score);
        draw_text(&src_image, score_text, rx, ry, COLOR_RED, 20);
        for(int j = 0; j < 5; j++) {
            draw_circle(&src_image, result.object[i].ponit[j].x, result.object[i].ponit[j].y, 2, COLOR_ORANGE, 4);
        }
    }
    write_image("result.jpg", &src_image);

out:
    ret = release_retinaface_model(&rknn_app_ctx);
    if (ret != 0) {
        printf("release_retinaface_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
    }

    return 0;
}
