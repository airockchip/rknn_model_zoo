#ifndef _RKNN_DEMO_PPOCRDET_H_
#define _RKNN_DEMO_PPOCRDET_H_

#include "rknn_api.h"
#include "common.h"
#include <string>

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
    int model_channel;
    int model_width;
    int model_height;
} rknn_app_context_t;

typedef struct rknn_point_t
{
    int x;  ///< X Coordinate
    int y;  ///< Y Coordinate
} rknn_point_t;

typedef struct rknn_quad_t
{
    rknn_point_t left_top;      // Left top point
    rknn_point_t right_top;     // Right top point
    rknn_point_t left_bottom;   // Left bottom point
    rknn_point_t right_bottom;  // Right bottom point
    float score;
} rknn_quad_t;

typedef struct {
    rknn_quad_t box[1000];                             // text location bounding box，(left top/right top/right bottom/left bottom)
    int count;                                                             // box num
} ppocr_det_result;

typedef struct ppocr_det_postprocess_params {
    float threshold;
    float box_threshold;
    bool use_dilate;
    char* db_score_mode;
    char* db_box_type;
    float db_unclip_ratio;
} ppocr_det_postprocess_params;

int init_ppocr_det_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_ppocr_det_model(rknn_app_context_t* app_ctx);

int inference_ppocr_det_model(rknn_app_context_t* app_ctx, image_buffer_t* img, ppocr_det_postprocess_params* params, ppocr_det_result* out_result);

int dbnet_postprocess(float* output, int det_out_w, int det_out_h, float db_threshold, float db_box_threshold, bool use_dilation,
                                                const std::string &db_score_mode, const float &db_unclip_ratio, const std::string &db_box_type,
                                                float scale_w, float scale_h, ppocr_det_result* results);

#endif //_RKNN_DEMO_PPOCRDET_H_