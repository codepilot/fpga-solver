#pragma once

#include <span>

namespace ocl {
class program {
public:
	cl_program program;
    cl_device_id device;

    always_inline static std::expected<ocl::program, status> create(cl_context context, cl_device_id device, std::span<char> source) noexcept {
        cl_int errcode_ret{};
        const char* strings{ source.data() };
        const size_t string_lengths{ source.size() };

        cl_program program{ clCreateProgramWithSource(context, 1, &strings, &string_lengths, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::program, status>(ocl::program{ .program{program}, .device{device} });
    }

    always_inline static std::expected<ocl::program, status> create(cl_context context, cl_device_id device, std::string_view source) noexcept {
        cl_int errcode_ret{};
        const char* strings{ source.data() };
        const size_t string_lengths{ source.size() };

        cl_program program{ clCreateProgramWithSource(context, 1, &strings, &string_lengths, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::program, status>(ocl::program{ .program{program}, .device{device} });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_program program, cl_program_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetProgramInfo(program, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_program_info param_name) const noexcept {
        return get_info_size(program, param_name);
    }

    always_inline static std::expected<std::string, status> get_info_string(cl_program program, cl_program_info param_name) noexcept {
        return get_info_size(program, param_name).and_then([&](size_t size) -> std::expected<std::string, status> {
            std::string string_pointer(size, 0);

            status sts1{ clGetProgramInfo(program, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<std::string, status>(string_pointer);
        });
    }

    always_inline std::expected<std::string, status> get_info_string(cl_program_info param_name) const noexcept {
        return get_info_string(program, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_program program, cl_program_info param_name) noexcept {
        return get_info_size(program, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetProgramInfo(program, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_program_info param_name) const noexcept {
        return get_info_integral<cl_integral>(program, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_PROGRAM_REFERENCE_COUNT);
    }

    auto built() {
        puts("built");
    }

    std::expected<void, status> build() noexcept {
        cl_int errcode_ret{ clBuildProgram(
            program,
            1,
            &device,
            nullptr,
            nullptr/* [](cl_program program, void* user_data)-> void {
                reinterpret_cast<ocl::program*>(user_data)->built();
            } */, nullptr /*this*/)};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    std::expected<void, status> build(std::string options) noexcept {
        cl_int errcode_ret{ clBuildProgram(
            program,
            1,
            &device,
            options.c_str(),
            nullptr/* [](cl_program program, void* user_data)-> void {
                reinterpret_cast<ocl::program*>(user_data)->built();
            } */, nullptr /*this*/)};
        
        if (errcode_ret) {
            std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }



    always_inline static std::expected<size_t, status> get_build_info_size(cl_program program, cl_device_id device, cl_program_build_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetProgramBuildInfo(program, device, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_build_info_size(cl_program_build_info param_name) const noexcept {
        return get_build_info_size(program, device, param_name);
    }

    always_inline static std::expected<std::string, status> get_build_info_string(cl_program program, cl_device_id device, cl_program_build_info param_name) noexcept {
        return get_build_info_size(program, device, param_name).and_then([&](size_t size) -> std::expected<std::string, status> {
            std::string string_pointer(size, 0);

            status sts1{ clGetProgramBuildInfo(program, device, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<std::string, status>(string_pointer);
            });
    }

    always_inline std::expected<std::string, status> get_build_info_string(cl_program_build_info param_name) const noexcept {
        return get_build_info_string(program, device, param_name);
    }

    always_inline std::expected<ocl::kernel, status> create_kernel(std::string kernel_name) noexcept {
        return ocl::kernel::create(program, kernel_name);
    }
};
};