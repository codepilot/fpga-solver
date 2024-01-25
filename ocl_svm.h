#pragma once

namespace ocl {
template<typename T>
class svm: public std::span<T> {
public:
    cl_context context;
    always_inline static std::expected<ocl::svm<T>, status> alloc(cl_context context, cl_svm_mem_flags flags, size_t size, cl_uint alignment=0) noexcept {
        auto ptr{ clSVMAlloc(context, flags, size, alignment) };
        if (!ptr) {
            printf("clSVMAlloc %f MiB failure\n", scalbln(static_cast<double>(size), -20));
            return std::unexpected<status>(status{ ocl::status::INVALID_VALUE });
        }
#ifdef _DEBUG
        printf("clSVMAlloc %f MiB success\n", scalbln(static_cast<double>(size), -20));
#endif
        return std::expected<ocl::svm<T>, status>(std::span<T>(reinterpret_cast<T *>(ptr), size / sizeof(T)));
    }
};
};