#pragma once

namespace ocl {
class buffer {
public:
	cl_mem mem;
    always_inline static std::expected<ocl::buffer, status> create(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr) noexcept {
        cl_int errcode_ret{};
#if 0
        cl_mem mem{ clCreateBufferWithProperties(context, nullptr, flags, size, host_ptr, &errcode_ret) };
#else
        cl_mem mem{ clCreateBuffer(context, flags, size, host_ptr, &errcode_ret) };
#endif
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::buffer, status>(ocl::buffer{ .mem{mem} });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_mem mem, cl_mem_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetMemObjectInfo(mem, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_mem_info param_name) const noexcept {
        return get_info_size(mem, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_mem mem, cl_mem_info param_name) noexcept {
        return get_info_size(mem, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetMemObjectInfo(mem, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_mem_info param_name) const noexcept {
        return get_info_integral<cl_integral>(mem, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_MEM_REFERENCE_COUNT);
    }
};
};