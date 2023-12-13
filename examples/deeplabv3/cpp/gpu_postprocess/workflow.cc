/*
 * @Author: Chifred
 * @Date: 2021-11-17 10:55:32
 * @LastEditTime: 2021-12-27 09:52:12
 * @Editors: Chifred
 * @Description: TODO
 */

#include "workflow.h"
#include <set>
#include <stdlib.h>

namespace gpu_postprocess
{
    void Workflow::Install(std::shared_ptr<OpenCLWorkspace>& ocl_workspace, std::string cl_path) {
    //ocl_workspace->InstallKernel(cl_path,kernel_name_);
      const char * p_src =  cl_path.c_str();
      ocl_workspace->InstallKernel(&p_src, nullptr, kernel_name_);
    }

    void Workflow::InitGPUWorkSize(std::vector<size_t>& work_size) {
        for (int i = 0; i < work_size.size();++i) {
            global_ws_[i] = work_size[i];//{width, height}
        }
        global_ws_len_ = work_size.size();

    }

    cl_image_format Workflow::GetClImgFmt(img_format tex_format) {
                //TODO:
        cl_image_format format = {CL_RGBA, CL_FLOAT}; 
        switch (tex_format)
        {
            case img_format::RGBA_FLOAT_ARRAY:
            case img_format::RGBA_FLOAT:
                format.image_channel_data_type = CL_FLOAT;
                format.image_channel_order = CL_RGBA;
                break;
            case img_format::RGB_UINT8:
                format.image_channel_data_type = CL_UNORM_INT8;
                format.image_channel_order = CL_RGB;
                break;
            case img_format::RGBA_UINT8:
                format.image_channel_data_type = CL_UNSIGNED_INT8;
                format.image_channel_order = CL_RGBA;
                break;
            case img_format::RGB_FLOAT_ARRAY:
                format.image_channel_data_type = CL_FLOAT;
                format.image_channel_order = CL_RGB;
                break;
            case img_format::R_FLOAT_ARRAY:
                format.image_channel_data_type = CL_FLOAT;
                format.image_channel_order = CL_R;
                break;
            default:
                LOGE("not support texture format: %d\n",tex_format);
                abort();
                break;
        }
        return format;
    }

    void Workflow::Allocate(std::shared_ptr<OpenCLWorkspace>& ocl_workspace) {
        std::unordered_map<std::string,size_t> &in_maps = mem_descs_->inBufSizeMaps_;
        std::unordered_map<std::string,size_t> &out_maps = mem_descs_->outBufSizeMaps;
        for (auto& in: in_maps) {
            ocl_workspace->AllocDataSpace(in.first, -1, 0, in.second, CL_MEM_READ_ONLY, kAlloc);
        }
        for (auto& out: out_maps) {
            ocl_workspace->AllocDataSpace(out.first, -1, 0, out.second, CL_MEM_WRITE_ONLY, kAlloc);
        }

        std::unordered_map<std::string, Resolution> &in_tex_maps = mem_descs_->inTextureMaps_;
        std::unordered_map<std::string, Resolution> &out_tex_maps = mem_descs_->outTextureMaps_;

        std::unordered_map<std::string, img_format> &in_img_fmt_maps = mem_descs_->in_tex_fmt_maps_;
        std::unordered_map<std::string, img_format> &out_img_fmt_maps = mem_descs_->out_tex_fmt_maps_;

        std::set<std::string> in_out_tex_names;
        //read_write texture
        for(auto& in: in_tex_maps) {
            if( (in.first != "") && (out_tex_maps.end() != out_tex_maps.find(in.first))) {
                in_out_tex_names.insert(in.first);
            }
        }
        
        for(auto io: in_out_tex_names) {
            auto io_res = in_tex_maps.at(io);
            img_format io_img_fmt = in_img_fmt_maps[io];
            cl_image_format format = GetClImgFmt(io_img_fmt);
            ocl_workspace->AllocImageTexture(io, CL_MEM_READ_WRITE, &format, io_res.height, io_res.width, kAlloc, io_res.img_array_size_);
        }

        for(auto& in: in_tex_maps) {
            if( (in.first == "") && (in_out_tex_names.find(in.first) != in_out_tex_names.end())) {
                continue;
            }
            auto search = in_img_fmt_maps.find(in.first);
            if(search != in_img_fmt_maps.end()) {
                cl_image_format format = GetClImgFmt(search->second);
                ocl_workspace->AllocImageTexture(in.first, CL_MEM_READ_ONLY, &format, in.second.height, in.second.width, kAlloc, in.second.img_array_size_);
            } else {
                LOGE("can not find input tex name: %s\n",in.first.c_str());
                abort();
            }
        }
        for(auto& out: out_tex_maps) {
            if(out.first == "" && in_out_tex_names.find(out.first) != in_out_tex_names.end()) {
                continue;
            }
            auto search = out_img_fmt_maps.find(out.first);
            if(search != out_img_fmt_maps.end()) {
                cl_image_format format = GetClImgFmt(search->second);
                ocl_workspace->AllocImageTexture(out.first, CL_MEM_WRITE_ONLY, &format, out.second.height, out.second.width, kAlloc, out.second.img_array_size_);
            } else {
                LOGE("can not find output tex name: %s\n",out.first.c_str());
                abort();
            }
        }
    }

    void Workflow::Import(std::shared_ptr<OpenCLWorkspace>& ocl_workspace) {
        std::unordered_map<std::string,size_t> &in_maps = mem_descs_->inBufSizeMaps_;
        std::unordered_map<std::string,size_t> &out_maps = mem_descs_->outBufSizeMaps;
        std::unordered_map<std::string, int> &in_fd_maps = mem_descs_->inBufFdMaps_;
        std::unordered_map<std::string, int> &out_fd_maps = mem_descs_->outBufFdMaps_;
        
        for (auto& in: in_maps) {
            auto fd_search = in_fd_maps.find(in.first);
            if(fd_search != in_fd_maps.end()) {
                ocl_workspace->AllocDataSpace(in.first, fd_search->second , 0, in.second, CL_MEM_READ_ONLY, kImport);
            } else {
                LOGE("can not find input fd name: %s\n",in.first.c_str());
                abort();
            }
        }
        for (auto& out: out_maps) {
            auto fd_search = out_fd_maps.find(out.first);
            if(fd_search != out_fd_maps.end()) {
                ocl_workspace->AllocDataSpace(out.first, fd_search->second, 0, out.second, CL_MEM_WRITE_ONLY, kImport);
            } else {
                LOGE("can not find output fd name: %s\n",out.first.c_str());
                abort();
            }
        }

        std::unordered_map<std::string, Resolution> &in_tex_maps = mem_descs_->inTextureMaps_;
        std::unordered_map<std::string, Resolution> &out_tex_maps = mem_descs_->outTextureMaps_;

        std::unordered_map<std::string, img_format> &in_img_fmt_maps = mem_descs_->in_tex_fmt_maps_;
        std::unordered_map<std::string, img_format> &out_img_fmt_maps = mem_descs_->out_tex_fmt_maps_;

        std::set<std::string> in_out_tex_names;
        //read_write texture
        for(auto& in: in_tex_maps) {
            if( (in.first != "") && (out_tex_maps.end() != out_tex_maps.find(in.first))) {
                in_out_tex_names.insert(in.first);
            }
        }
        
        for(auto io: in_out_tex_names) {
            auto io_res = in_tex_maps.at(io);
            img_format io_img_fmt = in_img_fmt_maps[io];
            cl_image_format format = GetClImgFmt(io_img_fmt);
            ocl_workspace->AllocImageTexture(io, CL_MEM_READ_WRITE, &format, io_res.height, io_res.width, kAlloc, io_res.img_array_size_);
        }

        for(auto& in: in_tex_maps) {
            if( (in.first == "") && (in_out_tex_names.find(in.first) != in_out_tex_names.end())) {
                continue;
            }
            auto search = in_img_fmt_maps.find(in.first);
            if(search != in_img_fmt_maps.end()) {
                cl_image_format format = GetClImgFmt(search->second);
                ocl_workspace->AllocImageTexture(in.first, CL_MEM_READ_ONLY, &format, in.second.height, in.second.width, kAlloc, in.second.img_array_size_);
            } else {
                LOGE("can not find input tex name: %s\n",in.first.c_str());
                abort();
            }
        }
        for(auto& out: out_tex_maps) {
            if(out.first == "" && in_out_tex_names.find(out.first) != in_out_tex_names.end()) {
                continue;
            }
            auto search = out_img_fmt_maps.find(out.first);
            if(search != out_img_fmt_maps.end()) {
                cl_image_format format = GetClImgFmt(search->second);
                ocl_workspace->AllocImageTexture(out.first, CL_MEM_WRITE_ONLY, &format, out.second.height, out.second.width, kAlloc, out.second.img_array_size_);
            } else {
                LOGE("can not find output tex name: %s\n",out.first.c_str());
                abort();
            }
        }
    }
    
    void Workflow::SetInputs(std::shared_ptr<OpenCLWorkspace>& ocl_workspace, std::string name,void *data) {
        std::unordered_map<std::string, Resolution> &in_tex_maps = mem_descs_->inTextureMaps_;
        for(auto& in: in_tex_maps) {
            if(in.first == name) {
                ocl_workspace->SetImageTexture(name, data, in.second.height, in.second.width);
            }
        }
        
        std::unordered_map<std::string,size_t> &in_maps = mem_descs_->inBufSizeMaps_;
        for (auto& in: in_maps) {
            if(in.first == name) {
                ocl_workspace->SetDataSpace(name, -1, data, in.second);
            }
        }
    }
    
    void Workflow::GetOutputs(std::shared_ptr<OpenCLWorkspace>& ocl_workspace, std::string name,void *data) {
        std::unordered_map<std::string, Resolution> &out_tex_maps = mem_descs_->outTextureMaps_;
        for(auto& out: out_tex_maps) {
            if(out.first == name) {
                ocl_workspace->GetImageTexture(name, data, out.second.height, out.second.width);
            }
        }
        
        std::unordered_map<std::string,size_t> &out_maps = mem_descs_->outBufSizeMaps;
        for (auto& out: out_maps) {
            if(out.first == name) {
                ocl_workspace->GetDataSpace(name, -1, data, out.second);
            }
        }
    }
}