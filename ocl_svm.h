#pragma once

namespace ocl {
template<typename T>
class svm: public std::span<T> {
public:
    cl_context context;
    always_inline static std::expected<ocl::svm<T>, status> alloc(cl_context context, cl_svm_mem_flags flags, size_t size, cl_uint alignment=0) noexcept {
        auto ptr{ clSVMAlloc(context, flags, size, alignment) };
        if (!ptr) {
            return std::unexpected<status>(status{ ocl::status::INVALID_VALUE });
        }
        return std::expected<ocl::svm<T>, status>(std::span<T>(reinterpret_cast<T *>(ptr), size / sizeof(T)));
    }
};
};