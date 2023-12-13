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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <fstream>
#include <iostream>

#include "resnet.h"
#include "image_utils.h"
#include "file_utils.h"

constexpr const char* Imagenet_classes_file_path = "./model/synset.txt";


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
   if (argc != 3) {
        printf("%s <model_path> <image_path>\n", argv[0]);
        return -1;
    }

    const char* model_path = argv[1];
    const char* image_path = argv[2];

    int line_count;
    char** lines = read_lines_from_file(Imagenet_classes_file_path, &line_count);
    if (lines == NULL) {
        printf("read classes label file fail! path=%s\n", Imagenet_classes_file_path);
        return -1;
    }

    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    ret = init_resnet_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_mobilenet_model fail! ret=%d model_path=%s\n", ret, model_path);
        return -1;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    ret = read_image(image_path, &src_image);
    if (ret != 0) {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        return -1;
    }

    int topk = 5;
    resnet_result result[topk];

    ret = inference_resnet_model(&rknn_app_ctx, &src_image, result, topk);
    if (ret != 0) {
        printf("init_mobilenet_model fail! ret=%d\n", ret);
        goto out;
    }

    for (int i = 0; i < topk; i++) {
        printf("[%d] score=%.6f class=%s\n", result[i].cls, result[i].score, lines[result[i].cls]);
    }

out:
    ret = release_resnet_model(&rknn_app_ctx);
    if (ret != 0) {
        printf("release_mobilenet_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
    }

    if (lines != NULL) {
        free_lines(lines, line_count);
    }

    return 0;

}
