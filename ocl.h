#pragma once

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <format>
#include <vector>
#include <optional>
#include <expected>
#include "each.h"
#include <iostream>

#ifndef always_inline
#ifdef _WIN32
#define always_inline inline __forceinline
#else
#define always_inline inline __attribute__((always_inline))
#endif
#endif // !always_inline


class ocl {
public:
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

    };

    class platform {
    public:
        cl_platform_id platform;
        always_inline static std::expected<size_t, status> get_info_size(cl_platform_id platform, cl_platform_info param_name) noexcept {
            size_t param_value_size_ret{};
            status sts0{ clGetPlatformInfo(platform, param_name, 0, nullptr, &param_value_size_ret) };
            if (sts0 != status::SUCCESS) return std::unexpected(sts0);
            return decltype(get_info_size(platform, param_name))(param_value_size_ret);
        }

        always_inline std::expected<size_t, status> get_info_size(cl_platform_info param_name) const noexcept {
            return get_info_size(platform, param_name);
        }

        always_inline static std::expected<std::string, status> get_info_string(cl_platform_id platform, cl_platform_info param_name) noexcept {
            std::string string_pointer(get_info_size(platform, param_name).value(), 0);

            status sts1{ clGetPlatformInfo(platform, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return decltype(get_info_string(platform, param_name))(string_pointer);
        }

        always_inline std::expected<std::string, status> get_info_string(cl_platform_info param_name) const noexcept {
            return get_info_string(platform, param_name);
        }

        always_inline static std::expected<std::string, status> get_profile(cl_platform_id platform) noexcept { return get_info_string(platform, CL_PLATFORM_PROFILE); }
        always_inline std::expected<std::string, status> get_profile() const noexcept { return get_profile(platform); }

        always_inline static std::expected<std::string, status> get_version(cl_platform_id platform) noexcept { return get_info_string(platform, CL_PLATFORM_VERSION); }
        always_inline std::expected<std::string, status> get_version() const noexcept { return get_version(platform); }

        always_inline static std::expected<std::string, status> get_name(cl_platform_id platform) noexcept { return get_info_string(platform, CL_PLATFORM_NAME); }
        always_inline std::expected<std::string, status> get_name() const noexcept { return get_name(platform); }

        always_inline static std::expected<std::string, status> get_vendor(cl_platform_id platform) noexcept { return get_info_string(platform, CL_PLATFORM_VENDOR); }
        always_inline std::expected<std::string, status> get_vendor() const noexcept { return get_vendor(platform); }

        always_inline static std::expected<std::string, status> get_extensions(cl_platform_id platform) noexcept { return get_info_string(platform, CL_PLATFORM_EXTENSIONS); }
        always_inline std::expected<std::string, status> get_extensions() const noexcept { return get_extensions(platform); }

        always_inline static void log_info(cl_platform_id platform) {
            std::cout << std::format("profile:{}\n", ocl::platform::get_profile(platform).value());
            std::cout << std::format("version:{}\n", ocl::platform::get_version(platform).value());
            std::cout << std::format("name:{}\n", ocl::platform::get_name(platform).value());
            std::cout << std::format("vendor:{}\n", ocl::platform::get_vendor(platform).value());
            std::cout << std::format("extensions:{}\n", ocl::platform::get_extensions(platform).value());
        }

        always_inline void log_info() {
            log_info(platform);
        }
    };

    always_inline static std::expected<std::vector<cl_platform_id>, status> GetPlatformIDs(cl_uint num_platforms) noexcept {
        std::vector<cl_platform_id> ret(static_cast<size_t>(num_platforms));
        status sts1{ clGetPlatformIDs(static_cast<cl_uint>(ret.size()), ret.data(), nullptr) };
        if (sts1 != status::SUCCESS) return std::unexpected(sts1);

        return decltype(GetPlatformIDs(num_platforms))(ret);
    }

    always_inline static std::expected<std::vector<platform>, status> GetPlatforms(cl_uint num_platforms) noexcept {
        std::vector<platform> ret(static_cast<size_t>(num_platforms));
        status sts1{ clGetPlatformIDs(static_cast<cl_uint>(ret.size()), reinterpret_cast<cl_platform_id *>(ret.data()), nullptr) };
        if (sts1 != status::SUCCESS) return std::unexpected(sts1);

        return decltype(GetPlatforms(num_platforms))(ret);
    }

    always_inline static std::expected<cl_uint, status> GetPlatformID_count() noexcept {
        cl_uint num_platforms{};
        status sts0{ clGetPlatformIDs(0, nullptr, &num_platforms) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return decltype(GetPlatformID_count())(num_platforms);
    }

    always_inline static std::expected<std::vector<cl_platform_id>, status> GetPlatformIDs() noexcept {
        return decltype(GetPlatformIDs())(GetPlatformIDs(GetPlatformID_count().value()));
    }

    always_inline static std::expected<std::vector<platform>, status> GetPlatforms() noexcept {
        return decltype(GetPlatforms())(GetPlatforms(GetPlatformID_count().value()));
    }

    always_inline static void each_platform(auto lambda) noexcept {
        each(ocl::GetPlatforms().value(), lambda);
    }

};
