/*
 * @Author: Chifred
 * @Date: 2021-10-26 10:36:42
 * @LastEditTime: 2021-12-27 10:00:01
 * @Editors: Chifred
 * @Description: TODO
 */

#ifndef __JNI_GPU_COMPOSE_H__
#define __JNI_GPU_COMPOSE_H__

#include <memory>
#include "opencl_wrapper.h"
#include "workflow.h"

namespace gpu_postprocess
{
#define COMPOSE_CHECK_ERROR(e)                        \
    {                                                 \
        if (e != 0)                                   \
        {                                             \
            LOGE("GPU Compose Error, code= %d\n", e); \
            abort();                                  \
        }                                             \
    }
    
    class gpu_compose_impl
    {
    public:
        gpu_compose_impl();
        virtual ~gpu_compose_impl();
        void addWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names, std::vector<size_t> in_buf_size,
                    std::vector<std::string> out_names, std::vector<size_t> out_size, std::vector<size_t> work_size,
                    std::string in_tex_name, size_t in_h, size_t in_w,
                    std::string out_tex_name, size_t out_h, size_t out_w, img_format format);

        void addWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names,
                                           std::vector<size_t> in_buf_size, std::vector<std::string> out_names, std::vector<size_t> out_size,
                                           std::vector<size_t> work_size,
                                           std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions,
                                           std::vector<img_format> in_formats,
                                           std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions,
                                           std::vector<img_format> out_formats);
        void addZeroCopyWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names,
                                 std::vector<size_t> in_buf_size, std::vector<int> in_buf_fd,
                                 std::vector<std::string> out_names, std::vector<size_t> out_size, std::vector<int> out_buf_fd,
                                 std::vector<size_t> work_size,
                                 std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions,
                                 std::vector<img_format> in_formats,
                                 std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions,
                                 std::vector<img_format> out_formats);

#if 0
        int UpsampleSoftmax(std::string kernel_name,
                        std::string src_name, char *src_img_ptr, std::string res_name, unsigned char *res,
                        const int srcHeight, const int srcWidth,
                        const int dstHeight, const int dstWidth, const int dstChannel,
                        const float scale_h_inv, const float scale_w_inv, const int img_arr_size);
#endif
        int UpsampleSoftmax(std::string kernel_name,
                            std::string src_name, float *src_img_ptr, std::string res_name, unsigned char *res,
                            const int srcHeight, const int srcWidth,
                            const int dstHeight, const int dstWidth, const int dstChannel,
                            const float scale_h_inv, const float scale_w_inv, const int src_stride);
        int UpsampleSoftmaxImage2D(std::string kernel_name,
                                   std::string src_name, float *src_img_ptr, std::string res_name, unsigned char *res,
                                   const int dstHeight, const int dstWidth, const int dstChannel, const int img_array_size);

    private:
        std::shared_ptr<OpenCLWorkspace> workspace_;
        std::unordered_map<std::string, std::shared_ptr<Workflow> > workflows_;
    };
} // namespace gpu_postprocess

#endif