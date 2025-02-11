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

#include "mobilesam.h"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"


/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 6)
    {
        printf("%s <encoder_model_path> <image_path> <decoder_model_path> <point_coords_path> <point_labels_path>\n", argv[0]);
        return -1;
    }

    const char *encoder_model_path = argv[1];
    const char *img_path = argv[2];
    const char *decoder_model_path = argv[3];
    const char *point_coords_path = argv[4];
    const char *point_labels_path = argv[5];

    int ret;
    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    ret = init_mobilesam_model(encoder_model_path, decoder_model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_mobilesam_model fail! ret=%d encoder_model_path=%s decoder_model_path=%s\n", ret, encoder_model_path, decoder_model_path);
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

    int coords_nums;
    float* ori_point_coords = read_coords_from_file(point_coords_path, &coords_nums);
    if (ori_point_coords == NULL)
    {
        printf("read point_coords fail! ret=%d point_coords_path=%s\n", ret, point_coords_path);
        return -1;
    }
    float* cvt_point_coords = (float*)malloc(coords_nums * 2 * sizeof(float));
    point_coords_preprocess(ori_point_coords, coords_nums*2, src_image.height, src_image.width, cvt_point_coords);

    int labels_nums;
    float* point_labels = read_coords_from_file(point_labels_path, &labels_nums);
    if (point_labels == NULL)
    {
        printf("read point_labels fail! ret=%d point_labels_path=%s\n", ret, point_labels_path);
        return -1;
    }

    if (coords_nums != labels_nums)
    {
        printf("point coords numbers are not equal points labels numbers\n");
        return -1;
    }

    mobilesam_res res;

    ret = inference_mobilesam_model(&rknn_app_ctx, &src_image, cvt_point_coords, point_labels, &res);
    if (ret != 0)
    {
        printf("inference_mobilesam_model fail! ret=%d\n", ret);
        goto out;
    }

    // draw mask
    draw_mask(&src_image, res.mask);

    // draw point or box
    mobilesam_box box;
    for (int i = 0; i < labels_nums; i++)
    {
        if (point_labels[i] == 0)
        {
            int point_x = ori_point_coords[i * 2];
            int point_y = ori_point_coords[i * 2 + 1];
            draw_circle(&src_image, point_x, point_y, 12, COLOR_RED, 2);
        }
        else if (point_labels[i] == 1)
        {
            int point_x = ori_point_coords[i * 2];
            int point_y = ori_point_coords[i * 2 + 1];
            draw_circle(&src_image, point_x, point_y, 12, COLOR_GREEN, 2);
        }
        else if (point_labels[i] == 2 || point_labels[i] == 3)
        {
            if (point_labels[i] == 2)
            {
                box.x1 = ori_point_coords[i * 2];
                box.y1 = ori_point_coords[i * 2 + 1];
            }
            else if (point_labels[i] == 3)
            {
                box.x2 = ori_point_coords[i * 2];
                box.y2 = ori_point_coords[i * 2 + 1];
            }
        }
    }
    if (!(box.x1 == 0 && box.y1 == 0 && box.x2 == 0 && box.y2 == 0))
    {
        draw_rectangle(&src_image, box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1, COLOR_GREEN, 2);
    }

    ret = write_image("out.png", &src_image);
    if (ret < 0) {
        printf("write out.png fail\n");
        goto out;
    }
    else {
        printf("result save to out.png");
    }


out:
    ret = release_mobilesam_model(&rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_mobilesam_model fail! ret=%d\n", ret);
    }

    if (src_image.virt_addr != NULL)
    {
        free(src_image.virt_addr);
    }

    if (ori_point_coords != NULL)
    {
        free(ori_point_coords);
    }

    if (cvt_point_coords != NULL)
    {
        free(cvt_point_coords);
    }

    if (point_labels != NULL)
    {
        free(point_labels);
    }

    if (res.mask != NULL)
    {
        free(res.mask);
    }

    return 0;
}
