/*
 * @Author: Randall.zhuo
 * @Date: 2020-12-08
 * @Description: TODO
 */

/**
 * \file opencl_wrapper.h
 * \brief Functions to simplify the use of OpenCL API.
 */

#ifndef OPENCL_WRAPPER_H
#define OPENCL_WRAPPER_H

#include <time.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <string>

#if defined(__ANDROID__)
#include <android/hardware_buffer.h>
#endif

/**
 * \brief Release any OpenCL objects that have been created.
 * \details If any of the OpenCL objects passed in are not NULL, they will be freed using the appropriate OpenCL function.
 * \return False if an error occurred, otherwise true.
 */
bool cleanUpOpenCL(cl_context context, cl_command_queue commandQueue, cl_program program, 
        cl_kernel *kernel, int numberOfkernels,
        cl_mem *memoryObjects, int numberOfMemoryObjects);

/**
 * \brief Create an OpenCL context on a GPU on the first available platform.
 * \param[out] context Pointer to the created OpenCL context.
 * \return False if an error occurred, otherwise true.
 */
bool createContext(cl_context *context, cl_device_id *devices);

/**
 * \brief Create an OpenCL command queue for a given context.
 * \param[in] context The OpenCL context to use.
 * \param[out] commandQueue The created OpenCL command queue.
 * \param[out] device The device in which the command queue is created.
 * \return False if an error occurred, otherwise true.
 */
bool createCommandQueue(cl_context context, cl_command_queue* commandQueue, cl_device_id* device);

/**
 * \brief Create an OpenCL program from a given file and compile it.
 * \param[in] context The OpenCL context in use.
 * \param[in] device The OpenCL device to compile the kernel for.
 * \param[in] filename Name of the file containing the OpenCL kernel code to load.
 * \param[out] program The created OpenCL program object.
 * \return False if an error occurred, otherwise true.
 */
bool createProgram(cl_context context, cl_device_id device, std::string filename, cl_program* program);
bool createProgram(cl_context context, cl_device_id device, const char **source, size_t *size, cl_program* program);
bool createProgramFromBinary(cl_context context, cl_device_id device, const unsigned char *binaryAddr,
                             const size_t binarySize, cl_program *program);


/**
 * \brief Convert OpenCL error numbers to their string form.
 * \details Uses the error number definitions from cl.h.
 * \param[in] errorNumber The error number returned from an OpenCL command.
 * \return A name of the error.
 */
std::string errorNumberToString(cl_int errorNumber);

/**
 * \brief Check an OpenCL error number for errors.
 * \details If errorNumber is not CL_SUCESS, the function will print the string form of the error number.
 * \param[in] errorNumber The error number returned from an OpenCL command.
 * \return False if errorNumber != CL_SUCCESS, true otherwise.
 */
bool checkSuccess(cl_int errorNumber);

/**
 * \brief Query an OpenCL device to see if it supports an extension.
 * \param[in] device The device to query.
 * \param[in] extension The string name of the extension to query for.
 * \return True if the extension is supported on the given device, false otherwise.
 */
bool isExtensionSupported(cl_device_id device, std::string extension);

cl_mem createCLMemFromDma(cl_context context, cl_device_id device, cl_mem_flags flags, int dmafd, size_t size);

#if defined(__ANDROID__)
cl_mem createCLMemFromAHB(cl_context context, cl_device_id device, cl_mem_flags flags, AHardwareBuffer *ahb);
#endif

#endif
