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

#include "lprnet.h"
#include "image_utils.h"
#include "file_utils.h"
#include "opencv2/opencv.hpp"

static void image_preprocess(image_buffer_t src_image)
{
    cv::Mat img_ori = cv::Mat(src_image.height, src_image.width, CV_8UC3, (uint8_t *)src_image.virt_addr);
    cv::resize(img_ori, img_ori, cv::Size(MODEL_WIDTH, MODEL_HEIGHT));
    cv::cvtColor(img_ori, img_ori, cv::COLOR_RGB2BGR);
    src_image.virt_addr = img_ori.data;
}

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
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    lprnet_result result;

    ret = init_lprnet_model(model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_lprnet_model fail! ret=%d model_path=%s\n", ret, model_path);
        goto out;
    }

    ret = read_image(image_path, &src_image);
    if (ret != 0)
    {
        printf("read image fail! ret=%d image_path=%s\n", ret, image_path);
        goto out;
    }
    // Image preprocessing
    image_preprocess(src_image);

    ret = inference_lprnet_model(&rknn_app_ctx, &src_image, &result);
    if (ret != 0)
    {
        printf("init_lprnet_model fail! ret=%d\n", ret);
        goto out;
    }

    std::cout << "车牌识别结果: " << result.plate_name << std::endl;

out:
    ret = release_lprnet_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_lprnet_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL)
    {
#if defined(RV1106_1103)
        dma_buf_free(rknn_app_ctx.img_dma_buf.size, &rknn_app_ctx.img_dma_buf.dma_buf_fd,
                     rknn_app_ctx.img_dma_buf.dma_buf_virt_addr);
#else
        free(src_image.virt_addr);
#endif
    }

    return 0;
}
