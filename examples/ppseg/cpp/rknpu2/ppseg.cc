#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <chrono>
#include "ppseg.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

// Define the type of color
using Color = std::tuple<int, int, int>;

//Define a structure to represent a row of the table
struct Entry {
    int id;
    const char* name;
    Color color;
};

//Define a global table
Entry cityscapes_label[] = {
    {0, "road", Color(128, 64, 128)},
    {1, "sidewalk", Color(244, 35, 232)},
    {2, "building", Color(70, 70, 70)},
    {3, "wall", Color(102, 102, 156)},
    {4, "fence", Color(190, 153, 153)},
    {5, "pole", Color(153, 153, 153)},
    {6, "traffic light", Color(250, 170, 30)},
    {7, "traffic sign", Color(220, 220, 0)},
    {8, "vegetation", Color(107, 142, 35)},
    {9, "terrain", Color(152, 251, 152)},
    {10, "sky", Color(70, 130, 180)},
    {11, "person", Color(220, 20, 60)},
    {12, "rider", Color(255, 0, 0)},
    {13, "car", Color(0, 0, 142)},
    {14, "truck", Color(0, 0, 70)},
    {15, "bus", Color(0, 60, 100)},
    {16, "train", Color(0, 80, 100)},
    {17, "motorcycle", Color(0, 0, 230)},
    {18, "bicycle", Color(119, 11, 32)}
};


static void dump_tensor_attr(rknn_tensor_attr* attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
            "zp=%d, scale=%f\n",
            attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
            attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
            get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

Color getColorById(int id) {
    for (const auto& entry : cityscapes_label) {
        if (entry.id == id) {
            return entry.color;
        }
    }
    return Color(0, 0, 0);
}

int draw_segment_image(float* result, image_buffer_t* result_img)
{
    int height = result_img->height;
    int width = result_img->width;
    int num_class = 19;
    result_img->virt_addr = (unsigned char*)malloc(3*height*width);
    memset(result_img->virt_addr, 0, 3*height*width);
    // [1,class,height,width] -> [1,3,height,width]
    for (int batch = 0; batch < 1; batch++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int maxClassIndex = 0;
                for (int c = 1; c < num_class; c++) {
                    int currentIndex = batch * (num_class * height * width) + c * (height * width) + y * width + x;
                    int maxClassPos = batch * (num_class * height * width) + maxClassIndex * (height * width) + y * width + x;
                    if (result[currentIndex] > result[maxClassPos]) {
                        maxClassIndex = c;
                    }
                }
                Color foundColor = getColorById(maxClassIndex);

                int imageIndex = batch * (3 * height * width) + y * width * 3 + x * 3;
                result_img->virt_addr[imageIndex] = std::get<0>(foundColor);       // R
                result_img->virt_addr[imageIndex + 1] = std::get<1>(foundColor);   // G
                result_img->virt_addr[imageIndex + 2] = std::get<2>(foundColor);   // B
            }
        }
    }
    return 0;
}

int init_ppseg_model(const char* model_path, rknn_app_context_t* app_ctx)
{
    int ret;
    int model_len = 0;
    char* model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL) {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0) {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;
    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr*)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr*)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        app_ctx->model_channel = input_attrs[0].dims[1];
        app_ctx->model_height  = input_attrs[0].dims[2];
        app_ctx->model_width   = input_attrs[0].dims[3];
    } else {
        printf("model is NHWC input fmt\n");
        app_ctx->model_height  = input_attrs[0].dims[1];
        app_ctx->model_width   = input_attrs[0].dims[2];
        app_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
        app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int release_ppseg_model(rknn_app_context_t* app_ctx)
{
    if (app_ctx->input_attrs != NULL) {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL) {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0) {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_ppseg_model(rknn_app_context_t* app_ctx, image_buffer_t* src_img, image_buffer_t* result_img)
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
    img.virt_addr = (unsigned char*)malloc(img.size);
    if (img.virt_addr == NULL) {
        printf("malloc buffer size:%d fail!\n", img.size);
        return -1;
    }

    ret = convert_image(src_img, &img, NULL, NULL, 0);
    if (ret < 0) {
        printf("convert_image fail! ret=%d\n", ret);
        return -1;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type  = RKNN_TENSOR_UINT8;
    inputs[0].fmt   = RKNN_TENSOR_NHWC;
    inputs[0].size  = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf   = img.virt_addr;

    
    ret = rknn_inputs_set(app_ctx->rknn_ctx, 1, inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    auto start = std::chrono::high_resolution_clock::now();
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "rknn run cost: " << float(duration.count()/1000.0) << " ms" << std::endl;

    // Get Output
    outputs[0].want_float = 1;
    ret = rknn_outputs_get(app_ctx->rknn_ctx, 1, outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post Process
    // outputs -> take top1 pixel by pixel -> assign color
    ret = draw_segment_image((float* )outputs[0].buf, result_img);
    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, 1, outputs);

out:
    if (img.virt_addr != NULL) {
        free(img.virt_addr);
    }

    return ret;
}