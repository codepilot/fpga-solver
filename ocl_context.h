#pragma once

namespace ocl {
class context {
public:
	cl_context context;
#if 0
    ~context() {
        clReleaseContext(context);
    }
#endif

    always_inline static std::expected<ocl::context, status> create(std::span<cl_device_id> devices) noexcept {
        cl_int errcode_ret{};
        cl_context context{ clCreateContext(nullptr, static_cast<cl_uint>(devices.size()), devices.data(), [](const char* errinfo, const void* private_info, size_t cb, void* user_data) {
            puts(errinfo);
        }, nullptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::context, status>(ocl::context{ .context{context} });
    }


    template<cl_device_type device_type = CL_DEVICE_TYPE_ALL>
    always_inline static std::expected<ocl::context, status> create(std::span<cl_context_properties> context_properties = {}) noexcept {
        cl_int errcode_ret{};
        cl_context context{ clCreateContextFromType(context_properties.size()?context_properties.data():nullptr, device_type, [](const char* errinfo, const void* private_info, size_t cb, void* user_data) {
            puts(errinfo);
        }, nullptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::context, status>(ocl::context{ .context{context} });
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

    always_inline std::expected<ocl::queue, status> create_queue(cl_device_id device) noexcept {
        return queue::create(context, device);
    }

    always_inline std::expected<std::vector<ocl::queue>, status> create_queues(std::vector<cl_device_id> devices) noexcept {
        std::vector<ocl::queue> queues;
        for (auto&& device : devices) {
            queues.emplace_back(queue::create(context, device).value());
        }
        return queues;
    }

    always_inline std::expected<std::vector<cl_device_id>, status> get_devices() const noexcept {
        return get_info_integral<cl_uint>(CL_CONTEXT_NUM_DEVICES).and_then([&](cl_uint num_devices)->std::expected<std::vector<cl_device_id>, status> {
            auto devices{ std::vector<cl_device_id>(static_cast<size_t>(num_devices), static_cast<cl_device_id>(nullptr)) };
            std::span<cl_device_id> s_devices{devices};
            ocl::status sts{ clGetContextInfo(context, CL_CONTEXT_DEVICES, s_devices.size_bytes(), s_devices.data(), nullptr) };
            if (sts != ocl::status::SUCCESS) return std::unexpected(sts);
            return devices;
        });
    }

    always_inline std::expected<std::vector<ocl::queue>, status> create_queues() noexcept {
        return get_devices().and_then([this](std::vector<cl_device_id> devices) {return create_queues(devices); });
    }

    always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, size_t size, void* host_ptr=nullptr) noexcept {
        return buffer::create(context, flags, size, host_ptr);
    }

    template<typename T>
    always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, std::span<T> host) noexcept {
        return buffer::create(context, flags, host.size_bytes(), host.data());
    }

    always_inline std::expected<ocl::program, status> create_program(std::string_view source) noexcept {
        return program::create(context, source);
    }

    always_inline std::expected<ocl::program, status> create_program(std::span<char> source) noexcept {
        return program::create(context, source);
    }

    template<typename T>
    always_inline std::expected<ocl::svm<T>, status> alloc_svm(cl_svm_mem_flags flags, size_t size, cl_uint alignment = 0) noexcept {
        return ocl::svm<T>::alloc(context, flags, size, alignment);
    }

    always_inline static std::expected<buffer, status> from_gl(ocl::context context, cl_mem_flags flags, cl_uint bufobj) {
        cl_int errcode_ret{};
        cl_mem mem{ clCreateFromGLBuffer(context.context, flags, bufobj, &errcode_ret) };

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::buffer, status>(ocl::buffer{ .mem{mem} });

    }

    always_inline std::expected<buffer, status> from_gl(cl_mem_flags flags, cl_uint bufobj) {
        return ocl::context::from_gl(*this, flags, bufobj);
    }

    always_inline static std::expected<std::vector<ocl::buffer>, status> from_gl(ocl::context context, cl_mem_flags flags, std::span<cl_uint> bufobjs) {
        std::vector<buffer> ret;
        for (auto&& bufobj : bufobjs) {
            auto mem{ from_gl(context, flags, bufobj) };
            if (mem.has_value()) {
                ret.emplace_back(std::move(mem.value()));
            }
            else {
                return std::unexpected(mem.error());
            }
        }
        return ret;
    }

    always_inline std::expected<std::vector<ocl::buffer>, status> from_gl(cl_mem_flags flags, std::span<cl_uint> bufobjs) {
        return from_gl(*this, flags, bufobjs);
    }
};
};