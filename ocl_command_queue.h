#pragma once

namespace ocl {
class command_queue {
public:
	cl_command_queue command_queue;
    always_inline static std::expected<ocl::command_queue, status> create(cl_context context, cl_device_id device) noexcept {
        cl_int errcode_ret{};
        cl_command_queue command_queue{ clCreateCommandQueueWithProperties(context, device, nullptr, &errcode_ret) };
        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::command_queue, status>(ocl::command_queue{ .command_queue{command_queue} });
    }

    always_inline static std::expected<size_t, status> get_info_size(cl_command_queue command_queue, cl_command_queue_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetCommandQueueInfo(command_queue, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_command_queue_info param_name) const noexcept {
        return get_info_size(command_queue, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_command_queue command_queue, cl_command_queue_info param_name) noexcept {
        return get_info_size(command_queue, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetCommandQueueInfo(command_queue, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
            });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_command_queue_info param_name) const noexcept {
        return get_info_integral<cl_integral>(command_queue, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_QUEUE_REFERENCE_COUNT);
    }
};
};