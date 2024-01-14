#pragma once

namespace ocl {
class event {
public:
	cl_event event;
    always_inline static std::expected<size_t, status> get_info_size(cl_event event, cl_event_info param_name) noexcept {
        size_t param_value_size_ret{};
        status sts0{ clGetEventInfo(event, param_name, 0, nullptr, &param_value_size_ret) };
        if (sts0 != status::SUCCESS) return std::unexpected(sts0);
        return std::expected<size_t, status>(param_value_size_ret);
    }
    always_inline std::expected<size_t, status> get_info_size(cl_event_info param_name) const noexcept {
        return get_info_size(event, param_name);
    }

    template<typename cl_integral>
    always_inline static std::expected<cl_integral, status> get_info_integral(cl_event event, cl_event_info param_name) noexcept {
        return get_info_size(event, param_name).and_then([&](size_t size) -> std::expected<cl_integral, status> {
            cl_integral integral_pointer{};

            status sts1{ clGetEventInfo(event, param_name, sizeof(integral_pointer), &integral_pointer, 0) };
            if (sts1 != status::SUCCESS) return std::unexpected(sts1);

            return std::expected<cl_integral, status>(integral_pointer);
        });
    }

    template<typename cl_integral>
    always_inline std::expected<cl_integral, status> get_info_integral(cl_event_info param_name) const noexcept {
        return get_info_integral<cl_integral>(event, param_name);
    }

    std::expected<cl_uint, status> get_reference_count() {
        return get_info_integral<cl_uint>(CL_EVENT_REFERENCE_COUNT);
    }

    std::expected<void, status> wait() {
        status sts{clWaitForEvents(1, &event)};
        if (sts != status::SUCCESS) return std::unexpected(sts);
        return std::expected<void, status>();
    }
};
};