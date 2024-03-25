#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory>
#include "deeplabv3.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include <opencv2/opencv.hpp>

#define NUM_LABEL 21

static int Dump_bin_to_file(void *pBuffer, const char *fileName, const size_t sz_data)
{

    FILE *pFile = fopen(fileName, "wb");
    if (pFile == NULL)
    {
        puts("error in outputing files.");
        return -1;
    }

    fwrite(pBuffer, 1, sz_data, pFile);
    fflush(pFile);

    if (fclose(pFile) != 0)
    {
        puts("Error in closing files.");
        return -1;
    }

    return 0;
}

static constexpr int FULL_COLOR_MAP[NUM_LABEL][3] = {
    {0, 0, 0},
    {128, 0, 0},
    {0, 128, 0},
    {128, 128, 0},
    {0, 0, 128},
    {128, 0, 128},
    {0, 128, 128},
    {128, 128, 128},
    {64, 0, 0},
    {192, 0, 0},
    {64, 128, 0},
    {192, 128, 0},
    {64, 0, 128},
    {192, 0, 128},
    {64, 128, 128},
    {192, 128, 128},
    {0, 64, 0},
    {128, 64, 0},
    {0, 192, 0},
    {128, 192, 0},
    {0, 64, 128}

};

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2], attr->dims[1], attr->dims[0],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

static void resize_by_opencv(float *input_image, int input_width, int input_height, float *output_image, int target_width, int target_height)
{
    cv::Mat src_image(input_height, input_width, CV_MAKETYPE(CV_32F, NUM_LABEL), input_image);
    cv::Mat dst_image(target_height, target_width, CV_MAKETYPE(CV_32F, NUM_LABEL), output_image);
    cv::resize(src_image, dst_image, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
    memcpy(output_image, dst_image.data, target_width * target_height * NUM_LABEL * sizeof(float));
}

static void compose_img(uint8_t *res_buf, uint8_t *img_buf, int height, int width)
{
    // blending two images
    // using 0 gamma, 0.5 a
    const float alpha = 0.5f;
    float beta = 1.0 - alpha;

    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            unsigned char map_label = res_buf[h * width + w];

            auto ori_pixel_r = img_buf[h * width * 3 + w * 3];
            auto ori_pixel_g = img_buf[h * width * 3 + w * 3 + 1];
            auto ori_pixel_b = img_buf[h * width * 3 + w * 3 + 2];

            img_buf[h * width * 3 + w * 3] = FULL_COLOR_MAP[map_label][0] * alpha + ori_pixel_r * beta;
            img_buf[h * width * 3 + w * 3 + 1] = FULL_COLOR_MAP[map_label][1] * alpha + ori_pixel_g * beta; // g
            img_buf[h * width * 3 + w * 3 + 2] = FULL_COLOR_MAP[map_label][2] * alpha + ori_pixel_b * beta; // b
        }
    }
}

int init_deeplabv3_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    using namespace std;

    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL)
    {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0);
    free(model);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;
    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        app_ctx->model_channel = input_attrs[0].dims[2];
        app_ctx->model_height = input_attrs[0].dims[1];
        app_ctx->model_width = input_attrs[0].dims[0];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        app_ctx->model_height = input_attrs[0].dims[2];
        app_ctx->model_width = input_attrs[0].dims[1];
        app_ctx->model_channel = input_attrs[0].dims[0];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int release_deeplabv3_model(rknn_app_context_t *app_ctx)
{
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

static void post_process(float *input, uint8_t *output, int seg_width, int seg_height, int n_label, int out_width, int out_height)
{
    float *mask = (float *)malloc(out_width * out_height * n_label * sizeof(float));
    resize_by_opencv(input, seg_width, seg_height, mask, out_width, out_height);

    // Find the index of the maximum value along the last axis
    int max_index;
    for (int i = 0; i < out_height * out_width; i++)
    {
        max_index = 0;
        for (int j = 1; j < n_label; j++)
        {
            if (mask[i * n_label + j] > mask[i * n_label + max_index])
            {
                max_index = j;
            }
        }
        output[i] = max_index;
    }

    free(mask);
}

int inference_deeplabv3_model(rknn_app_context_t *app_ctx, image_buffer_t *src_img)
{
    int ret;
    image_buffer_t img;
    rknn_input inputs[1];
    rknn_output outputs[1];

    memset(&img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    img.width = app_ctx->model_width;
    img.height = app_ctx->model_height;
    img.format = IMAGE_FORMAT_RGB888;
    img.size = get_image_size(&img);
    img.virt_addr = (unsigned char *)malloc(img.size);
    uint8_t *seg_img = (uint8_t *)malloc(img.width * img.height * sizeof(uint8_t));
    if (img.virt_addr == NULL)
    {
        printf("malloc buffer size:%d fail!\n", img.size);
        return -1;
    }

    ret = convert_image(src_img, &img, NULL, NULL, 0);
    if (ret < 0)
    {
        printf("convert_image fail! ret=%d\n", ret);
        return -1;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf = img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    // Get Output
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post Process
    post_process((float *)outputs[0].buf, seg_img, app_ctx->output_attrs[0].dims[2], app_ctx->output_attrs[0].dims[1], app_ctx->output_attrs[0].dims[0],
                 img.width, img.height);

    // draw mask
    compose_img(seg_img, src_img->virt_addr, src_img->height, src_img->width);
    free(seg_img);

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);

out:
    if (img.virt_addr != NULL)
    {
        free(img.virt_addr);
    }

    return ret;
}