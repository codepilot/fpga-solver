#pragma once

#ifndef  CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 300
#endif // ! CL_TARGET_OPENCL_VERSION

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_ext.h>
#include <format>
#include <vector>
#include <optional>
#include <expected>
#include "each.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <span>
#include <set>
#include <map>

#ifndef always_inline
#ifdef _WIN32
#define always_inline inline __forceinline
#else
#define always_inline inline
//__attribute__((always_inline))
#endif
#endif // !always_inline


namespace ocl {
    const inline static std::map<cl_kernel_arg_address_qualifier, std::string> ADDRESS_QUALIFIER{
        {CL_KERNEL_ARG_ADDRESS_GLOBAL, "global"},
        {CL_KERNEL_ARG_ADDRESS_LOCAL, "local"},
        {CL_KERNEL_ARG_ADDRESS_CONSTANT, "constant"},
        {CL_KERNEL_ARG_ADDRESS_PRIVATE, "private"},
    };

    const inline static std::map<cl_kernel_arg_access_qualifier, std::string> ACCESS_QUALIFIER{
        {CL_KERNEL_ARG_ACCESS_READ_ONLY, "read_only"},
        {CL_KERNEL_ARG_ACCESS_WRITE_ONLY, "write_only"},
        {CL_KERNEL_ARG_ACCESS_READ_WRITE, "read_write"},
        {CL_KERNEL_ARG_ACCESS_NONE, ""},
    };

    const inline static std::map<cl_kernel_arg_type_qualifier, std::string> TYPE_QUALIFIER{
        {CL_KERNEL_ARG_TYPE_NONE, "none"},
        {CL_KERNEL_ARG_TYPE_CONST, "const"},
        {CL_KERNEL_ARG_TYPE_RESTRICT, "restrict"},
        {CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_RESTRICT, "const restrict"},
        {CL_KERNEL_ARG_TYPE_VOLATILE, "volatile"},
        {CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_VOLATILE, "const volatile"},
        {CL_KERNEL_ARG_TYPE_RESTRICT | CL_KERNEL_ARG_TYPE_VOLATILE, "restrict volatile"},
        {CL_KERNEL_ARG_TYPE_CONST | CL_KERNEL_ARG_TYPE_RESTRICT | CL_KERNEL_ARG_TYPE_VOLATILE, "const restrict volatile"},
        {CL_KERNEL_ARG_TYPE_PIPE, "pipe"},
    };

    inline static std::set<std::string> split(std::string str) noexcept {
        uint64_t start{};
        std::set<std::string> ret;
        each(str, [&](uint64_t i, char n) {
            if (n == 32) {
                ret.insert(std::string(str.data() + start, i - start));
                start = i + 1;
            }
            });
        return ret;
    }

    enum class status:cl_int {
        SUCCESS = CL_SUCCESS,
        DEVICE_NOT_FOUND = CL_DEVICE_NOT_FOUND,
        DEVICE_NOT_AVAILABLE = CL_DEVICE_NOT_AVAILABLE,
        COMPILER_NOT_AVAILABLE = CL_COMPILER_NOT_AVAILABLE,
        MEM_OBJECT_ALLOCATION_FAILURE = CL_MEM_OBJECT_ALLOCATION_FAILURE,
        OUT_OF_RESOURCES = CL_OUT_OF_RESOURCES,
        OUT_OF_HOST_MEMORY = CL_OUT_OF_HOST_MEMORY,
        PROFILING_INFO_NOT_AVAILABLE = CL_PROFILING_INFO_NOT_AVAILABLE,
        MEM_COPY_OVERLAP = CL_MEM_COPY_OVERLAP,
        IMAGE_FORMAT_MISMATCH = CL_IMAGE_FORMAT_MISMATCH,
        IMAGE_FORMAT_NOT_SUPPORTED = CL_IMAGE_FORMAT_NOT_SUPPORTED,
        BUILD_PROGRAM_FAILURE = CL_BUILD_PROGRAM_FAILURE,
        MAP_FAILURE = CL_MAP_FAILURE,

#ifdef CL_VERSION_1_1
        MISALIGNED_SUB_BUFFER_OFFSET = CL_MISALIGNED_SUB_BUFFER_OFFSET,
        EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
#endif

#ifdef CL_VERSION_1_2
        COMPILE_PROGRAM_FAILURE = CL_COMPILE_PROGRAM_FAILURE,
        LINKER_NOT_AVAILABLE = CL_LINKER_NOT_AVAILABLE,
        LINK_PROGRAM_FAILURE = CL_LINK_PROGRAM_FAILURE,
        DEVICE_PARTITION_FAILED = CL_DEVICE_PARTITION_FAILED,
        KERNEL_ARG_INFO_NOT_AVAILABLE = CL_KERNEL_ARG_INFO_NOT_AVAILABLE,
#endif

        INVALID_VALUE = CL_INVALID_VALUE,
        INVALID_DEVICE_TYPE = CL_INVALID_DEVICE_TYPE,
        INVALID_PLATFORM = CL_INVALID_PLATFORM,
        INVALID_DEVICE = CL_INVALID_DEVICE,
        INVALID_CONTEXT = CL_INVALID_CONTEXT,
        INVALID_QUEUE_PROPERTIES = CL_INVALID_QUEUE_PROPERTIES,
        INVALID_COMMAND_QUEUE = CL_INVALID_COMMAND_QUEUE,
        INVALID_HOST_PTR = CL_INVALID_HOST_PTR,
        INVALID_MEM_OBJECT = CL_INVALID_MEM_OBJECT,
        INVALID_IMAGE_FORMAT_DESCRIPTOR = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR,
        INVALID_IMAGE_SIZE = CL_INVALID_IMAGE_SIZE,
        INVALID_SAMPLER = CL_INVALID_SAMPLER,
        INVALID_BINARY = CL_INVALID_BINARY,
        INVALID_BUILD_OPTIONS = CL_INVALID_BUILD_OPTIONS,
        INVALID_PROGRAM = CL_INVALID_PROGRAM,
        INVALID_PROGRAM_EXECUTABLE = CL_INVALID_PROGRAM_EXECUTABLE,
        INVALID_KERNEL_NAME = CL_INVALID_KERNEL_NAME,
        INVALID_KERNEL_DEFINITION = CL_INVALID_KERNEL_DEFINITION,
        INVALID_KERNEL = CL_INVALID_KERNEL,
        INVALID_ARG_INDEX = CL_INVALID_ARG_INDEX,
        INVALID_ARG_VALUE = CL_INVALID_ARG_VALUE,
        INVALID_ARG_SIZE = CL_INVALID_ARG_SIZE,
        INVALID_KERNEL_ARGS = CL_INVALID_KERNEL_ARGS,
        INVALID_WORK_DIMENSION = CL_INVALID_WORK_DIMENSION,
        INVALID_WORK_GROUP_SIZE = CL_INVALID_WORK_GROUP_SIZE,
        INVALID_WORK_ITEM_SIZE = CL_INVALID_WORK_ITEM_SIZE,
        INVALID_GLOBAL_OFFSET = CL_INVALID_GLOBAL_OFFSET,
        INVALID_EVENT_WAIT_LIST = CL_INVALID_EVENT_WAIT_LIST,
        INVALID_EVENT = CL_INVALID_EVENT,
        INVALID_OPERATION = CL_INVALID_OPERATION,
        INVALID_GL_OBJECT = CL_INVALID_GL_OBJECT,
        INVALID_BUFFER_SIZE = CL_INVALID_BUFFER_SIZE,
        INVALID_MIP_LEVEL = CL_INVALID_MIP_LEVEL,
        INVALID_GLOBAL_WORK_SIZE = CL_INVALID_GLOBAL_WORK_SIZE,

#ifdef CL_VERSION_1_1
        INVALID_PROPERTY = CL_INVALID_PROPERTY,
#endif

#ifdef CL_VERSION_1_2
        INVALID_IMAGE_DESCRIPTOR = CL_INVALID_IMAGE_DESCRIPTOR,
        INVALID_COMPILER_OPTIONS = CL_INVALID_COMPILER_OPTIONS,
        INVALID_LINKER_OPTIONS = CL_INVALID_LINKER_OPTIONS,
        INVALID_DEVICE_PARTITION_COUNT = CL_INVALID_DEVICE_PARTITION_COUNT,
#endif

#ifdef CL_VERSION_2_0
        INVALID_PIPE_SIZE = CL_INVALID_PIPE_SIZE,
        INVALID_DEVICE_QUEUE = CL_INVALID_DEVICE_QUEUE,
#endif

#ifdef CL_VERSION_2_2
        INVALID_SPEC_ID = CL_INVALID_SPEC_ID,
        MAX_SIZE_RESTRICTION_EXCEEDED = CL_MAX_SIZE_RESTRICTION_EXCEEDED,
#endif
#ifdef CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR
        INVALID_GL_SHAREGROUP_REFERENCE_KHR = CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR,
#endif
    };
class platform;
class device;
class context;
class queue;
class buffer;
class program;
class kernel;
class event;
template<typename T> class svm;
};

#include "ocl_platform.h"
#include "ocl_svm.h"
#include "ocl_event.h"
#include "ocl_buffer.h"
#include "ocl_kernel.h"
#include "ocl_program.h"
#include "ocl_queue.h"
#include "ocl_context.h"
#include "ocl_device.h"
