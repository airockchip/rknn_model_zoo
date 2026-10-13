// Copyright (c) 2026 by Rockchip Electronics Co., Ltd. All Rights Reserved.
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

#ifndef _RKNN_YOLO26_DEMO_POSTPROCESS_H_
#define _RKNN_YOLO26_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include "rknn_api.h"
#include "common.h"
#include "image_utils.h"

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 256
#define OBJ_CLASS_NUM 80
#define BOX_THRESH 0.25
#define PROP_BOX_SIZE 8400

typedef struct {
    image_rect_t box;
    float prop;
    int cls_id;
} object_detect_result;

typedef struct {
    int id;
    int count;
    object_detect_result results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

int init_post_process();
void deinit_post_process();
char *coco_cls_to_name(int cls_id);

/**
 * @brief YOLO26 post-process (No NMS needed due to one2one head)
 * 
 * @param app_ctx Application context
 * @param outputs Model outputs (rknn_output array)
 * @param letter_box Letterbox info for coordinate transformation
 * @param conf_threshold Confidence threshold
 * @param od_results Output detection results
 * @return int 0 on success, -1 on failure
 */
int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, 
                 float conf_threshold, object_detect_result_list *od_results);

#endif //_RKNN_YOLO26_DEMO_POSTPROCESS_H_
