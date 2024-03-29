#pragma once

namespace ocl {
class kernel : public ocl_handle::shared_handle<cl_kernel> {
public:
    always_inline std::expected<ocl::kernel, status> clone() const {
        cl_int errcode_ret{};
        cl_kernel cloned_kernel{ clCloneKernel(m_ptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::kernel, status>(ocl::kernel{ cloned_kernel });
    }

    always_inline std::expected<std::vector<ocl::kernel>, status> clone(size_t count) const {
        std::vector<ocl::kernel> ret;
        ret.reserve(count);
        for (size_t i{}; i < count; i++) {
            auto cloned_kernel{ clone() };
            if (!cloned_kernel.has_value()) return std::unexpected<status>(cloned_kernel.error());
            ret.emplace_back(std::move(cloned_kernel.value()));
        }
        return ret;
    }

    always_inline static std::expected<ocl::kernel, status> create(cl_program program, std::string kernel_name) noexcept {
        cl_int errcode_ret{};
        cl_kernel kernel{ clCreateKernel(program, kernel_name.c_str(), &errcode_ret)};
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::kernel, status>(ocl::kernel{ kernel });
    }
    always_inline static std::expected<cl_uint, status> create_count(cl_program program) noexcept {
        cl_uint num_kernels_ret{};
        ocl::status sts{ clCreateKernelsInProgram(program, 0, nullptr, &num_kernels_ret) };
        if (sts != ocl::status::SUCCESS) return std::unexpected(sts);
        return num_kernels_ret;
    }

    always_inline static std::expected<std::vector<cl_kernel>, status> create_kernels(cl_program program) noexcept {
        return create_count(program).and_then([&](cl_uint num_kernels)->std::expected<std::vector<cl_kernel>, status> {
            auto kernels{ std::vector<cl_kernel>(static_cast<size_t>(num_kernels), static_cast<cl_kernel>(nullptr)) };

            ocl::status sts{ clCreateKernelsInProgram(program, static_cast<cl_uint>(kernels.size()), kernels.data(), nullptr)};
            if (sts != ocl::status::SUCCESS) return std::unexpected(sts);
            return kernels;
        });
    }

    always_inline static std::expected<std::vector<ocl::kernel>, status> create(cl_program program) noexcept {
        return create_kernels(program).and_then([](std::vector<cl_kernel> kernels)->std::expected<std::vector<ocl::kernel>, status> {
            std::vector<ocl::kernel> ret;
            ret.reserve(kernels.size());
            for (auto&& kernel : kernels) {
                ret.emplace_back(kernel);
            }
            return ret;
        });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_kernel kernel, cl_kernel_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetKernelInfo(kernel, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_kernel_info param_name) const noexcept {
        return get_info_size(m_ptr, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_kernel kernel, cl_kernel_info param_name) noexcept {
        return get_info_size(kernel, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetKernelInfo(kernel, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_kernel_info param_name) const noexcept {
        return get_info_integral<cl_integral>(m_ptr, param_name);
    }

    always_inline static std::expected<std::string, status> get_info_string(cl_kernel kernel, cl_kernel_info param_name) noexcept {
        return get_info_size(kernel, param_name).and_then([&](size_t size) -> std::expected<std::string, status> {
            std::string string_pointer(size, 0);

            status sts1{ clGetKernelInfo(kernel, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<std::string, status>(string_pointer);
            });
    }

    always_inline std::expected<std::string, status> get_info_string(cl_kernel_info param_name) const noexcept {
        return get_info_string(m_ptr, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_KERNEL_REFERENCE_COUNT);
    }


    always_inline static std::expected<size_t, status> get_arg_info_size(cl_kernel kernel, cl_uint arg_index, cl_kernel_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetKernelArgInfo(kernel, arg_index, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_arg_info_size(cl_uint arg_index, cl_kernel_info param_name) const noexcept {
        return get_arg_info_size(m_ptr, arg_index, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_arg_info_integral(cl_kernel kernel, cl_uint arg_index, cl_kernel_info param_name) noexcept {
        return get_arg_info_size(kernel, arg_index, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetKernelArgInfo(kernel, arg_index, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
        });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_arg_info_integral(cl_uint arg_index, cl_kernel_info param_name) const noexcept {
        return get_arg_info_integral<cl_integral>(m_ptr, arg_index, param_name);
    }

    always_inline static std::expected<std::string, status> get_arg_info_string(cl_kernel kernel, cl_uint arg_index, cl_kernel_info param_name) noexcept {
        return get_arg_info_size(kernel, arg_index, param_name).and_then([&](size_t size) -> std::expected<std::string, status> {
            std::string string_pointer(size, 0);

            status sts1{ clGetKernelArgInfo(kernel, arg_index, param_name, string_pointer.size(), string_pointer.data(), 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<std::string, status>(string_pointer);
        });
    }

    always_inline std::expected<std::string, status> get_arg_info_string(cl_uint arg_index, cl_kernel_info param_name) const noexcept {
        return get_arg_info_string(m_ptr, arg_index, param_name);
    }

    always_inline std::expected<void, status> set_arg_t(cl_uint arg_index, auto arg) noexcept {
        cl_int errcode_ret{ clSetKernelArg(
            m_ptr,
            arg_index,
            sizeof(arg),
            &arg) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> set_arg(cl_uint arg_index, ocl::buffer &buf) noexcept {
        cl_int errcode_ret{ clSetKernelArg(
            m_ptr,
            arg_index,
            sizeof(cl_mem),
            &buf.m_ptr) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> set_arg(cl_uint arg_index, ocl::svm<T> svm) noexcept {
        cl_int errcode_ret{ clSetKernelArgSVMPointer(
            m_ptr,
            arg_index,
            svm.data()) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }






    always_inline static std::expected<size_t, status> get_work_group_info_size(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetKernelWorkGroupInfo(kernel, device, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_work_group_info_size(cl_kernel_work_group_info param_name, cl_device_id device) const noexcept {
        return get_work_group_info_size(m_ptr, device, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_work_group_info_integral(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name) noexcept {
        return get_work_group_info_size(kernel, device, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetKernelWorkGroupInfo(kernel, device, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_work_group_info_integral(cl_device_id device, cl_kernel_work_group_info param_name) const noexcept {
        return get_work_group_info_integral<cl_integral>(m_ptr, device, param_name);
    }
};
};