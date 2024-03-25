#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <memory>

#include "deeplabv3.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"

#include "gpu_compose_impl.h"
#include "cl_kernels/kernel_upsampleSoftmax.h"



using namespace gpu_postprocess;

namespace {
    const constexpr char UPSAMPLE_SOFTMAX_KERNEL_NAME[] = "UpsampleSoftmax";
    const constexpr char UP_SOFTMAX_IN0[] =  "UP_SOFTMAX_IN";
    const constexpr char UP_SOFTMAX_OUT0[] =  "UP_SOFTMAX_OUT";

    const size_t NUM_LABEL = 21;
    std::shared_ptr<gpu_compose_impl> Gpu_Impl = nullptr;
    
}  

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

static void dump_tensor_attr(rknn_tensor_attr* attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
            "zp=%d, scale=%f\n",
            attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
            attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
            get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

typedef struct {
    float value;
    int index;
} element_t;


//blending two images
//using 0 gamma, 0.5 a
static void compose_img(uint8_t *res_buf, uint8_t *img_buf, int height, int width)
{
  const float alpha = 0.5f;
  float beta = 1.0 - alpha;

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      unsigned char map_label = res_buf[h * width + w];
      // printf("[%d][%d]: %d\n",h,w,pixel);

      auto ori_pixel_r = img_buf[h * width * 3 + w * 3];
      auto ori_pixel_g = img_buf[h * width * 3 + w * 3 + 1];
      auto ori_pixel_b = img_buf[h * width * 3 + w * 3 + 2];
     
      img_buf[h * width * 3 + w * 3] = FULL_COLOR_MAP[map_label][0] * alpha + ori_pixel_r * beta; 
      img_buf[h * width * 3 + w * 3 + 1] = FULL_COLOR_MAP[map_label][1] * alpha + ori_pixel_g * beta; //g 
      img_buf[h * width * 3 + w * 3 + 2] = FULL_COLOR_MAP[map_label][2] * alpha + ori_pixel_b * beta; //b
    
    }
  }
}




void swap(element_t* a, element_t* b) {
    element_t temp = *a;
    *a = *b;
    *b = temp;
}

int partition(element_t arr[], int low, int high) {
    float pivot = arr[high].value;
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (arr[j].value >= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }

    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quick_sort(element_t arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

void softmax(float* array, int size) {
    // Find the maximum value in the array
    float max_val = array[0];
    for (int i = 1; i < size; i++) {
        if (array[i] > max_val) {
            max_val = array[i];
        }
    }

    // Subtract the maximum value from each element to avoid overflow
    for (int i = 0; i < size; i++) {
        array[i] -= max_val;
    }

    // Compute the exponentials and sum
    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        array[i] = expf(array[i]);
        sum += array[i];
    }

    // Normalize the array by dividing each element by the sum
    for (int i = 0; i < size; i++) {
        array[i] /= sum;
    }
}



int init_deeplabv3_model(const char* model_path, rknn_app_context_t* app_ctx)
{
    using namespace std;
    
    int ret;
    int model_len = 0;
    char* model;
    rknn_context ctx = 0;

    if (!Gpu_Impl)
        Gpu_Impl = make_shared<gpu_compose_impl>();     

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

int release_deeplabv3_model(rknn_app_context_t* app_ctx)
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

int inference_deeplabv3_model(rknn_app_context_t* app_ctx, image_buffer_t* src_img)
{
    using namespace std;

    int ret;
    image_buffer_t img;

    memset(&img, 0, sizeof(image_buffer_t));
    //fetch model IO info according to NHWC layout !!!
    //OUT_SIZE is only for square output size 
    size_t OUT_SIZE=0;
    size_t MASK_SIZE=0;

    if (app_ctx->input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        printf("model is NCHW input fmt\n");
        OUT_SIZE = app_ctx->output_attrs[0].dims[2]; //65  
        MASK_SIZE = app_ctx->input_attrs[0].dims[3]; //513  
    }
    else {
        printf("model is NHWC input fmt\n");
        OUT_SIZE = app_ctx->output_attrs[0].dims[2]; //65  
        MASK_SIZE = app_ctx->input_attrs[0].dims[2]; //513  
    }

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

    if (src_img->width != app_ctx->model_width || src_img->height != app_ctx->model_height) {
        ret = convert_image(src_img, &img, NULL, NULL, 0);
        if (ret < 0) {
            printf("convert_image failc ret=%d\n", ret);
            return -1;
        }
    }
    else {
        memcpy(img.virt_addr, src_img->virt_addr, MASK_SIZE*MASK_SIZE*3);
    }


    const auto num_inputs = app_ctx->io_num.n_input;
    rknn_tensor_mem *input_mems[app_ctx->io_num.n_input];

    for (size_t i=0; i < num_inputs;i++) {
        app_ctx->input_attrs[i].type = RKNN_TENSOR_UINT8;
        app_ctx->input_attrs[i].fmt = RKNN_TENSOR_NHWC;

        input_mems[i] = rknn_create_mem(app_ctx->rknn_ctx, app_ctx->input_attrs[i].size_with_stride); 

        auto width  = app_ctx->input_attrs[i].dims[2];
        auto stride = app_ctx->input_attrs[i].w_stride;

        // Copy input data to input tensor memory
        if (width == stride || app_ctx->input_attrs[i].fmt == RKNN_TENSOR_UNDEFINED) {
            memcpy(input_mems[i]->virt_addr, img.virt_addr, MASK_SIZE*MASK_SIZE*3);
        } else {
            auto height  = app_ctx->input_attrs[i].dims[1];
            auto channel = app_ctx->input_attrs[i].dims[3];
        // copy from src to dst with stride
            uint8_t* src_ptr = img.virt_addr;
            uint8_t* dst_ptr = (uint8_t*)input_mems[i]->virt_addr;
        // width-channel elements
            auto src_wc_elems = width * channel * sizeof(uint8_t);
            auto dst_wc_elems = stride * channel * sizeof(uint8_t);
            for (int b = 0; b < app_ctx->input_attrs[i].dims[0]; b++) {
                for (int h = 0; h < height; ++h) {
                    memcpy(dst_ptr, src_ptr, src_wc_elems);
                    src_ptr += src_wc_elems;
                    dst_ptr += dst_wc_elems;
                }
            }
        }

        ret = rknn_set_io_mem(app_ctx->rknn_ctx,  input_mems[i], &(app_ctx->input_attrs[i]));
        if (ret < 0) {
            printf("rknn_set_io_mem failed, ret=%d\n", ret);
            return -1;
        }
    }

    const auto num_outputs = app_ctx->io_num.n_output;
    rknn_tensor_mem *output_mems[num_outputs];

    for (size_t i=0; i < num_outputs;i++) {
        // allocate float32 output tensor
        auto output_size = app_ctx->output_attrs[i].n_elems * sizeof(float);
        output_mems[i] = rknn_create_mem(app_ctx->rknn_ctx, output_size);

        // default output type is depend on model, this require float32 to compute top5
        app_ctx->output_attrs[i].type = RKNN_TENSOR_FLOAT32;
        //the model itself perform the tranpose at the end to chang to nhwc format, so dont need to do layout transform again
        app_ctx->output_attrs[i].fmt = RKNN_TENSOR_NCHW;

        ret = rknn_set_io_mem(app_ctx->rknn_ctx, output_mems[i], &app_ctx->output_attrs[i]);
        if (ret < 0) {
            printf("rknn_set_io_mem fail! ret=%d\n", ret);
            return -1;
        }
    }
    
    rknn_tensor_mem *post_buf_mem[1];
    post_buf_mem[0] = rknn_create_mem(app_ctx->rknn_ctx, MASK_SIZE * MASK_SIZE);

    int out_size = app_ctx->output_attrs[0].dims[2];
    int channel_size = app_ctx->output_attrs[0].dims[3];

    printf("output_mems-> fd = %d, offset = %d, size = %d\n", output_mems[0]->fd, output_mems[0]->offset, output_mems[0]->size);
    printf("post_buf_mem-> fd = %d, offset = %d, size = %d\n", post_buf_mem[0]->fd, post_buf_mem[0]->offset, post_buf_mem[0]->size);

    vector<string> in_buf_names = {UP_SOFTMAX_IN0};
    vector<size_t> in_buf_sizes{OUT_SIZE * OUT_SIZE * NUM_LABEL * sizeof(float)};
    vector<string> out_buf_names = {UP_SOFTMAX_OUT0};
    vector<size_t> out_buf_sizes{MASK_SIZE * MASK_SIZE};
    vector<size_t> g_work_size{MASK_SIZE, MASK_SIZE};
    vector<int> in_buf_fd{output_mems[0]->fd};
    vector<int> out_buf_fd{post_buf_mem[0]->fd};

    Gpu_Impl->addZeroCopyWorkflow(CL_kernel_string_src, UPSAMPLE_SOFTMAX_KERNEL_NAME, in_buf_names, in_buf_sizes, in_buf_fd,
                             out_buf_names, out_buf_sizes, out_buf_fd, g_work_size,
                             {}, {}, {}, {}, {}, {});

    float scale_w_inv = OUT_SIZE / (float)MASK_SIZE;
    float scale_h_inv = OUT_SIZE / (float)MASK_SIZE;
    
    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    const auto SRC_STRIDE = OUT_SIZE * NUM_LABEL;

    // Post Process
    Gpu_Impl->UpsampleSoftmax(UPSAMPLE_SOFTMAX_KERNEL_NAME, UP_SOFTMAX_IN0, nullptr,
                           UP_SOFTMAX_OUT0, nullptr, OUT_SIZE, OUT_SIZE, MASK_SIZE, MASK_SIZE, NUM_LABEL, scale_h_inv, scale_w_inv, SRC_STRIDE);
    
    compose_img((unsigned char *)post_buf_mem[0]->virt_addr, src_img->virt_addr, MASK_SIZE, MASK_SIZE);

    //For debugging purpose
    //Dump_bin_to_file(out_result->img, "test_img_out.bin", MASK_SIZE*MASK_SIZE*3*sizeof(uint8_t));

    // Remeber to release rknn tensor 
    rknn_destroy_mem(app_ctx->rknn_ctx, post_buf_mem[0]);

    for (size_t i=0; i < app_ctx->io_num.n_input; i++) {
        rknn_destroy_mem(app_ctx->rknn_ctx, input_mems[i]);
    }

    for (size_t i=0; i < app_ctx->io_num.n_output; i++) {
        rknn_destroy_mem(app_ctx->rknn_ctx, output_mems[i]);
    }


out:
    if (img.virt_addr != NULL) {
        free(img.virt_addr);
    }

    return ret;
}