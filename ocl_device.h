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

    }

    always_inline void log_info() {
        log_info(device);
    }

};
};