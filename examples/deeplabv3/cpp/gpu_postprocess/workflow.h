/*
 * @Author: Chifred
 * @Date: 2021-11-17 09:59:55
 * @LastEditTime: 2023-11-29 16:20:41
 * @Editors: Chifred
 * @Description: TODO
 */
#ifndef __GPU_COMPOSE_WORKFLOW_H__
#define __GPU_COMPOSE_WORKFLOW_H__
#include <string>
#include <vector>
#include <unordered_map>
#include <string.h>
#include "opencl_workspace.h"

namespace gpu_postprocess
{

enum class img_format {
    RGBA_UINT8 = 0,
    RGBA_FLOAT = 1,
    RGB_UINT8 = 2,
    R_FLOAT_ARRAY = 3,
    RGB_FLOAT_ARRAY = 4,
    RGBA_FLOAT_ARRAY = 5,
};

struct Resolution
{
    size_t height;
    size_t width;
    size_t img_array_size_;
    Resolution(size_t h, size_t w, size_t array_size = 1) : height(h), width(w), img_array_size_(array_size) {}
};

class MemDescriptor {
public:
    MemDescriptor(std::vector<std::string>& in_buf_names,
                  std::vector<size_t>& in_buf_size, std::vector<std::string>& out_buf_names, std::vector<size_t>& out_buf_size,
                  std::vector<std::string>& in_tex_names, std::vector<Resolution>& in_resolutions,
                  std::vector<img_format>& in_formats,
                  std::vector<std::string>& out_tex_names, std::vector<Resolution>& out_resolutions,
                  std::vector<img_format>& out_formats)
    {
        // cl buffer object
        for (int i = 0; i < in_buf_names.size();++i)
        {
            inBufSizeMaps_.insert(std::make_pair(in_buf_names[i], in_buf_size[i]));
        }
        for (int i = 0; i < out_buf_names.size();++i)
        {
            outBufSizeMaps.insert(std::make_pair(out_buf_names[i], out_buf_size[i]));
        }
        //cl image2d object
        for (int i = 0; i < in_tex_names.size();++i)
        {
            LOGI("[%d]img_array_size_ = %zu\n",i,in_resolutions[i].img_array_size_);
            inTextureMaps_.insert(std::make_pair(in_tex_names[i], in_resolutions[i]));
            in_tex_fmt_maps_.insert(std::make_pair(in_tex_names[i], in_formats[i]));
        }
        for (int i = 0; i < out_tex_names.size();++i)
        {
            outTextureMaps_.insert(std::make_pair(out_tex_names[i], out_resolutions[i]));
            out_tex_fmt_maps_.insert(std::make_pair(out_tex_names[i], out_formats[i]));
        }
    }


    MemDescriptor(std::vector<std::string>& in_buf_names, std::vector<size_t>& in_buf_size, std::vector<int>& in_buf_fd, 
                  std::vector<std::string>& out_buf_names, std::vector<size_t>& out_buf_size, std::vector<int>& out_buf_fd, 
                  std::vector<std::string>& in_tex_names, std::vector<Resolution>& in_resolutions,
                  std::vector<img_format>& in_formats,
                  std::vector<std::string>& out_tex_names, std::vector<Resolution>& out_resolutions,
                  std::vector<img_format>& out_formats)
    {
        // cl buffer object
        for (int i = 0; i < in_buf_names.size();++i)
        {
            inBufSizeMaps_.insert(std::make_pair(in_buf_names[i], in_buf_size[i]));
            inBufFdMaps_.insert(std::make_pair(in_buf_names[i], in_buf_fd[i]));
        }
        for (int i = 0; i < out_buf_names.size();++i)
        {
            outBufSizeMaps.insert(std::make_pair(out_buf_names[i], out_buf_size[i]));
            outBufFdMaps_.insert(std::make_pair(out_buf_names[i], out_buf_fd[i]));
        }
        //cl image2d object
        for (int i = 0; i < in_tex_names.size();++i)
        {
            LOGI("[%d]img_array_size_ = %zu\n",i,in_resolutions[i].img_array_size_);
            inTextureMaps_.insert(std::make_pair(in_tex_names[i], in_resolutions[i]));
            in_tex_fmt_maps_.insert(std::make_pair(in_tex_names[i], in_formats[i]));
        }
        for (int i = 0; i < out_tex_names.size();++i)
        {
            outTextureMaps_.insert(std::make_pair(out_tex_names[i], out_resolutions[i]));
            out_tex_fmt_maps_.insert(std::make_pair(out_tex_names[i], out_formats[i]));
        }
    }

public:
    std::unordered_map<std::string, int> inBufFdMaps_;
    std::unordered_map<std::string, int> outBufFdMaps_;
    std::unordered_map<std::string,size_t> inBufSizeMaps_;
    std::unordered_map<std::string,size_t> outBufSizeMaps;
    std::unordered_map<std::string,Resolution> inTextureMaps_;
    std::unordered_map<std::string,Resolution> outTextureMaps_;
    std::unordered_map<std::string, img_format> in_tex_fmt_maps_;
    std::unordered_map<std::string, img_format> out_tex_fmt_maps_;
};

class Workflow {
public:
    Workflow(std::string kname,std::vector<std::string> in_names, std::vector<size_t> in_size, std::vector<std::string> out_names, 
                std::vector<size_t> out_size, std::vector<size_t> work_size, std::string in_tex_name, size_t in_h, size_t in_w, 
                std::string out_tex_name, size_t out_h, size_t out_w ,img_format format):
                kernel_name_(kname) 
    {
        InitGPUWorkSize(work_size);
        Resolution in_res{in_h, in_w};
        Resolution out_res{out_h, out_w};
        std::vector<std::string> in_tex_names{in_tex_name};
        std::vector<std::string> out_tex_names{out_tex_name};
        std::vector<Resolution> in_resolutions = {in_res};
        std::vector<Resolution> out_resolutions = {out_res};
        std::vector<img_format> io_formats = {format};
        mem_descs_ = std::make_shared<MemDescriptor>(in_names, in_size, out_names, out_size,
                                                     in_tex_names, in_resolutions, io_formats, out_tex_names, out_resolutions, io_formats);
    }
    
    Workflow(std::string kname, std::vector<std::string> in_buf_names,
                                       std::vector<size_t> in_buf_size, std::vector<std::string> out_buf_names, std::vector<size_t> out_buf_size,
                                       std::vector<size_t> work_size,
                                       std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions, 
                                       std::vector<img_format> in_formats,
                                       std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions, 
                                       std::vector<img_format> out_formats):
    kernel_name_(kname) 
    {
        InitGPUWorkSize(work_size);
        mem_descs_ = std::make_shared<MemDescriptor>(in_buf_names, in_buf_size, out_buf_names, out_buf_size,
                                                     in_tex_names, in_resolutions, in_formats, out_tex_names, out_resolutions, out_formats);
    }
    
    Workflow(std::string kname, std::vector<std::string> in_buf_names, std::vector<size_t> in_buf_size,  std::vector<int> in_buf_fd,
                                       std::vector<std::string> out_buf_names, std::vector<size_t> out_buf_size,  std::vector<int> out_buf_fd,
                                       std::vector<size_t> work_size,
                                       std::vector<std::string> in_tex_names, std::vector<Resolution> in_resolutions, 
                                       std::vector<img_format> in_formats,
                                       std::vector<std::string> out_tex_names, std::vector<Resolution> out_resolutions, 
                                       std::vector<img_format> out_formats):
    kernel_name_(kname) 
    {
        InitGPUWorkSize(work_size);
        mem_descs_ = std::make_shared<MemDescriptor>(in_buf_names, in_buf_size, in_buf_fd, out_buf_names, out_buf_size, out_buf_fd,
                                                     in_tex_names, in_resolutions, in_formats, out_tex_names, out_resolutions, out_formats);
    }
    ~Workflow() {
        mem_descs_.reset();
    }

    void Install(std::shared_ptr<OpenCLWorkspace>& ocl_workspace, std::string cl_path);
    void Allocate(std::shared_ptr<OpenCLWorkspace>& ocl_workspace);
    void Import(std::shared_ptr<OpenCLWorkspace> &ocl_workspace);
    void SetInputs(std::shared_ptr<OpenCLWorkspace> &ocl_workspace, std::string name, void *data);
    void GetOutputs(std::shared_ptr<OpenCLWorkspace>& ocl_workspace, std::string name, void *data);

public:
    size_t global_ws_[3];
    size_t global_ws_len_;

    std::string kernel_name_;
    std::shared_ptr<MemDescriptor> mem_descs_;

private:
    void InitGPUWorkSize(std::vector<size_t> &work_size);

    cl_image_format GetClImgFmt(img_format tex_format);
};
}
#endif
