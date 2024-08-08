#ifndef _RKNN_YOLOV8_POSE_DEMO_POSTPROCESS_H_
#define _RKNN_YOLOV8_POSE_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include "rknn_api.h"
#include "common.h"
#include "image_utils.h"

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_CLASS_NUM 1
#define NMS_THRESH 0.4
#define BOX_THRESH 0.5
#define PROP_BOX_SIZE (5 + OBJ_CLASS_NUM)

// class rknn_app_context_t;

typedef struct {
    image_rect_t box;
    float keypoints[17][3];//keypoints x,y,conf
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
int post_process(rknn_app_context_t *app_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results);

void deinitPostProcess();
#endif //_RKNN_YOLOV5_DEMO_POSTPROCESS_H_
