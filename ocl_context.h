#pragma once

namespace ocl {
class context {
public:
	cl_context context;
    cl_device_id device;
#if 0
    ~context() {
        clReleaseContext(context);
    }
#endif
    always_inline static std::expected<ocl::context, status> create_context(cl_device_id device) noexcept {
        cl_int errcode_ret{};
        cl_context context{ clCreateContext(nullptr, 1, &device, [](const char* errinfo, const void* private_info, size_t cb, void* user_data) {
            puts(errinfo);
        }, nullptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::context, status>(ocl::context{ .context{context}, .device{device} });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_context context, cl_context_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetContextInfo(context, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_context_info param_name) const noexcept {
        return get_info_size(context, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_context context, cl_context_info param_name) noexcept {
        return get_info_size(context, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetContextInfo(context, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_context_info param_name) const noexcept {
        return get_info_integral<cl_integral>(context, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_CONTEXT_REFERENCE_COUNT);
    }

    always_inline std::expected<ocl::command_queue, status> create_command_queue() noexcept {
        return command_queue::create_command_queue(context, device);
    }

    always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, size_t size, void* host_ptr=nullptr) noexcept {
        return buffer::create_buffer(context, flags, size, host_ptr);
    }

};
};