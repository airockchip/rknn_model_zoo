#pragma once

/* There are many OpenCL platforms that do not yet support OpenCL 2.0,
 * hence we use 1.2 APIs, some of which are now deprecated.  In order
 * to turn off the deprecation warnings (elevated to errors by
 * -Werror) we explicitly disable the 1.2 deprecation warnings.
 *
 * At the point TVM supports minimum version 2.0, we can remove this
 * define.
 */
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

/* Newer releases of OpenCL header files (after May 2018) work with
 * any OpenCL version, with an application's target version
 * specified. Setting the target version disables APIs from after that
 * version, and sets appropriate USE_DEPRECATED macros.  The above
 * macro for CL_USE_DEPRECATED_OPENCL_1_2_APIS is still needed in case
 * we are compiling against the earlier version-specific OpenCL header
 * files.  This also allows us to expose the OpenCL version through
 * tvm.runtime.Device.
 */
//#define CL_TARGET_OPENCL_VERSION 120

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "opencl_memory.h"
#include "Log.h"

namespace gpu_postprocess
{

    enum DeviceAttrKind : int
    {
        kExist = 0,
        kMaxThreadsPerBlock = 1,
        kWarpSize = 2,
        kMaxSharedMemoryPerBlock = 3,
        kComputeVersion = 4,
        kDeviceName = 5,
        kMaxClockRate = 6,
        kMultiProcessorCount = 7,
        kMaxThreadDimensions = 8,
        kMaxRegistersPerBlock = 9,
        kGcnArch = 10,
        kApiVersion = 11,
        kDriverVersion = 12
    };

    static_assert(sizeof(cl_mem) == sizeof(void *), "Required to store cl_mem inside void*");

    inline const char *CLGetErrorString(cl_int error)
    {
        switch (error)
        {
        case CL_SUCCESS:
            return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND:
            return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:
            return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:
            return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES:
            return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:
            return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:
            return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:
            return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE:
            return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE:
            return "CL_MAP_FAILURE";
        case CL_INVALID_VALUE:
            return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:
            return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:
            return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:
            return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:
            return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:
            return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:
            return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:
            return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:
            return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:
            return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:
            return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:
            return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:
            return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:
            return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:
            return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:
            return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:
            return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:
            return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:
            return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:
            return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:
            return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:
            return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE:
            return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:
            return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:
            return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:
            return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:
            return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:
            return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:
            return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:
            return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:
            return "CL_INVALID_MIP_LEVEL";
        default:
            return "Unknown OpenCL error code";
        }
    }

/*!
 * \brief Protected OpenCL call
 * \param func Expression to call.
 */
#define OPENCL_CHECK_ERROR(e)                                             \
    {                                                                     \
        if (e != CL_SUCCESS)                                              \
            LOGE("OpenCL Error, code= %d: %s, %s:%d\n", e, CLGetErrorString(e), __FILE__, __LINE__); \
    }

#define OPENCL_CALL(func)      \
    {                          \
        cl_int e = (func);     \
        OPENCL_CHECK_ERROR(e); \
    }

    /*!
 * \brief Process global OpenCL workspace.
 */
    const cl_import_properties_arm props_[3] = {
            CL_IMPORT_TYPE_ARM,
            CL_IMPORT_TYPE_DMA_BUF_ARM,
            0,
    };
    class OpenCLWorkspace
    {
    public:


    public:
        // type key
        std::string type_key;
        // global platform id
        cl_platform_id platform_id;
        // global platform name
        std::string platform_name;
        // global context of this process
        cl_context context{nullptr};
        // whether the workspace it initialized.
        bool initialized_{false};
        // the device type
        std::string device_type;
        // the devices
        cl_device_id device;
        // the queues
        cl_command_queue queue;

        // program
        cl_program program;

        // name kernel map
        std::unordered_map<std::string, cl_kernel> kernel_maps;

        // input/output memory objects
        std::unordered_map<std::string, std::shared_ptr<BufferDescriptor>> bufDescMaps;

        //input/output texture image
        std::unordered_map<std::string, std::shared_ptr<ImageDescriptor>> imgDescMaps;

        // the mutex for initialization
        std::mutex mu;

        OpenCLWorkspace(){
            Init();
        }
        // destructor
        virtual ~OpenCLWorkspace()
        {
            for (auto &kv : kernel_maps)
            {
                OPENCL_CALL(clReleaseKernel(kv.second));
            }
            kernel_maps.clear();
            if (program != nullptr)
            {
                OPENCL_CALL(clReleaseProgram(program));
                program = nullptr;
            }
            if (queue != nullptr)
            {
                OPENCL_CALL(clReleaseCommandQueue(queue));
            }
            if (context != nullptr)
            {
                OPENCL_CALL(clReleaseContext(context));
                context = nullptr;
            }
            if (device != nullptr)
            {
                OPENCL_CALL(clReleaseDevice(device));
            }
        }
        // Initialzie the device.
        void Init(const std::string &type_key, const std::string &device_type,
                  const std::string &platform_name = "");
        virtual void Init() { Init("opencl", "gpu"); }

        virtual void GetAttr(DeviceAttrKind kind);

        // override device API
        virtual void InstallKernel(const std::string &cl_path, std::string &func_name);
        virtual void InstallKernel(const std::string &cl_path, std::vector<std::string> &func_names);
        virtual void InstallKernel(const unsigned char *binaryPtr, const size_t binarySize, std::vector<std::string> &func_names);
        virtual void InstallKernel(const char **cl_source, size_t *source_size, std::string &func_names);

        virtual void AllocDataSpace(std::string name, int fd, int offset, size_t size, cl_mem_flags flag, MemFetchType mem_type);
        virtual int SetDataSpace(std::string name, int fd, void *buf, size_t size);
        virtual std::shared_ptr<BufferDescriptor> GetDataDesc(std::string name) const;
        virtual int GetDataSpace(std::string name, int fd, void *buf, size_t size);
        virtual void FreeDataSpace();

        void AllocImageTexture(std::string name, cl_mem_flags flag, const cl_image_format* image_format,
              size_t height, size_t width, MemFetchType mem_type ,size_t img_array_size = 1) ;
        int SetImageTexture(std::string name, void *buf, size_t height, size_t width, size_t img_array_size = 1);
        std::shared_ptr<ImageDescriptor> GetImageDesc(std::string name) const;
        int GetImageTexture(std::string name,void *buf, size_t height, size_t width, size_t img_array_size = 1);
        void FreeImageTexture();

        // void OpenCLWorkspace::CopyDataFromTo(BufferDescriptor *from, BufferDescriptor *to);
        void getProgramBinary(std::vector<char> &buf, std::string out_file);
    };
}
