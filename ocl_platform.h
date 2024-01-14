#pragma once

namespace ocl {
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
        std::cout << std::format("profile: {}\n", ocl::platform::get_profile(platform).value());
        std::cout << std::format("version: {}\n", ocl::platform::get_version(platform).value());
        std::cout << std::format("name: {}\n", ocl::platform::get_name(platform).value());
        std::cout << std::format("vendor: {}\n", ocl::platform::get_vendor(platform).value());
        std::cout << std::format("extensions: {}\n", ocl::platform::get_extensions(platform).value());
    }

    always_inline void log_info() {
        log_info(platform);
    }

    always_inline static std::expected<std::vector<cl_platform_id>, status> get_ids(cl_uint num) noexcept {
        std::vector<cl_platform_id> ret(static_cast<size_t>(num));
        status sts1{ clGetPlatformIDs(static_cast<cl_uint>(ret.size()), ret.data(), nullptr) };
        if (sts1 != status::SUCCESS) return std::unexpected(sts1);

        return decltype(get_ids(num))(ret);
    }

    always_inline static std::expected<std::vector<ocl::platform>, status> get(cl_uint num) noexcept {
        std::vector<ocl::platform> ret(static_cast<size_t>(num));
        status sts1{ clGetPlatformIDs(static_cast<cl_uint>(ret.size()), reinterpret_cast<cl_platform_id*>(ret.data()), nullptr) };
        if (sts1 != status::SUCCESS) return std::unexpected(sts1);

        return decltype(get(num))(ret);
    }

    always_inline static std::expected<cl_uint, status> size() noexcept {
        cl_uint num{};
        status sts0{ clGetPlatformIDs(0, nullptr, &num) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return decltype(size())(num);
    }

    always_inline static std::expected<std::vector<cl_platform_id>, status> get_ids() noexcept {
        return decltype(get_ids())(get_ids(size().value()));
    }

    always_inline static std::expected<std::vector<ocl::platform>, status> get() noexcept {
        return decltype(get())(get(size().value()));
    }

    always_inline static void each(auto lambda) noexcept {
        ::each(get().value(), lambda);
    }

    template<cl_device_type device_type = CL_DEVICE_TYPE_DEFAULT>
    always_inline std::expected<cl_uint, status> devices_size() const noexcept {
        cl_uint num{};
        status sts0{ clGetDeviceIDs(platform, device_type, 0, nullptr, &num) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<cl_uint, status>(num);
    }

    template<cl_device_type device_type = CL_DEVICE_TYPE_DEFAULT>
    always_inline std::expected<std::vector<ocl::device>, status> get_devices() noexcept {
        std::vector<ocl::device> ret(static_cast<size_t>(devices_size<device_type>().value()));
        status sts1{ clGetDeviceIDs(platform, device_type, static_cast<cl_uint>(ret.size()), reinterpret_cast<cl_device_id*>(ret.data()), nullptr) };
        if (sts1 != status::SUCCESS) return std::unexpected(sts1);

        return std::expected<std::vector<ocl::device>, status>(ret);
    }

    template<cl_device_type device_type = CL_DEVICE_TYPE_DEFAULT>
    always_inline void each_device(auto lambda) noexcept {
        ::each(get_devices<device_type>().value(), lambda);
    }

};

};