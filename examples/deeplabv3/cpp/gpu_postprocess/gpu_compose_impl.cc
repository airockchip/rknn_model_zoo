/*
 * @Author: Chifred
 * @Date: 2021-10-26 10:36:21
 * @LastEditTime: 2021-12-27 10:00:33
 * @Editors: Chifred
 * @Description: TODO
 */
#include "gpu_compose_impl.h"

namespace gpu_postprocess
{
    gpu_compose_impl::gpu_compose_impl()
    {
        workspace_ = std::make_shared<OpenCLWorkspace>();
        workspace_->Init();
    }
    gpu_compose_impl::~gpu_compose_impl()
    {
        workspace_->FreeDataSpace();
        workspace_->FreeImageTexture();
        workspace_.reset();
    }

    void gpu_compose_impl::addWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names,
                                       std::vector<size_t> in_buf_size, std::vector<std::string> out_names, std::vector<size_t> out_size,
                                       std::vector<size_t> work_size,
                                       std::string in_tex_name, size_t in_h, size_t in_w,
                                       std::string out_tex_name, size_t out_h, size_t out_w, img_format format)
    {
        auto wf = std::make_shared<Workflow>(kernel_name, in_buf_names, in_buf_size, out_names, out_size, work_size,
                                             in_tex_name, in_h, in_w, out_tex_name, out_h, out_w, format);
        wf->Install(workspace_, cl_path);
        wf->Allocate(workspace_);
        workflows_.insert(std::make_pair(kernel_name, wf));
    }
    
    void gpu_compose_impl::addWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names,
                                       std::vector<size_t> in_buf_size, std::vector<std::string> out_names, std::vector<size_t> out_size,
                                       std::vector<size_t> work_size,
                                       std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions, 
                                       std::vector<img_format> in_formats,
                                       std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions, 
                                       std::vector<img_format> out_formats)
    {
        auto wf = std::make_shared<Workflow>(kernel_name, in_buf_names, in_buf_size, out_names, out_size, work_size,
                                             in_tex_names, in_resolutions, in_formats, out_tex_names, out_resolutions, out_formats);
        wf->Install(workspace_, cl_path);
        wf->Allocate(workspace_);
        workflows_.insert(std::make_pair(kernel_name, wf));
    }

    void gpu_compose_impl::addZeroCopyWorkflow(std::string cl_path, std::string kernel_name, std::vector<std::string> in_buf_names,
                                       std::vector<size_t> in_buf_size, std::vector<int> in_buf_fd,
                                       std::vector<std::string> out_names, std::vector<size_t> out_size, std::vector<int> out_buf_fd,
                                       std::vector<size_t> work_size,
                                       std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions, 
                                       std::vector<img_format> in_formats,
                                       std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions, 
                                       std::vector<img_format> out_formats)
    {
        auto wf = std::make_shared<Workflow>(kernel_name, in_buf_names, in_buf_size, in_buf_fd, out_names, out_size, out_buf_fd, work_size,
                                             in_tex_names, in_resolutions, in_formats, out_tex_names, out_resolutions, out_formats);
        wf->Install(workspace_, cl_path);
        wf->Import(workspace_);
        workflows_.insert(std::make_pair(kernel_name, wf));
    }

#if 0
    int gpu_compose_impl::UpsampleSoftmax(std::string kernel_name,
                                std::string src_name, char *src_img_ptr, std::string res_name, unsigned char *res, 
                                const int srcHeight, const int srcWidth,
                                const int dstHeight, const int dstWidth, const int dstChannel, 
                                const float scale_h_inv, const float scale_w_inv, const int img_arr_size) {
        if (workspace_->kernel_maps.find(kernel_name) == workspace_->kernel_maps.end())
        {
            LOGE("kernel: '%s' kernel not found\n", kernel_name.c_str());
            return -1;
        }
        cl_kernel &kernel = workspace_->kernel_maps[kernel_name];
        if (workflows_.find(kernel_name) == workflows_.end())
        {
            LOGE("kernel: '%s' workflow not found\n", kernel_name.c_str());
            return -1;
        }
        std::shared_ptr<Workflow> &wf = workflows_[kernel_name];
        wf->SetInputs(workspace_, src_name, (void *)src_img_ptr);
        // std::shared_ptr<ImageDescriptor> src_desc = workspace_->GetImageDesc(src_name);
        std::shared_ptr<BufferDescriptor> src_desc = workspace_->GetDataDesc(src_name);
        std::shared_ptr<BufferDescriptor> out_desc = workspace_->GetDataDesc(res_name);
        cl_mem src_mem = src_desc->cl_buf_;
        cl_mem out_mem = out_desc->cl_buf_;
        
        int arg_idx = 0;
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &src_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &out_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &srcHeight));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &srcWidth));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstHeight));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstWidth));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstChannel));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(float), &scale_h_inv));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(float), &scale_w_inv));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &img_arr_size));


        LOGI("dst_height = %d, dst_width = %d, dstChannel = %d, srcHeight = %d, srcWidth = %d, scale_h_inv = %f, scale_w_inv = %f, wf->global_ws_len_ = %d,  wf->global_ws_[0] = %d,  wf->global_ws_[1] = %d\n",
                dstHeight, dstWidth, dstChannel, srcHeight, srcWidth, scale_h_inv, scale_w_inv, wf->global_ws_len_,  wf->global_ws_[0],  wf->global_ws_[1]);

        // launch kernel
        OPENCL_CALL(clEnqueueNDRangeKernel(workspace_->queue, kernel, wf->global_ws_len_, nullptr, wf->global_ws_, nullptr,
                                           0, nullptr, nullptr));
        OPENCL_CALL(clFinish(workspace_->queue));
        wf->GetOutputs(workspace_, res_name, res);
        return 0;
    }
#endif

    int gpu_compose_impl::UpsampleSoftmax(std::string kernel_name,
                                std::string src_name, float *src_img_ptr, std::string res_name, unsigned char *res, 
                                const int srcHeight, const int srcWidth,
                                const int dstHeight, const int dstWidth, const int dstChannel, 
                                const float scale_h_inv, const float scale_w_inv, const int src_stride) {
        if (workspace_->kernel_maps.find(kernel_name) == workspace_->kernel_maps.end())
        {
            LOGE("kernel: '%s' kernel not found\n", kernel_name.c_str());
            return -1;
        }
        cl_kernel &kernel = workspace_->kernel_maps[kernel_name];
        if (workflows_.find(kernel_name) == workflows_.end())
        {
            LOGE("kernel: '%s' workflow not found\n", kernel_name.c_str());
            return -1;
        }
        std::shared_ptr<Workflow> &wf = workflows_[kernel_name];
        if(src_img_ptr != nullptr)
            wf->SetInputs(workspace_, src_name, (void *)src_img_ptr);
        // std::shared_ptr<ImageDescriptor> src_desc = workspace_->GetImageDesc(src_name);
        std::shared_ptr<BufferDescriptor> src_desc = workspace_->GetDataDesc(src_name);
        std::shared_ptr<BufferDescriptor> out_desc = workspace_->GetDataDesc(res_name);
        cl_mem src_mem = src_desc->cl_buf_;
        cl_mem out_mem = out_desc->cl_buf_;
        
        int arg_idx = 0;
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &src_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &out_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &srcHeight));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &srcWidth));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstHeight));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstWidth));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstChannel));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(float), &scale_h_inv));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(float), &scale_w_inv));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &src_stride));


        // LOGI("dst_height = %d, dst_width = %d, dstChannel = %d, srcHeight = %d, srcWidth = %d, scale_h_inv = %f, scale_w_inv = %f, wf->global_ws_len_ = %d,  wf->global_ws_[0] = %d,  wf->global_ws_[1] = %d\n",
        //         dstHeight, dstWidth, dstChannel, srcHeight, srcWidth, scale_h_inv, scale_w_inv, wf->global_ws_len_,  wf->global_ws_[0],  wf->global_ws_[1]);

        // launch kernel
        OPENCL_CALL(clEnqueueNDRangeKernel(workspace_->queue, kernel, wf->global_ws_len_, nullptr, wf->global_ws_, nullptr,
                                           0, nullptr, nullptr));
        OPENCL_CALL(clFinish(workspace_->queue));
        if(res != nullptr)
            wf->GetOutputs(workspace_, res_name, res);
        return 0;
    }

    int gpu_compose_impl::UpsampleSoftmaxImage2D(std::string kernel_name,
                                std::string src_name, float *src_img_ptr, std::string res_name, unsigned char *res, 
                                const int dstHeight, const int dstWidth, const int dstChannel, const int img_array_size) {
        if (workspace_->kernel_maps.find(kernel_name) == workspace_->kernel_maps.end())
        {
            LOGE("kernel: '%s' kernel not found\n", kernel_name.c_str());
            return -1;
        }
        cl_kernel &kernel = workspace_->kernel_maps[kernel_name];
        if (workflows_.find(kernel_name) == workflows_.end())
        {
            LOGE("kernel: '%s' workflow not found\n", kernel_name.c_str());
            return -1;
        }
        std::shared_ptr<Workflow> &wf = workflows_[kernel_name];
        wf->SetInputs(workspace_, src_name, (void *)src_img_ptr);
        std::shared_ptr<ImageDescriptor> src_desc = workspace_->GetImageDesc(src_name);
        std::shared_ptr<BufferDescriptor> out_desc = workspace_->GetDataDesc(res_name);
        cl_mem src_mem = src_desc->cl_buf_;
        cl_mem out_mem = out_desc->cl_buf_;
        
        int arg_idx = 0;
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &src_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &out_mem));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstHeight));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstWidth));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &dstChannel));
        OPENCL_CALL(clSetKernelArg(kernel, arg_idx++, sizeof(int), &img_array_size));


        // LOGI("dst_height = %d, dst_width = %d, dstChannel = %d, img_array_size = %d, wf->global_ws_len_ = %d,  wf->global_ws_[0] = %d,  wf->global_ws_[1] = %d\n",
        //         dstHeight, dstWidth, dstChannel, img_array_size, wf->global_ws_len_,  wf->global_ws_[0],  wf->global_ws_[1]);

        // launch kernel
        OPENCL_CALL(clEnqueueNDRangeKernel(workspace_->queue, kernel, wf->global_ws_len_, nullptr, wf->global_ws_, nullptr,
                                           0, nullptr, nullptr));
        OPENCL_CALL(clFinish(workspace_->queue));
        wf->GetOutputs(workspace_, res_name, res);
        return 0;
    }

}