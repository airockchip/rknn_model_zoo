/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file opencl_workspace.cc
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <memory>
#include <string.h>
#include "opencl_memory.h"
#include "opencl_workspace.h"

namespace gpu_postprocess
{
  std::string GetPlatformInfo(cl_platform_id pid, cl_platform_info param_name)
  {
    size_t ret_size;
    OPENCL_CALL(clGetPlatformInfo(pid, param_name, 0, nullptr, &ret_size));
    std::string ret;
    ret.resize(ret_size);
    OPENCL_CALL(clGetPlatformInfo(pid, param_name, ret_size, &ret[0], nullptr));
    return ret;
  }

  std::string GetDeviceInfo(cl_device_id pid, cl_device_info param_name)
  {
    size_t ret_size;
    OPENCL_CALL(clGetDeviceInfo(pid, param_name, 0, nullptr, &ret_size));
    std::string ret;
    ret.resize(ret_size);
    OPENCL_CALL(clGetDeviceInfo(pid, param_name, ret_size, &ret[0], nullptr));
    return ret;
  }

  void OpenCLWorkspace::GetAttr(DeviceAttrKind kind)
  {
    this->Init();
    if (kind == kExist)
    {
      printf("query exits\n");
      return;
    }
    switch (kind)
    {
    case kExist:
      break;
    case kMaxThreadsPerBlock:
    {
      size_t value;
      OPENCL_CALL(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                                  &value, nullptr));
      printf("CL_DEVICE_MAX_WORK_GROUP_SIZE = %zu\n", value);
      break;
    }
    case kWarpSize:
    {
      /* TODO: the warp size of OpenCL device is not always 1
               e.g. Intel Graphics has a sub group concept which contains 8 - 32 work items,
               corresponding to the number of SIMD entries the heardware configures.
               We need to figure out a way to query this information from the hardware.
      */
      break;
    }
    case kMaxSharedMemoryPerBlock:
    {
      cl_ulong value;
      OPENCL_CALL(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong),
                                  &value, nullptr));
      printf("CL_DEVICE_LOCAL_MEM_SIZE = %lu\n", value);
      break;
    }
    case kComputeVersion:
    {
      // String returned is "OpenCL $MAJOR.$MINOR $VENDOR_INFO".  To
      // match other implementations, we want to return "$MAJOR.$MINOR"
      std::string ret = GetDeviceInfo(device, CL_DEVICE_VERSION);

      const size_t version_start = 7; // Length of initial "OpenCL " prefix to skip
      const size_t version_end = ret.find(' ', version_start);
      break;
    }
      return;
    case kDeviceName:
      GetDeviceInfo(device, CL_DEVICE_NAME);
      break;
    case kMaxClockRate:
    {
      cl_uint value;
      OPENCL_CALL(clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint),
                                  &value, nullptr));
      // OpenCL returns the clock rate in MHz, while CUDA/ROCm return the
      // clock rate in kHz.  Converting to the same units for each.
      break;
    }
    case kMultiProcessorCount:
    {
      cl_uint value;
      OPENCL_CALL(clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
                                  &value, nullptr));
      break;
    }
    case kMaxThreadDimensions:
    {
      size_t dims[3];
      OPENCL_CALL(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(dims), dims,
                                  nullptr));

      printf("CL_DEVICE_MAX_WORK_ITEM_SIZES = [%zu, %zu, %zu]\n", dims[0], dims[1], dims[2]);
      break;
    }
    case kMaxRegistersPerBlock:
      return;
    case kGcnArch:
      return;
    case kApiVersion:
    {
      break;
    }
    case kDriverVersion:
    {
      char value[128] = {0};
      OPENCL_CALL(
          clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(value) - 1, value, nullptr));
      break;
    }
    }
  }

  void OpenCLWorkspace::AllocDataSpace(std::string name, int fd, int offset, size_t size, cl_mem_flags flag, MemFetchType mem_type)
  {
    this->Init();
    if (context == nullptr)
    {
      printf("No OpenCL device");
    }
    if (bufDescMaps.find(name) != bufDescMaps.end())
    {
      LOGI("%s memory has been existed, skipped\n", name.c_str());
      return;
    }
    LOGI("allocate cl buffer for: '%s'\n",name.c_str());
    cl_int err_code;
    std::shared_ptr<BufferDescriptor> buf_desc = std::make_shared<BufferDescriptor>(fd, mem_type, flag, offset, size, name);
    buf_desc->Create(context, device);
    bufDescMaps.insert(std::make_pair(name, buf_desc));
    return;
  }

  std::shared_ptr<BufferDescriptor> OpenCLWorkspace::GetDataDesc(std::string name) const{
    if (bufDescMaps.find(name) == bufDescMaps.end())
    {
      LOGE("%s buffer not found\n", name.c_str());
      return nullptr;
    }
    return bufDescMaps.at(name);
  }

  int OpenCLWorkspace::SetDataSpace(std::string name, int fd, void *buf, size_t size)
  {
    cl_int err_code = 0;
    if (bufDescMaps.find(name) == bufDescMaps.end())
    {
      LOGE("%s buffer not found\n", name.c_str());
      return -1;
    }
    std::shared_ptr<BufferDescriptor> &desc = bufDescMaps[name];
    if (desc->flag_ != CL_MEM_READ_ONLY)
    {
      LOGE("%s buffer is not READ ONLY, can not set data\n", name.c_str());
      return -1;
    }
    if (desc->mem_type_ == kAlloc)
    {
      OPENCL_CALL(clEnqueueWriteBuffer(queue, desc->cl_buf_, CL_TRUE, 0, desc->size_, buf, 0, nullptr, nullptr));
    }
    else if (desc->mem_type_ == kMap)
    {
      void *map_ptr = clEnqueueMapBuffer(queue, desc->cl_buf_, CL_TRUE, CL_MAP_WRITE, 0, size, 0, nullptr, nullptr, &err_code);
      memcpy(map_ptr, buf, size);
      OPENCL_CALL(clEnqueueUnmapMemObject(queue, desc->cl_buf_, map_ptr, 0, nullptr, nullptr));
    }
    else
    {
      //TODO: need cache
      if (fd != desc->fd_)
      {
        OPENCL_CALL(clReleaseMemObject(desc->cl_buf_));
        createCLMemFromDma(this->context,
                           this->device,
                           desc->flag_,
                           fd,
                           size);
        desc->fd_ = fd;
      }
    }
    OPENCL_CALL(clFinish(queue));
    return 0;
  }
  
  int OpenCLWorkspace::GetDataSpace(std::string name, int fd, void *buf, size_t size) {
    cl_int err_code = 0;
    if (bufDescMaps.find(name) == bufDescMaps.end())
    {
      LOGE("'%s' buffer not found\n", name.c_str());
      return -1;
    }
    std::shared_ptr<BufferDescriptor> &desc = bufDescMaps[name];
    if (desc->flag_ != CL_MEM_WRITE_ONLY)
    {
      LOGE("%s buffer is not WRITE ONLY, can not set data\n", name.c_str());
      return -1;
    }
    if (desc->mem_type_ == kAlloc)
    {
      OPENCL_CALL(clEnqueueReadBuffer(queue, desc->cl_buf_, CL_TRUE, 0, desc->size_, buf, 0, nullptr, nullptr));
    }
    else if (desc->mem_type_ == kMap)
    {
      void *map_ptr = clEnqueueMapBuffer(queue, desc->cl_buf_, CL_TRUE, CL_MAP_READ, 0, size, 0, nullptr, nullptr, &err_code);
      memcpy(buf, map_ptr, size);
      OPENCL_CALL(clEnqueueUnmapMemObject(queue, desc->cl_buf_, map_ptr, 0, nullptr, nullptr));
    }
    else
    {
      //TODO: need cache
      LOGE("Can not get data from fd\n");
      return -1;
    }
    OPENCL_CALL(clFinish(queue));
    return 0;
  }

  void OpenCLWorkspace::FreeDataSpace()
  {
    // We have to make sure that the memory object is not in the command queue
    // for some OpenCL platforms.
    OPENCL_CALL(clFinish(this->queue));

    for (auto str_desces : bufDescMaps)
    {
      std::shared_ptr<BufferDescriptor> &buf_desces = str_desces.second;
      buf_desces->Destroy();
    }
    bufDescMaps.clear();
  }

  void OpenCLWorkspace::AllocImageTexture(std::string name, cl_mem_flags flag, const cl_image_format* image_format,
              size_t height, size_t width, MemFetchType mem_type, size_t img_array_size) {
    this->Init();
    if (context == nullptr)
    {
      printf("No OpenCL device");
    }
    if (imgDescMaps.find(name) != imgDescMaps.end())
    {
      LOGI("%s texture memory has been existed, skipped\n", name.c_str());
      return;
    }
    LOGI("allocate cl image2d for: '%s'\n",name.c_str());
    cl_int err_code;
    std::shared_ptr<ImageDescriptor> buf_desc = std::make_shared<ImageDescriptor>(mem_type,height,width,flag,image_format,name, img_array_size);
    buf_desc->Create(context, device);
    imgDescMaps.insert(std::make_pair(name, buf_desc));
    return;          
  }


  int OpenCLWorkspace::SetImageTexture(std::string name, void *buf, size_t height, size_t width, size_t img_array_size) {
    cl_int err_code = 0;
    if (imgDescMaps.find(name) == imgDescMaps.end())
    {
      LOGE("%s buffer not found\n", name.c_str());
      return -1;
    }
    std::shared_ptr<ImageDescriptor> &desc = imgDescMaps[name];
    if (desc->flag_ != CL_MEM_READ_ONLY && desc->flag_ != CL_MEM_READ_WRITE)
    {
      LOGE("%s buffer is not READ ONLY, can not set data\n", name.c_str());
      return -1;
    }
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, img_array_size};
    if (desc->mem_type_ == kAlloc)
    {
      OPENCL_CALL(clEnqueueWriteImage(queue, desc->cl_buf_, CL_TRUE, origin, region, 0, 0, buf, 0, nullptr, nullptr));
    }
    else
    {
      //TODO: need cache
      LOGE("%s type is %d, is not supported\n", name.c_str(), desc->mem_type_);
      return -1;
    }
    OPENCL_CALL(clFinish(queue));
    return 0;
  }


  std::shared_ptr<ImageDescriptor> OpenCLWorkspace::GetImageDesc(std::string name) const {

    if (imgDescMaps.find(name) == imgDescMaps.end())
    {
      LOGE("'%s' image2d not found\n", name.c_str());
      return nullptr;
    }
    return imgDescMaps.at(name);
    
  }


  int OpenCLWorkspace::GetImageTexture(std::string name,void *buf, size_t height, size_t width, size_t img_array_size) {
    cl_int err_code = 0;
    if (imgDescMaps.find(name) == imgDescMaps.end())
    {
      LOGE("%s buffer not found\n", name.c_str());
      return -1;
    }
    std::shared_ptr<ImageDescriptor> &desc = imgDescMaps[name];
    if (desc->flag_ != CL_MEM_WRITE_ONLY && desc->flag_ != CL_MEM_READ_WRITE)
    {
      LOGE("%s buffer is not WRITE ONLY, can not set data\n", name.c_str());
      return -1;
    }
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, img_array_size};
    if (desc->mem_type_ == kAlloc)
    {
      OPENCL_CALL(clEnqueueReadImage(queue, desc->cl_buf_, CL_TRUE, origin, region, 0, 0, buf, 0, nullptr, nullptr));
    } 
    else
    {
      //TODO: need cache
      LOGE("Can not get image from fd\n");
      return -1;
    }
    OPENCL_CALL(clFinish(queue));
    return 0;
  }


  void OpenCLWorkspace::FreeImageTexture() {
    OPENCL_CALL(clFinish(this->queue));

    for (auto str_desces : imgDescMaps)
    {
      std::shared_ptr<ImageDescriptor> &img_descs = str_desces.second;
      img_descs->Destroy();
    }
    imgDescMaps.clear();
  }
  std::vector<cl_platform_id> GetPlatformIDs()
  {
    cl_uint ret_size;
    cl_int code = clGetPlatformIDs(0, nullptr, &ret_size);
    std::vector<cl_platform_id> ret;
    if (code != CL_SUCCESS)
      return ret;
    ret.resize(ret_size);
    OPENCL_CALL(clGetPlatformIDs(ret_size, &ret[0], nullptr));
    return ret;
  }

#if 0
  void OpenCLWorkspace::CopyDataFromTo(BufferDescriptor* from, BufferDescriptor* to) {
  size_t nbytes = from.GetDataSize();
  ICHECK_EQ(nbytes, GetDataSize(*to));

  if (IsOpenCLDevice(from->device) && IsOpenCLDevice(to->device)) {
    const auto* from_desc = static_cast<const BufferDescriptor*>(from->data);
        << "Device to device copying is currently only implemented for OpenCL buffer storage";
    auto* to_desc = static_cast<BufferDescriptor*>(to->data);
    OPENCL_CALL(clEnqueueCopyBuffer(this->GetQueue(to->device), from_desc->buffer, to_desc->buffer,
                                    from->byte_offset, to->byte_offset, nbytes, 0, nullptr,
                                    nullptr));
  } else if (IsOpenCLDevice(from->device) && to->device.device_type == kDLCPU) {
    const auto* from_desc = static_cast<const BufferDescriptor*>(from->data);
    switch (from_desc->layout) {
      case MemoryLayout::kBuffer1D:
        OPENCL_CALL(clEnqueueReadBuffer(
            this->GetQueue(from->device), from_desc->buffer, CL_FALSE, from->byte_offset, nbytes,
            static_cast<char*>(to->data) + to->byte_offset, 0, nullptr, nullptr));
        break;
      case MemoryLayout::kImage2DActivation:
      case MemoryLayout::kImage2DWeight:
        auto image_info = GetImageInfo(from_desc, from);
        // TODO(csullivan): Support calculating row_pitch correctly in the case of reuse.
        // Note that when utilizing texture pools for memory reuse, the allocated image
        // size can be larger than the size to be read.
        OPENCL_CALL(clEnqueueReadImage(
            this->GetQueue(from->device), from_desc->buffer, CL_FALSE, image_info.origin,
            image_info.region, image_info.row_pitch, image_info.slice_pitch,
            static_cast<char*>(to->data) + to->byte_offset, 0, nullptr, nullptr));
        break;
    }
    OPENCL_CALL(clFinish(this->GetQueue(from->device)));
  } else if (from->device.device_type == kDLCPU && IsOpenCLDevice(to->device)) {
    auto* to_desc = static_cast<BufferDescriptor*>(to->data);
    switch (to_desc->layout) {
      case MemoryLayout::kBuffer1D:
        OPENCL_CALL(clEnqueueWriteBuffer(
            this->GetQueue(to->device), to_desc->buffer, CL_FALSE, to->byte_offset, nbytes,
            static_cast<const char*>(from->data) + from->byte_offset, 0, nullptr, nullptr));
        break;
      case MemoryLayout::kImage2DActivation:
      case MemoryLayout::kImage2DWeight:
        auto image_info = GetImageInfo(to_desc, to);
        OPENCL_CALL(clEnqueueWriteImage(
            this->GetQueue(to->device), to_desc->buffer, CL_FALSE, image_info.origin,
            image_info.region, image_info.row_pitch, image_info.slice_pitch,
            static_cast<const char*>(from->data) + from->byte_offset, 0, nullptr, nullptr));
        break;
    }
    OPENCL_CALL(clFinish(this->GetQueue(to->device)));
  } else {
    LOG(FATAL) << "Expect copy from/to OpenCL or between OpenCL";
  }
}
#endif

  std::vector<cl_device_id> GetDeviceIDs(cl_platform_id pid, std::string device_type)
  {
    cl_device_type dtype = CL_DEVICE_TYPE_ALL;
    if (device_type == "cpu")
      dtype = CL_DEVICE_TYPE_CPU;
    if (device_type == "gpu")
      dtype = CL_DEVICE_TYPE_GPU;
    if (device_type == "accelerator")
      dtype = CL_DEVICE_TYPE_ACCELERATOR;
    cl_uint ret_size;
    cl_int code = clGetDeviceIDs(pid, dtype, 0, nullptr, &ret_size);
    std::vector<cl_device_id> ret;
    if (code != CL_SUCCESS)
      return ret;
    ret.resize(ret_size);
    OPENCL_CALL(clGetDeviceIDs(pid, dtype, ret_size, &ret[0], nullptr));
    return ret;
  }

  bool MatchPlatformInfo(cl_platform_id pid, cl_platform_info param_name, std::string value)
  {
    if (value.length() == 0)
      return true;
    std::string param_value = GetPlatformInfo(pid, param_name);
    return param_value.find(value) != std::string::npos;
  }

  void OpenCLWorkspace::Init(const std::string &type_key, const std::string &device_type,
                             const std::string &platform_name)
  {
    if (initialized_)
      return;
    std::lock_guard<std::mutex> lock(this->mu);
    if (initialized_)
      return;
    if (context != nullptr)
      return;
    this->type_key = type_key;
    // matched platforms
    std::vector<cl_platform_id> platform_ids = GetPlatformIDs();
    if (platform_ids.size() == 0)
    {
      LOGI("No OpenCL platform matched given existing options ...");
      return;
    }
    this->platform_id = nullptr;
    for (auto platform_id : platform_ids)
    {
      if (!MatchPlatformInfo(platform_id, CL_PLATFORM_NAME, platform_name))
      {
        continue;
      }
      std::vector<cl_device_id> devices_matched = GetDeviceIDs(platform_id, device_type);
      if ((devices_matched.size() == 0) && (device_type == "gpu"))
      {
        LOGI("Using CPU OpenCL device");
        devices_matched = GetDeviceIDs(platform_id, "cpu");
      }
      if (devices_matched.size() > 0)
      {
        this->platform_id = platform_id;
        this->platform_name = GetPlatformInfo(platform_id, CL_PLATFORM_NAME);
        this->device_type = device_type;
        this->device = devices_matched[0];
        break;
      }
    }
    if (this->platform_id == nullptr)
    {
      LOGE("No OpenCL device");
      return;
    }
    cl_int err_code;
    this->context = clCreateContext(nullptr, 1, &(this->device), nullptr,
                                    nullptr, &err_code);
    OPENCL_CHECK_ERROR(err_code);
    this->queue = clCreateCommandQueue(this->context, this->device, 0, &err_code);
    initialized_ = true;
  }

  void OpenCLWorkspace::InstallKernel(const std::string &cl_path, std::vector<std::string> &func_names)
  {
    cl_int err_code;
    if (!createProgram(this->context, this->device, cl_path, &program))
    {
      LOGE("CreateProgram failed\n");
      return;
    }
    for (auto name : func_names)
    {
      cl_kernel kernel = clCreateKernel(this->program, name.c_str(), &err_code);
      OPENCL_CHECK_ERROR(err_code);
      kernel_maps.insert(std::make_pair(name, kernel));
    }
  }

  void OpenCLWorkspace::InstallKernel(const unsigned char *binaryPtr, const size_t binarySize, std::vector<std::string> &func_names)
  {
    cl_int err_code;
    if (!createProgramFromBinary(this->context, this->device, binaryPtr, binarySize, &program))
    {
      LOGE("CreateProgram failed\n");
      return;
    }

    for (auto name : func_names)
    {
      cl_kernel kernel = clCreateKernel(this->program, name.c_str(), &err_code);
      OPENCL_CHECK_ERROR(err_code);
      kernel_maps.insert(std::make_pair(name, kernel));
    }
  }

  void OpenCLWorkspace::InstallKernel(const std::string &cl_path, std::string &func_name)
  {
    cl_int err_code;
    if (!createProgram(this->context, this->device, cl_path, &program))
    {
      LOGE("CreateProgram failed\n");
      return;
    }

    cl_kernel kernel = clCreateKernel(this->program, func_name.c_str(), &err_code);
    OPENCL_CHECK_ERROR(err_code);
    kernel_maps.insert(std::make_pair(func_name, kernel));
  }

  //create program and kernel with const char **
  void OpenCLWorkspace::InstallKernel(const char **cl_source, size_t *source_size, std::string &func_name)
  {
    cl_int err_code;
    if (!createProgram(this->context, this->device, cl_source, source_size, &program))
    {
      LOGE("CreateProgram failed\n");
      return;
    }
    cl_kernel kernel = clCreateKernel(this->program, func_name.c_str(), &err_code);
    OPENCL_CHECK_ERROR(err_code);
    kernel_maps.insert(std::make_pair(func_name, kernel));
  }


  void OpenCLWorkspace::getProgramBinary(std::vector<char> &buf, std::string out_file)
  {
    if (this->program == nullptr)
    {
      LOGE("program is nullptr\n");
      assert(0);
    }
    size_t sz = 0;
    clGetProgramInfo(this->program, CL_PROGRAM_BINARY_SIZES, sizeof(sz), &sz, NULL);
    buf.resize(sz);
    char *ptr = (char *)&buf[0];
    clGetProgramInfo(this->program, CL_PROGRAM_BINARIES, sizeof(ptr), &ptr, NULL);
    if (!buf.empty() && !out_file.empty())
    {
      std::ofstream out(out_file.c_str(), std::ios::out | std::ios::binary);
      std::copy(buf.cbegin(), buf.cend(), std::ostreambuf_iterator<char>(out));
      out.close();
    }
  }
}
