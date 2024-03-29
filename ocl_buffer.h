#pragma once

namespace ocl {
class buffer : public ocl_handle::shared_handle<cl_mem> {
public:
    always_inline static std::span<cl_mem> get_buffer_mems(std::span<ocl::buffer> buffers) {
        return std::span<cl_mem>(&buffers[0].m_ptr, buffers.size());
    }

    always_inline static std::expected<size_t, status> size_bytes(ocl::buffer &buffer) {
        return buffer.get_info_integral<size_t>(CL_MEM_SIZE);
    }

    always_inline std::expected<size_t, status> size_bytes() {
        return size_bytes(*this);
    }

    always_inline static std::expected<size_t, status> sum_size_bytes(std::span<ocl::buffer> buffers) {
        size_t total_size{};
        for (auto&& buffer: buffers) {
            auto result{ buffer.get_info_integral<size_t>(CL_MEM_SIZE) };
            if (!result.has_value()) {
                return std::unexpected<status>(result.error());
            }
            total_size += result.value();
        }
        return total_size;
    }

    always_inline static std::expected<ocl::buffer, status> sub_region(ocl::buffer &buf, cl_mem_flags flags, cl_buffer_region region) {
        cl_int errcode_ret{};
#ifdef _DEBUG
        puts(std::format("clCreateBuffer size:{} MiB", std::scalbln(static_cast<double>(region.size), -20)).c_str());
#endif
        auto _ptr{ clCreateSubBuffer(buf.m_ptr, flags, CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode_ret) };
#ifdef _DEBUG
        puts(std::format("clCreateBuffer status:{}", errcode_ret).c_str());
#endif
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::buffer, status>(ocl::buffer{ _ptr });
    }

    always_inline std::expected<ocl::buffer, status> sub_region(cl_mem_flags flags, cl_buffer_region region) {
        return sub_region(*this, flags, region);
    }

    always_inline static std::expected<ocl::buffer, status> sub_region(ocl::buffer& buf, cl_buffer_region region) {
        return sub_region(buf, 0, region);
    }

    always_inline std::expected<ocl::buffer, status> sub_region(cl_buffer_region region) {
        return sub_region(*this, region);
    }

    always_inline static std::expected<ocl::buffer, status> create(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr) noexcept {
        cl_int errcode_ret{};
#ifdef _DEBUG
        puts(std::format("clCreateBuffer size:{} MiB", std::scalbln(static_cast<double>(size), -20)).c_str());
#endif
        cl_mem _ptr{ clCreateBuffer(context, flags, size, host_ptr, &errcode_ret) };
#ifdef _DEBUG
        puts(std::format("clCreateBuffer status:{}", errcode_ret).c_str());
#endif
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::buffer, status>(ocl::buffer{ _ptr });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_mem mem, cl_mem_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetMemObjectInfo(mem, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_mem_info param_name) const noexcept {
        return get_info_size(m_ptr, param_name);
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
        return get_info_integral<cl_integral>(m_ptr, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_MEM_REFERENCE_COUNT);
    }

};
};
static_assert(sizeof(ocl::buffer) == sizeof(cl_mem));
