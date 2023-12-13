/*
 * @Author: Chifred
 * @Date: 2021-09-27 17:28:42
 * @LastEditTime: 2021-12-24 14:52:42
 * @Editors: Chifred
 * @Description: TODO
 */
#ifndef __OPENCL_MEMORY_H__
#define __OPENCL_MEMORY_H__
#include <set>
#include "opencl_wrapper.h"

namespace gpu_postprocess
{
    #define CHAN_NUM 3
    enum MemFetchType : int
    {
        kAlloc = 0,
        kMap,
        kImport,
    };
    
    class BufferDescriptor
    {
    public:
        BufferDescriptor(int fd, MemFetchType type,cl_mem_flags flag, int offset, size_t size, std::string name) : 
                        fd_(fd), mem_type_(type),flag_(flag), offset_(offset), size_(size), name_(name) { 
            cl_buf_ = nullptr; 
        }
        virtual ~BufferDescriptor(){};
        virtual int Create(cl_context ctx, cl_device_id dev);
        virtual int Destroy();

        int fd_;
        int offset_;
        size_t size_;
        MemFetchType mem_type_;
        cl_mem_flags flag_;
        cl_mem cl_buf_;
        std::string name_;
    };
    

    class ImageDescriptor
    {
    public:
        ImageDescriptor(MemFetchType type, int height, int width, cl_mem_flags flag, const cl_image_format* format, std::string name,
                        int img_array_size = 1) : 
                        mem_type_(type), flag_(flag), height_(height), width_(width), name_(name), img_array_size_(img_array_size) { 
            cl_buf_ = nullptr;
            format_.image_channel_data_type = format->image_channel_data_type;
            format_.image_channel_order = format->image_channel_order;

            printf("ImageDescriptor img_array_size = %d\n",img_array_size);
            //cl image desc
            if(img_array_size_ > 1) {
                image_desc_.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
            } else {
                image_desc_.image_type = CL_MEM_OBJECT_IMAGE2D;
            }
            image_desc_.image_width = width;
            image_desc_.image_height = height;
            image_desc_.image_array_size = img_array_size;
            image_desc_.image_row_pitch = 0;
            image_desc_.image_slice_pitch = 0;
            image_desc_.num_mip_levels = 0;
            image_desc_.num_samples = 0;
            image_desc_.buffer = NULL;
        }
        virtual ~ImageDescriptor(){};
        virtual int Create(cl_context ctx, cl_device_id dev);
        virtual int Destroy();

        int fd_;
        int offset_;
        int height_;
        int width_;
        int img_array_size_;
        MemFetchType mem_type_;
        cl_image_format format_;
        cl_image_desc image_desc_;
        cl_mem_flags flag_;
        cl_mem cl_buf_;
        std::string name_;
    };

} //namespace  gpu_postprocess
#endif //__OPENCL_MEMORY_H__