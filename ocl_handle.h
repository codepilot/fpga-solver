#pragma once

namespace ocl_handle {

    template<typename T> static std::expected<T, ocl::status> release(T _ptr) noexcept;
    template<typename T> static std::expected<T, ocl::status> retain(T _ptr) noexcept;

    template<> static std::expected<cl_context, ocl::status> release(cl_context _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseContext(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseContext(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_context, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_context, ocl::status> retain(cl_context _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainContext(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainContext(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_context, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_mem, ocl::status> release(cl_mem _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseMemObject(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseMemObject(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_mem, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_mem, ocl::status> retain(cl_mem _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainMemObject(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainMemObject(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_mem, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_kernel, ocl::status> release(cl_kernel _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseKernel(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseKernel(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_kernel, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_kernel, ocl::status> retain(cl_kernel _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainKernel(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainKernel(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_kernel, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_command_queue, ocl::status> release(cl_command_queue _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseCommandQueue(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseCommandQueue(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_command_queue, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_command_queue, ocl::status> retain(cl_command_queue _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainCommandQueue(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainCommandQueue(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_command_queue, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_program, ocl::status> release(cl_program _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseProgram(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseProgram(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_program, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_program, ocl::status> retain(cl_program _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainProgram(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainProgram(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_program, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_device_id, ocl::status> release(cl_device_id _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clReleaseDevice(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clReleaseDevice(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_device_id, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<> static std::expected<cl_device_id, ocl::status> retain(cl_device_id _ptr) noexcept {
#ifdef _DEBUG
        std::cout << std::format("clRetainDevice(0x{:x})\n", std::bit_cast<uintptr_t>(_ptr));
#endif
        ocl::status result{ clRetainDevice(_ptr) };
        if (result == ocl::status::SUCCESS) return std::expected<cl_device_id, ocl::status>(_ptr);
        return std::unexpected<ocl::status>(result);
    }

    template<typename T>
    class shared_handle {
    public:
        T m_ptr{};
        //value constructors
        shared_handle() noexcept = default;
        shared_handle(T _ptr) noexcept : m_ptr{ _ptr } { }

        //move constructors
        shared_handle(shared_handle&& other) noexcept : m_ptr{ std::exchange(other.m_ptr, nullptr) } { }
        shared_handle& operator=(shared_handle&& other) noexcept { reset(); m_ptr = std::exchange(other.m_ptr, nullptr); return *this; }

        //copy constructors
        shared_handle(const shared_handle& other) noexcept : m_ptr{ retain(other.m_ptr).value() } { }
        shared_handle& operator=(const shared_handle& other) noexcept {
            reset();
            m_ptr = retain(other.m_ptr).value();
        }

        std::expected<void, ocl::status> reset() noexcept {
            if (m_ptr == nullptr) return std::expected<void, ocl::status>();
            auto result{ release(std::exchange(this->m_ptr, nullptr)) };
            if (result.has_value()) return std::expected<void, ocl::status>();
            return std::unexpected<ocl::status>(result.error());
        }
        ~shared_handle() noexcept {
            reset().value();
        }


    };
};