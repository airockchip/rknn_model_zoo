/*
 * @Author: Chifred
 * @Date: 2021-10-26 11:47:26
 * @LastEditTime: 2023-11-29 16:22:01
 * @Editors: Chifred
 * @Description: TODO
 */
#include <stdlib.h>
#include "opencl_memory.h"
#include "opencl_wrapper.h"
#include "opencl_workspace.h"

namespace gpu_postprocess
{
    int BufferDescriptor::Create(cl_context ctx, cl_device_id dev)
    {
        cl_int err_code = 0;
        if (cl_buf_ != nullptr)
            OPENCL_CALL(clReleaseMemObject(cl_buf_));
        if (mem_type_ == kImport)
        {
            LOGI("createCLMemFromDma: size = %zu,flag = %lu\n", size_, flag_);
            cl_buf_ = createCLMemFromDma(ctx,
                                         dev,
                                         flag_,
                                         fd_,
                                         size_);
        }
        else if(mem_type_ == kAlloc) {
            LOGI("clCreateBuffer: size = %zu,flag = %lu\n", size_, flag_);
            cl_buf_ = clCreateBuffer(ctx, flag_, size_, nullptr, &err_code);
        }
        else {
            LOGE("Unsupport memory type:%d\n", mem_type_);
            return -1;
        }
        return 0;
    }

    int BufferDescriptor::Destroy() {
        auto ret = -1;
        if(cl_buf_ != nullptr) {
            ret = clReleaseMemObject(cl_buf_);
            OPENCL_CALL(ret);
            cl_buf_ = nullptr;
        }

        return ret;
    }

    int ImageDescriptor::Create(cl_context ctx, cl_device_id dev) {
        cl_int err_code = 0;
        if (cl_buf_ != nullptr)
            OPENCL_CALL(clReleaseMemObject(cl_buf_));
        if(mem_type_ == kAlloc) {
            LOGI("clCreateImage: image_type = 0x%x, height = %d, width = %d, arr_size = %d,flag = %lu, data_type = 0x%x, order = 0x%x\n", 
                image_desc_.image_type, height_, width_, img_array_size_, flag_, format_.image_channel_data_type, format_.image_channel_order);
            // cl_buf_ = clCreateImage2D(ctx, flag_, &format_, width_, height_, 0, nullptr, &err_code);
            cl_buf_ = clCreateImage(ctx, flag_, &format_, &image_desc_, nullptr, &err_code);
        }
        else {
            LOGE("Unsupport memory type:%d\n", mem_type_);
            return -1;
        }
        if(cl_buf_ == nullptr) {
            LOGE("clCreateImage failed, error code = %s\n",errorNumberToString(err_code).c_str());
            cl_image_format image_format[100];
            unsigned int num_format = 0;
            clGetSupportedImageFormats(ctx, flag_, CL_MEM_OBJECT_IMAGE2D, 100, image_format, &num_format);
            LOGI("Support image2d format:\n");
            for (int n = 0; n < num_format; ++n)
            {
                LOGI("[%d]: type = 0x%x, order = 0x%x\n",n, image_format[n].image_channel_data_type, image_format[n].image_channel_order);
            }
            clGetSupportedImageFormats(ctx, flag_, CL_MEM_OBJECT_IMAGE2D_ARRAY, 100, image_format, &num_format);
            LOGI("Support image array format:\n");
            for (int n = 0; n < num_format; ++n)
            {
                LOGI("[%d]: type = 0x%x, order = 0x%x\n",n, image_format[n].image_channel_data_type, image_format[n].image_channel_order);
            }

            clGetSupportedImageFormats(ctx, flag_, CL_MEM_OBJECT_IMAGE3D, 100, image_format, &num_format);
            LOGI("Support image3d format:\n");
            for (int n = 0; n < num_format; ++n)
            {
                LOGI("[%d]: type = 0x%x, order = 0x%x\n",n, image_format[n].image_channel_data_type, image_format[n].image_channel_order);
            }
            abort();
        }
        return 0;
    }

    int ImageDescriptor::Destroy() {
        if(cl_buf_ != nullptr) {
            OPENCL_CALL(clReleaseMemObject(cl_buf_));
            cl_buf_ = nullptr;
        }
        return 0;
    }

}