#pragma once

namespace ocl {
class queue {
public:
	cl_command_queue queue;
    always_inline static std::expected<ocl::queue, status> create(cl_context context, cl_device_id device) noexcept {
        cl_int errcode_ret{};
        cl_command_queue queue{ clCreateCommandQueueWithProperties(context, device, nullptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::queue, status>(ocl::queue{ .queue{queue} });
    }

    always_inline static std::expected<ocl::queue, status> create(cl_context context, cl_device_id device, std::vector<cl_queue_properties> properties) noexcept {
        cl_int errcode_ret{};
        cl_command_queue queue{ clCreateCommandQueueWithProperties(context, device, properties.data(), &errcode_ret)};
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::queue, status>(ocl::queue{ .queue{queue} });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_command_queue queue, cl_command_queue_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetCommandQueueInfo(queue, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_command_queue_info param_name) const noexcept {
        return get_info_size(queue, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_command_queue queue, cl_command_queue_info param_name) noexcept {
        return get_info_size(queue, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetCommandQueueInfo(queue, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_command_queue_info param_name) const noexcept {
        return get_info_integral<cl_integral>(queue, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_QUEUE_REFERENCE_COUNT);
    }

    template<cl_uint work_dim>
    always_inline std::expected<void, status> enqueue(cl_kernel kernel, std::array<size_t, work_dim> global_work_offset, std::array<size_t, work_dim> global_work_size, std::array<size_t, work_dim> local_work_size) {
        cl_int errcode_ret{ clEnqueueNDRangeKernel(
            queue,
            kernel,
            work_dim,
            global_work_offset.data(),
            global_work_size.data(),
            local_work_size.data(),
            0,
            nullptr,
            nullptr
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueRead(cl_mem buffer, cl_bool blocking_read, size_t offset, std::span<T> dest) {
        cl_int errcode_ret{ clEnqueueReadBuffer(
            queue,
            buffer,
            blocking_read,
            offset,
            dest.size_bytes(),
            dest.data(),
            0,
            nullptr,
            nullptr
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMemFill(std::span<T> dst, const T &pattern) {
        cl_int errcode_ret{ clEnqueueSVMMemFill(
            queue,
            dst.data(),
            &pattern,
            sizeof(pattern),
            dst.size_bytes(),
            0,
            nullptr,
            nullptr
        ) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMemcpy(cl_bool blocking_copy, std::span<T> dst, std::span<T> src) {
        cl_int errcode_ret{ clEnqueueSVMMemcpy(
            queue,
            blocking_copy,
            dst.data(),
            src.data(),
            dst.size_bytes(),
            0,
            nullptr,
            nullptr
        ) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMigrate(ocl::svm<T> svm) {
        if (svm.empty()) {
            return std::expected<void, status>();
        }
        const void* svm_pointer{ svm.data() };

        cl_int errcode_ret{ clEnqueueSVMMigrateMem(
            queue,
            1,
            &svm_pointer,
            nullptr,
            0,
            0,
            nullptr,
            nullptr
        ) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMigrate(std::span<ocl::svm<T>> v_svm) {
        std::vector<const void *> svm_pointers;
        svm_pointers.reserve(v_svm.size());
        for (auto&& svm_n : v_svm) {
            svm_pointers.emplace_back(svm_n.data());
        }

        cl_int errcode_ret{ clEnqueueSVMMigrateMem(
            queue,
            svm_pointers.size(),
            svm_pointers.data(),
            nullptr,
            0,
            0,
            nullptr,
            nullptr
        ) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMap(cl_bool blocking_map, cl_map_flags flags, ocl::svm<T> svm) {
        cl_int errcode_ret{ clEnqueueSVMMap(
            queue,
            blocking_map,
            flags,
            svm.data(),
            svm.size_bytes(),
            0,
            nullptr,
            nullptr
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMUnmap(ocl::svm<T> svm) {
        cl_int errcode_ret{ clEnqueueSVMUnmap(
            queue,
            svm.data(),
            0,
            nullptr,
            nullptr
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    template<typename T>
    always_inline std::expected<void, status> enqueueSVMMap(cl_map_flags flags, ocl::svm<T> svm, auto lambda) {
        if (!svm.empty()) {
            cl_int map_errcode_ret{ clEnqueueSVMMap(
                queue,
                true,
                flags,
                svm.data(),
                svm.size_bytes(),
                0,
                nullptr,
                nullptr
            ) };
            if (map_errcode_ret) {
                return std::unexpected<status>(status{ map_errcode_ret });
            }
        }
        lambda();
        if (!svm.empty()) {
            cl_int unmap_errcode_ret{ clEnqueueSVMUnmap(
                queue,
                svm.data(),
                0,
                nullptr,
                nullptr
            ) };

            if (unmap_errcode_ret) {
                return std::unexpected<status>(status{ unmap_errcode_ret });
            }
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> flush() {
        cl_int errcode_ret{ clFlush(queue) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> finish() {
        cl_int errcode_ret{ clFinish(queue) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> acquireGL(std::span<ocl::buffer> buffers) noexcept {
        ocl::status sts{ clEnqueueAcquireGLObjects(queue, static_cast<cl_uint>(buffers.size()), reinterpret_cast<cl_mem*>(buffers.data()), 0, nullptr, 0) };
        if (sts != ocl::status::SUCCESS) {
            return std::unexpected(sts);
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> releaseGL(std::span<ocl::buffer> buffers) noexcept {
        ocl::status sts{ clEnqueueReleaseGLObjects(queue, static_cast<cl_uint>(buffers.size()), reinterpret_cast<cl_mem*>(buffers.data()), 0, nullptr, 0) };
        if (sts != ocl::status::SUCCESS) {
            return std::unexpected(sts);
        }
        return std::expected<void, status>();
    }

    always_inline std::expected<void, status> useGL(std::span<ocl::buffer> buffers, auto lambda) noexcept {
        return acquireGL(buffers).and_then([&]()->std::expected<void, status> {
            lambda();
            return releaseGL(buffers);
        });
    }

};
};