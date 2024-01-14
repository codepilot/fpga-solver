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
    always_inline std::expected<ocl::event, status> enqueue(cl_kernel kernel, std::array<size_t, work_dim> global_work_offset, std::array<size_t, work_dim> global_work_size, std::array<size_t, work_dim> local_work_size) {
        cl_event event{};
        cl_int errcode_ret{ clEnqueueNDRangeKernel(
            queue,
            kernel,
            work_dim,
            global_work_offset.data(),
            global_work_size.data(),
            local_work_size.data(),
            0,
            nullptr,
            &event
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::event, status>(ocl::event{ .event{event} });
    }

    template<typename T>
    always_inline std::expected<ocl::event, status> enqueueRead(cl_mem buffer, cl_bool blocking_read, size_t offset, std::span<T> dest, std::vector<cl_event> event_wait_list) {
        cl_event event{};
        cl_int errcode_ret{ clEnqueueReadBuffer(
            queue,
            buffer,
            blocking_read,
            offset,
            dest.size_bytes(),
            dest.data(),
            static_cast<cl_uint>(event_wait_list.size()),
            event_wait_list.data(),
            &event
        )};

        if (errcode_ret) {
            return std::unexpected<status>(status{ errcode_ret });
        }
        return std::expected<ocl::event, status>(ocl::event {.event{ event } });
    }


};
};