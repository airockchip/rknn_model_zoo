/*
 * @Author: Randall.zhuo
 * @Date: 2020-12-08
 * @Description: TODO
 */

#include "opencl_wrapper.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <stdlib.h>
#include "Log.h"

using namespace std;

#define ENABLE_CL_PRINT

#ifdef ENABLE_CL_PRINT 
static void printf_callback(const char *buffer, unsigned int len, size_t complete, void *user_data)
{
    printf("%.*s", len, buffer);
    fflush(NULL);
}
#endif

bool cleanUpOpenCL(cl_context context, cl_command_queue commandQueue, cl_program program, 
        cl_kernel *kernel, int numberOfkernels,
        cl_mem *memoryObjects, int numberOfMemoryObjects)
{
    bool returnValue = true;

    if (commandQueue != 0)
    {
        if (!checkSuccess(clReleaseCommandQueue(commandQueue)))
        {
            cerr << "Releasing the OpenCL command queue failed. " << __FILE__ << ":" << __LINE__ << endl;
            returnValue = false;
        }
    }

    for (int index = 0; index < numberOfkernels; index++)
    {
        if (kernel[index] != 0)
        {
            if (!checkSuccess(clReleaseKernel(kernel[index])))
            {
                cerr << "Releasing the OpenCL kernel failed. " << __FILE__ << ":" << __LINE__ << endl;
                returnValue = false;
            }
        }
    }

    if (program != 0)
    {
        if (!checkSuccess(clReleaseProgram(program)))
        {
            cerr << "Releasing the OpenCL program failed. " << __FILE__ << ":" << __LINE__ << endl;
            returnValue = false;
        }
    }

    for (int index = 0; index < numberOfMemoryObjects; index++)
    {
        if (memoryObjects[index] != 0)
        {
            if (!checkSuccess(clReleaseMemObject(memoryObjects[index])))
            {
                cerr << "Releasing the OpenCL memory object " << index << " failed. " << __FILE__ << ":" << __LINE__ << endl;
                returnValue = false;
            }
        }
    }

    if (context != 0)
    {
        if (!checkSuccess(clReleaseContext(context)))
        {
            cerr << "Releasing the OpenCL context failed. " << __FILE__ << ":" << __LINE__ << endl;
            returnValue = false;
        }
    }

    return returnValue;
}

bool createContext(cl_context *context, cl_device_id *devices)
{
    cl_int errorNumber = 0;
    cl_uint numberOfPlatforms = 0;
    cl_platform_id firstPlatformID = 0;
    

    /* Retrieve a single platform ID. */
    if (!checkSuccess(clGetPlatformIDs(1, &firstPlatformID, &numberOfPlatforms)))
    {
        cerr << "Retrieving OpenCL platforms failed. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    if (numberOfPlatforms <= 0)
    {
        cerr << "No OpenCL platforms found. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    //获取设备
    if (!checkSuccess(clGetDeviceIDs(firstPlatformID, CL_DEVICE_TYPE_GPU, 1, devices, NULL)))
    {
        printf("Get device failed!\n");
        return -1;
    }

    printf("devices=%d\n", *devices);

    size_t ext_size = 0;
    if (!checkSuccess(clGetDeviceInfo(*devices, CL_DEVICE_EXTENSIONS, 0, NULL, &ext_size)))
    {
        return -1;
    }

    // Check if 32 bit local atomics are supported
    char *ext_string = (char *)malloc(ext_size + 1);
    if (!checkSuccess(clGetDeviceInfo(*devices, CL_DEVICE_EXTENSIONS, ext_size + 1, ext_string, NULL)))
    {
        return -1;
    }
    printf("CL_DEVICE_EXTENSIONS:%s\n", ext_string);

    /* Get a context with a GPU device from the platform found above. */
    cl_context_properties contextProperties[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)firstPlatformID, 
#ifdef ENABLE_CL_PRINT        
        // Enable a printf callback function for this context.
        CL_PRINTF_CALLBACK_ARM, reinterpret_cast<cl_context_properties>(printf_callback),
        // Request a minimum printf buffer size of 4MB for devices in the
        // context that support this extension.
        CL_PRINTF_BUFFERSIZE_ARM, 0x1000,
#endif
        0
        };
    *context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, NULL, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cerr << "Creating an OpenCL context failed. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    return true;
}

bool createCommandQueue(cl_context context, cl_command_queue *commandQueue, cl_device_id *device)
{
    cl_int errorNumber = 0;
    cl_device_id *devices = NULL;
    size_t deviceBufferSize = -1;

    /* Retrieve the size of the buffer needed to contain information about the devices in this OpenCL context. */
    if (!checkSuccess(clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize)))
    {
        cerr << "Failed to get OpenCL context information. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    if (deviceBufferSize == 0)
    {
        cerr << "No OpenCL devices found. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    /* Retrieve the list of devices available in this context. */
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    if (!checkSuccess(clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL)))
    {
        cerr << "Failed to get the OpenCL context information. " << __FILE__ << ":" << __LINE__ << endl;
        delete[] devices;
        return false;
    }

    /* Use the first available device in this context. */
    *device = devices[0];
    delete[] devices;

    /* Set up the command queue with the selected device. */
    *commandQueue = clCreateCommandQueue(context, *device, NULL, &errorNumber);
    if (!checkSuccess(errorNumber))
    {
        cerr << "Failed to create the OpenCL command queue. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    return true;
}

bool createProgram(cl_context context, cl_device_id device, string filename, cl_program *program)
{
    cl_int errorNumber = 0;
    ifstream kernelFile(filename.c_str(), ios::in);

    if (!kernelFile.is_open())
    {
        cerr << "Unable to open " << filename << ". " << __FILE__ << ":" << __LINE__ << endl;
        LOGE("Unable to open %s\n",filename.c_str());
        return false;
    }

    /*
     * Read the kernel file into an output stream.
     * Convert this into a char array for passing to OpenCL.
     */
    ostringstream outputStringStream;
    outputStringStream << kernelFile.rdbuf();
    string srcStdStr = outputStringStream.str();
    const char *charSource = srcStdStr.c_str();

    *program = clCreateProgramWithSource(context, 1, &charSource, NULL, &errorNumber);
    if (!checkSuccess(errorNumber) || program == NULL)
    {
        cerr << "Failed to create OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        LOGE("Failed to create OpenCL program. %s\n",filename.c_str());
        return false;
    }

    /* Try to build the OpenCL program. */
    bool buildSuccess = checkSuccess(clBuildProgram(*program, 0, NULL, NULL, NULL, NULL));

    /* Get the size of the build log. */
    size_t logSize = 0;
    clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

    /*
     * If the build succeeds with no log, an empty string is returned (logSize = 1),
     * we only want to print the message if it has some content (logSize > 1).
     */
    if (logSize > 1)
    {
        char *log = new char[logSize];
        clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);

        string *stringChars = new string(log, logSize);
        cerr << "Build log:\n " << *stringChars << endl;
        LOGE("Build log: %s\n",(*stringChars).c_str());

        delete[] log;
        delete stringChars;
    }

    if (!buildSuccess)
    {
        clReleaseProgram(*program);
        cerr << "Failed to build OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    return true;
}

bool createProgram(cl_context context, cl_device_id device, const char **source, size_t *size, cl_program *program)
{
    cl_int errorNumber = 0;

    *program = clCreateProgramWithSource(context, 1, source, size, &errorNumber);
    if (!checkSuccess(errorNumber) || program == NULL)
    {
        cerr << "Failed to create OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    /* Try to build the OpenCL program. */
    bool buildSuccess = checkSuccess(clBuildProgram(*program, 0, NULL, NULL, NULL, NULL));

    /* Get the size of the build log. */
    size_t logSize = 0;
    clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

    /*
     * If the build succeeds with no log, an empty string is returned (logSize = 1),
     * we only want to print the message if it has some content (logSize > 1).
     */
    if (logSize > 1)
    {
        char *log = new char[logSize];
        clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);

        string *stringChars = new string(log, logSize);
        cerr << "Build log:\n " << *stringChars << endl;

        delete[] log;
        delete stringChars;
    }

    if (!buildSuccess)
    {
        clReleaseProgram(*program);
        cerr << "Failed to build OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    return true;
}

bool createProgramFromBinary(cl_context context, cl_device_id device, const unsigned char *binaryAddr, 
                            const size_t binarySize, cl_program *program)
{
    cl_int errorNumber = 0;

    *program = clCreateProgramWithBinary(context, 1, &device,
                                         &binarySize, &binaryAddr, NULL, &errorNumber);
    if (!checkSuccess(errorNumber) || program == NULL)
    {
        cerr << "Failed to create OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    /* Try to build the OpenCL program. */
    bool buildSuccess = checkSuccess(clBuildProgram(*program, 0, NULL, NULL, NULL, NULL));

    /* Get the size of the build log. */
    size_t logSize = 0;
    clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

    /*
     * If the build succeeds with no log, an empty string is returned (logSize = 1),
     * we only want to print the message if it has some content (logSize > 1).
     */
    if (logSize > 1)
    {
        char *log = new char[logSize];
        clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);

        string *stringChars = new string(log, logSize);
        cerr << "Build log:\n " << *stringChars << endl;

        delete[] log;
        delete stringChars;
    }

    if (!buildSuccess)
    {
        clReleaseProgram(*program);
        cerr << "Failed to build OpenCL program. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    return true;
}

bool checkSuccess(cl_int errorNumber)
{
    if (errorNumber != CL_SUCCESS)
    {
        cerr << "OpenCL error: " << errorNumberToString(errorNumber) << endl;
        return false;
    }
    return true;
}

string errorNumberToString(cl_int errorNumber)
{
    switch (errorNumber)
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
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
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
        return "Unknown error";
    }
}

bool isExtensionSupported(cl_device_id device, string extension)
{
    if (extension.empty())
    {
        return false;
    }

    /* First find out how large the ouput of the OpenCL device query will be. */
    size_t extensionsReturnSize = 0;
    if (!checkSuccess(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionsReturnSize)))
    {
        cerr << "Failed to get return size from clGetDeviceInfo for parameter CL_DEVICE_EXTENSIONS. " << __FILE__ << ":" << __LINE__ << endl;
        return false;
    }

    /* Allocate enough memory for the output. */
    char *extensions = new char[extensionsReturnSize];

    /* Get the list of all extensions supported. */
    if (!checkSuccess(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, extensionsReturnSize, extensions, NULL)))
    {
        cerr << "Failed to get data from clGetDeviceInfo for parameter CL_DEVICE_EXTENSIONS. " << __FILE__ << ":" << __LINE__ << endl;
        delete[] extensions;
        return false;
    }

    /* See if the requested extension is in the list. */
    string *extensionsString = new string(extensions);
    bool returnResult = false;
    if (extensionsString->find(extension) != string::npos)
    {
        returnResult = true;
    }

    delete[] extensions;
    delete extensionsString;

    return returnResult;
}

cl_mem createCLMemFromDma(cl_context context, cl_device_id device, cl_mem_flags flags, int dmafd, size_t size)
{
    cl_mem memoryObject;

    const cl_import_properties_arm props[3] = {
        CL_IMPORT_TYPE_ARM,
        CL_IMPORT_TYPE_DMA_BUF_ARM,
        0,
    };

    cl_int error = CL_SUCCESS;
    memoryObject = clImportMemoryARM(context, flags, props, &dmafd, size, &error);

    if(!checkSuccess(error)) {
        printf("clImportMemoryARM fail!\n");
        return NULL;
    }

    return memoryObject;
}

#if __ANDROID__
cl_mem createCLMemFromAHB(cl_context context, cl_device_id device, cl_mem_flags flags, AHardwareBuffer *ahb)
{
    cl_mem memoryObject;

    const cl_import_properties_arm props[3] = {
        CL_IMPORT_TYPE_ARM,
        CL_IMPORT_TYPE_ANDROID_HARDWARE_BUFFER_ARM,
        0,
    };

    cl_int error = CL_SUCCESS;
    memoryObject = clImportMemoryARM(context, flags, props, ahb, CL_IMPORT_MEMORY_WHOLE_ALLOCATION_ARM, &error);

    if(!checkSuccess(error)) {
        printf("clImportMemoryARM fail!\n");
        return NULL;
    }

    return memoryObject;
}
#endif