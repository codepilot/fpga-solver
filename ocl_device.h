#pragma once

namespace ocl {
class device {
public:
	cl_device_id device;

    always_inline static std::expected<size_t, status> get_info_size(cl_device_id device, cl_device_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetDeviceInfo(device, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_device_info param_name) const noexcept {
        return get_info_size(device, param_name);
    }

    always_inline static std::expected<std::string, status> get_info_string(cl_device_id device, cl_device_info param_name) noexcept {
        return get_info_size(device, param_name).and_then([&](size_t size) -> std::expected<std::string, status> {
            std::string string_pointer(size, 0);

            status sts1{ clGetDeviceInfo(device, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<std::string, status>(string_pointer);
        });
    }
    always_inline std::expected<std::string, status> get_info_string(cl_device_info param_name) const noexcept {
        return get_info_string(device, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_device_id device, cl_device_info param_name) noexcept {
        return get_info_size(device, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetDeviceInfo(device, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
        });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_device_info param_name) const noexcept {
        return get_info_integral<cl_integral>(device, param_name);
    }

    always_inline static std::expected<std::string, status> get_profile(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_PROFILE); }
    always_inline std::expected<std::string, status> get_profile() const noexcept { return get_profile(device); }

    always_inline static std::expected<std::string, status> get_version(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_VERSION); }
    always_inline std::expected<std::string, status> get_version() const noexcept { return get_version(device); }

    always_inline static std::expected<std::string, status> get_name(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_NAME); }
    always_inline std::expected<std::string, status> get_name() const noexcept { return get_name(device); }

    always_inline static std::expected<std::string, status> get_vendor(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_VENDOR); }
    always_inline std::expected<std::string, status> get_vendor() const noexcept { return get_vendor(device); }

    always_inline static std::expected<std::string, status> get_extensions(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_EXTENSIONS); }
    always_inline std::expected<std::string, status> get_extensions() const noexcept { return get_extensions(device); }

    always_inline static std::expected<std::string, status> get_il_version(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_IL_VERSION); }
    always_inline std::expected<std::string, status> get_il_version() const noexcept { return get_il_version(device); }

    always_inline static std::expected<std::string, status> get_built_in_kernels(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_BUILT_IN_KERNELS); }
    always_inline std::expected<std::string, status> get_built_in_kernels() const noexcept { return get_built_in_kernels(device); }

    always_inline static std::expected<std::string, status> get_opencl_c_version(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_OPENCL_C_VERSION); }
    always_inline std::expected<std::string, status> get_opencl_c_version() const noexcept { return get_opencl_c_version(device); }

    always_inline static std::expected<std::string, status> get_latest_conformancce_version_passed(cl_device_id device) noexcept { return get_info_string(device, CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED); }
    always_inline std::expected<std::string, status> get_latest_conformancce_version_passed() const noexcept { return get_latest_conformancce_version_passed(device); }

    always_inline static std::expected<std::string, status> get_driver_version(cl_device_id device) noexcept { return get_info_string(device, CL_DRIVER_VERSION); }
    always_inline std::expected<std::string, status> get_driver_version() const noexcept { return get_driver_version(device); }

    always_inline static void log_info(cl_device_id device) {
        std::string na{"N/A"};
        std::cout << std::format("  profile: {}\n", ocl::device::get_profile(device).value_or(na));
        std::cout << std::format("  version: {}\n", ocl::device::get_version(device).value_or(na));
        std::cout << std::format("  name: {}\n", ocl::device::get_name(device).value_or(na));
        std::cout << std::format("  vendor: {}\n", ocl::device::get_vendor(device).value_or(na));
        std::cout << std::format("  extensions: {}\n", ocl::device::get_extensions(device).value_or(na));
        std::cout << std::format("  il_version: {}\n", ocl::device::get_il_version(device).value_or(na));
        std::cout << std::format("  built_in_kernels: {}\n", ocl::device::get_built_in_kernels(device).value_or(na));
        std::cout << std::format("  opencl_c_version: {}\n", ocl::device::get_opencl_c_version(device).value_or(na));
        std::cout << std::format("  latest_conformancce_version_passed: {}\n", ocl::device::get_latest_conformancce_version_passed(device).value_or(na));
        std::cout << std::format("  driver_version: {}\n", ocl::device::get_driver_version(device).value_or(na));

#define show_info_string(e) std::cout << std::format("  " #e ": {}\n", get_info_string(device, e).value_or(na));
#define show_info_bool(e) std::cout << std::format("  " #e ": {}\n", static_cast<bool>(get_info_integral<cl_bool>(device, e).value_or(0)));
#define show_info_uint(e) std::cout << std::format("  " #e ": {}\n", get_info_integral<cl_uint>(device, e).value_or(0));
#define show_info_ulong(e) std::cout << std::format("  " #e ": {}\n", get_info_integral<cl_ulong>(device, e).value_or(0));
#define show_info_device_type(e) std::cout << std::format("  " #e ": {}\n", get_info_integral<cl_device_type>(device, e).value_or(0));
#define show_info_size_t(e) std::cout << std::format("  " #e ": {}\n", get_info_integral<size_t>(device, e).value_or(0));
#define show_info_3x_size_t(e) {auto val{get_info_integral<std::array<size_t, 3>>(device, e).value_or(std::array<size_t, 3>{})}; std::cout << std::format("  " #e ": {}, {}, {}\n", val[0], val[1], val[2]); }
        show_info_device_type(CL_DEVICE_TYPE);
        show_info_uint(CL_DEVICE_VENDOR_ID);
        show_info_uint(CL_DEVICE_MAX_COMPUTE_UNITS);
        show_info_uint(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
        show_info_3x_size_t(CL_DEVICE_MAX_WORK_ITEM_SIZES);
        show_info_size_t(CL_DEVICE_MAX_WORK_GROUP_SIZE)
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
        show_info_uint(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE);
        show_info_uint(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF);
        show_info_uint(CL_DEVICE_MAX_CLOCK_FREQUENCY);
        show_info_uint(CL_DEVICE_ADDRESS_BITS);
        show_info_ulong(CL_DEVICE_MAX_MEM_ALLOC_SIZE);
        show_info_bool(CL_DEVICE_IMAGE_SUPPORT);
        show_info_uint(CL_DEVICE_MAX_READ_IMAGE_ARGS);
        show_info_uint(CL_DEVICE_MAX_WRITE_IMAGE_ARGS);
        show_info_uint(CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS);
        show_info_string(CL_DEVICE_IL_VERSION);
        // CL_DEVICE_ILS_WITH_VERSION
        show_info_size_t(CL_DEVICE_IMAGE2D_MAX_WIDTH);
        show_info_size_t(CL_DEVICE_IMAGE2D_MAX_HEIGHT);
        show_info_size_t(CL_DEVICE_IMAGE3D_MAX_WIDTH);
        show_info_size_t(CL_DEVICE_IMAGE3D_MAX_HEIGHT);
        show_info_size_t(CL_DEVICE_IMAGE3D_MAX_DEPTH);
        show_info_size_t(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE);
        show_info_size_t(CL_DEVICE_IMAGE_MAX_ARRAY_SIZE);
        show_info_uint(CL_DEVICE_MAX_SAMPLERS);
        show_info_uint(CL_DEVICE_IMAGE_PITCH_ALIGNMENT);
        show_info_uint(CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT);
        show_info_uint(CL_DEVICE_MAX_PIPE_ARGS);
        show_info_uint(CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS);
        show_info_uint(CL_DEVICE_PIPE_MAX_PACKET_SIZE);
        show_info_size_t(CL_DEVICE_MAX_PARAMETER_SIZE);
        show_info_uint(CL_DEVICE_MEM_BASE_ADDR_ALIGN);
        show_info_uint(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE);
        //CL_DEVICE_SINGLE_FP_CONFIG
        //CL_DEVICE_DOUBLE_FP_CONFIG
        //CL_DEVICE_GLOBAL_MEM_CACHE_TYPE
        show_info_uint(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
        show_info_ulong(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
        show_info_ulong(CL_DEVICE_GLOBAL_MEM_SIZE);
        show_info_ulong(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
        show_info_uint(CL_DEVICE_MAX_CONSTANT_ARGS);
        show_info_size_t(CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE);
        show_info_size_t(CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE);
        //CL_DEVICE_LOCAL_MEM_TYPE
        show_info_ulong(CL_DEVICE_LOCAL_MEM_SIZE);
        show_info_bool(CL_DEVICE_ERROR_CORRECTION_SUPPORT);
        show_info_bool(CL_DEVICE_HOST_UNIFIED_MEMORY);
        show_info_size_t(CL_DEVICE_PROFILING_TIMER_RESOLUTION);
        show_info_uint(CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE);
        show_info_uint(CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE);
        show_info_uint(CL_DEVICE_MAX_ON_DEVICE_QUEUES);
        show_info_uint(CL_DEVICE_MAX_ON_DEVICE_EVENTS);
        show_info_uint(CL_DEVICE_PARTITION_MAX_SUB_DEVICES);
        show_info_uint(CL_DEVICE_REFERENCE_COUNT);
        show_info_uint(CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT);
        show_info_uint(CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT);
        show_info_uint(CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT);
        show_info_uint(CL_DEVICE_MAX_NUM_SUB_GROUPS);
#undef show_info
    }

    always_inline void log_info() {
        log_info(device);
    }

    always_inline auto create_context() {
        return context::create_context(this);
    }

};
};