#pragma once

#include <cstdint>
#include <array>
#include <cmath>
#include <expected>
#include <iostream>
#include <set>
#include <span>
#include <vector>
#include <map>
#include <thread>
#include "each.h"

using ushort = uint16_t;
using uint = uint32_t;
using ulong = uint64_t;

using float2 = std::array<float, 2>;
using ushort2 = std::array<ushort, 2>;
using uint2 = std::array<uint, 2>;
using uint3 = std::array<uint, 3>;
using uint4 = std::array<uint, 4>;
using ulong2 = std::array<ulong, 2>;
using ulong16 = std::array<ulong, 16>;

#define make_float2(arg0, arg1) float2{(float)arg0, (float)arg1}
#define make_ushort2(arg0, arg1) ushort2{(ushort)arg0, (ushort)arg1}
#define make_uint2(arg0, arg1) uint2{(uint)arg0, (uint)arg1}
#define make_uint4(arg0, arg1, arg2, arg3) uint4{(uint)arg0, (uint)arg1, (uint)arg2, (uint)arg3}
#define make_ulong2(arg0, arg1) ulong2{(ulong)arg0, (ulong)arg1}
#define make_uint3(arg0, arg1, arg2) uint3{(uint)arg0, (uint)arg1, (uint)arg2}

inline thread_local std::array<size_t, 3> global_id;
#define get_global_id(arg) global_id.at(0)
#define __attribute__(arg)

#ifndef make_ushort2
#define make_ushort2(arg0, arg1) (ushort2)(arg0, arg1)
#endif

#ifndef make_uint2
#define make_uint2(arg0, arg1) (uint2)(arg0, arg1)
#endif

#ifndef make_uint3
#define make_uint3(arg0, arg1, arg2) (uint3)(arg0, arg1, arg2)
#endif

#ifndef convert_float2
#define convert_float2(arg) make_float2((float)arg[0], (float)arg[1])
#endif

#define min(a, b) ((a) < ((b))?(a):(b))
#define max(a, b) ((a) > ((b))?(a):(b))
#define distance(a, b) sqrt((a[0] - b[0])*(a[0] - b[0]))+((a[1] - b[1])*(a[1] - b[1]))

#define ocl_counter_max 256
#define constant const
#define restrict
#define global
#define kernel

typedef ushort2 routed_lines[ocl_counter_max];

ushort2 best_next_tile(ushort2 sourcePos, constant uint2* restrict tile_tile_count_offset, constant ushort2* restrict dest_tile, global ulong16* restrict stubLocations);
#include "kernels/draw_wires.cl"

#undef ocl_counter_max
#undef kernel

#undef min
#undef max
#undef distance

#undef make_float2
#undef make_ushort2
#undef make_uint2
#undef make_uint4
#undef make_ulong2
#undef make_uint3
#undef get_global_id
#undef __attribute__

#define CL_TARGET_OPENCL_VERSION 300

/* OpenCL Version */
#if CL_TARGET_OPENCL_VERSION >= 300 && !defined(CL_VERSION_3_0)
#define CL_VERSION_3_0  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 220 && !defined(CL_VERSION_2_2)
#define CL_VERSION_2_2  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 210 && !defined(CL_VERSION_2_1)
#define CL_VERSION_2_1  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 200 && !defined(CL_VERSION_2_0)
#define CL_VERSION_2_0  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 120 && !defined(CL_VERSION_1_2)
#define CL_VERSION_1_2  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 110 && !defined(CL_VERSION_1_1)
#define CL_VERSION_1_1  1
#endif
#if CL_TARGET_OPENCL_VERSION >= 100 && !defined(CL_VERSION_1_0)
#define CL_VERSION_1_0  1
#endif

#define always_inline inline

using cl_device_id = uint32_t;
using cl_uint = uint32_t;
using cl_ulong = uint64_t;
using cl_context_properties = uint32_t;
using cl_device_type = uint32_t;
using cl_device_info = uint32_t;
using cl_program_build_info = uint32_t;
using cl_mem_flags = uint32_t;

#define CL_DEVICE_MAX_MEM_ALLOC_SIZE                     0x1010

/* cl_device_type - bitfield */
#define CL_DEVICE_TYPE_DEFAULT                      (1 << 0)
#define CL_DEVICE_TYPE_CPU                          (1 << 1)
#define CL_DEVICE_TYPE_GPU                          (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR                  (1 << 3)
#ifdef CL_VERSION_1_2
#define CL_DEVICE_TYPE_CUSTOM                       (1 << 4)
#endif
#define CL_DEVICE_TYPE_ALL                          0xFFFFFFFF

/* cl_program_build_info */
#define CL_PROGRAM_BUILD_STATUS                     0x1181
#define CL_PROGRAM_BUILD_OPTIONS                    0x1182
#define CL_PROGRAM_BUILD_LOG                        0x1183
#ifdef CL_VERSION_1_2
#define CL_PROGRAM_BINARY_TYPE                      0x1184
#endif
#ifdef CL_VERSION_2_0
#define CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE 0x1185
#endif

/* cl_mem_flags and cl_svm_mem_flags - bitfield */
#define CL_MEM_READ_WRITE                           (1 << 0)
#define CL_MEM_WRITE_ONLY                           (1 << 1)
#define CL_MEM_READ_ONLY                            (1 << 2)
#define CL_MEM_USE_HOST_PTR                         (1 << 3)
#define CL_MEM_ALLOC_HOST_PTR                       (1 << 4)
#define CL_MEM_COPY_HOST_PTR                        (1 << 5)
/* reserved                                         (1 << 6)    */
#ifdef CL_VERSION_1_2
#define CL_MEM_HOST_WRITE_ONLY                      (1 << 7)
#define CL_MEM_HOST_READ_ONLY                       (1 << 8)
#define CL_MEM_HOST_NO_ACCESS                       (1 << 9)
#endif
#ifdef CL_VERSION_2_0
#define CL_MEM_SVM_FINE_GRAIN_BUFFER                (1 << 10)   /* used by cl_svm_mem_flags only */
#define CL_MEM_SVM_ATOMICS                          (1 << 11)   /* used by cl_svm_mem_flags only */
#define CL_MEM_KERNEL_READ_AND_WRITE                (1 << 12)
#endif


namespace ocl {

	using status = int32_t;

	class context;
	class program;
	class buffer;
	class queue;
	class kernel;
	class device;

	class buffer {
	public:
		void* host_ptr;
	};



	class kernel {
	public:

		always_inline std::expected<void, status> set_arg_t(cl_uint arg_index, auto arg) noexcept {
			if (arg_index == 0) kernel_arg0 = arg;
			return std::expected<void, status>();
		}
		always_inline std::expected<void, status> set_arg(cl_uint arg_index, ocl::buffer buf) noexcept {
			if (arg_index == 1) kernel_arg1 = reinterpret_cast<decltype(kernel_arg1)>(buf.host_ptr);
			if (arg_index == 2) kernel_arg2 = reinterpret_cast<decltype(kernel_arg2)>(buf.host_ptr);
			if (arg_index == 3) kernel_arg3 = reinterpret_cast<decltype(kernel_arg3)>(buf.host_ptr);
			if (arg_index == 4) kernel_arg4 = reinterpret_cast<decltype(kernel_arg4)>(buf.host_ptr);
			if (arg_index == 5) kernel_arg5 = reinterpret_cast<decltype(kernel_arg5)>(buf.host_ptr);
			if (arg_index == 6) kernel_arg6 = reinterpret_cast<decltype(kernel_arg6)>(buf.host_ptr);
			return std::expected<void, status>();
		}
		uint kernel_arg0;// count,
		global routed_lines* restrict kernel_arg1; // routed,
		global uint4* restrict kernel_arg2; // drawIndirect,
		global ulong16* restrict kernel_arg3; // stubLocations, // cost: 13 bit, previous: 12, tt_id:20bit, tile:19bit
		constant uint2* restrict kernel_arg4; // tile_tile_count_offset,
		constant ushort2* restrict kernel_arg5; // dest_tile,
		constant ushort2* restrict kernel_arg6; // allSourcePos
		void exec(size_t x, size_t y, size_t z) {
			global_id = { x, y, z };
			draw_wires(
				kernel_arg0,
				kernel_arg1,
				kernel_arg2,
				kernel_arg3,
				kernel_arg4,
				kernel_arg5,
				kernel_arg6
			);
		}
	};


	class device {
	public:
		static std::expected<std::vector<cl_device_id>, ocl::status> get_gl_devices(std::span<cl_context_properties> context_properties) {
			std::vector<cl_device_id> ret;
			return ret;
		}
		template<typename cl_integral>
		always_inline static std::expected<cl_integral, status> get_info_integral(cl_device_id device, cl_device_info param_name) noexcept {
			if (param_name == CL_DEVICE_MAX_MEM_ALLOC_SIZE) return UINT32_MAX;
			return cl_integral{};
		}
	};

	class program {
	public:
		std::expected<void, status> build(std::string options) noexcept {
			return std::expected<void, status>();
		}
		always_inline std::expected<std::vector<std::string>, status> get_build_info_string(cl_program_build_info param_name) const noexcept {
			return std::vector<std::string>{};
		}
		always_inline std::expected<std::vector<ocl::kernel>, status> create_kernels() noexcept {
			return std::vector<ocl::kernel>{ocl::kernel{}};
		}
	};

	class queue {
	public:
		template<cl_uint work_dim>
		always_inline std::expected<void, status> enqueue_no_event(ocl::kernel kernel, std::array<size_t, work_dim> global_work_offset, std::array<size_t, work_dim> global_work_size, std::array<size_t, work_dim> local_work_size) {
			if constexpr (work_dim == 3) {
				for (size_t z{ global_work_offset.at(2) }; z < global_work_size.at(2); z++) {
					for (size_t y{ global_work_offset.at(1) }; y < global_work_size.at(1); y++) {
						for (size_t x{ global_work_offset.at(0) }; x < global_work_size.at(0); x++) {
							kernel.exec(x, y, z);
						}
					}
				}
			}
			if constexpr (work_dim == 2) {
				for (size_t y{ global_work_offset.at(1) }; y < global_work_size.at(1); y++) {
					for (size_t x{ global_work_offset.at(0) }; x < global_work_size.at(0); x++) {
						kernel.exec(x, y, 0);
					}
				}
			}
			if constexpr (work_dim == 1) {
				uint64_t group_size{ std::thread::hardware_concurrency() };
				std::vector<std::jthread> threads;
				threads.reserve(group_size);
				for (size_t thread_offset{}; thread_offset < group_size; thread_offset++) {
					threads.emplace_back([thread_offset, &global_work_offset, &global_work_size, &group_size, &kernel]() {
						size_t thread_chunk_size{ (global_work_size.at(0) - global_work_offset.at(0)) / group_size };
						size_t begin_count{ global_work_offset.at(0) + thread_chunk_size * thread_offset };
						size_t end_count{ global_work_offset.at(0) + thread_chunk_size * (thread_offset + 1ull) };

						printf("global_work_size(0) = %I64u, thread_chunk_size = %I64u, begin_count = %I64u, end_count = %I64u\n",
							global_work_size.at(0),
							thread_chunk_size,
							begin_count,
							end_count
						);

						for (size_t x{ begin_count }; x < end_count; x++) {
							kernel.exec(x, 0, 0);
						}
					});
				}
			}
			return std::expected<void, status>();
		}
		always_inline std::expected<void, status> useGL(std::span<ocl::buffer> buffers, auto lambda) noexcept {
			return std::expected<void, status>();
		}
		always_inline std::expected<void, status> finish() {
			return std::expected<void, status>();
		}
	};

	class context {
	public:
		always_inline static std::expected<ocl::context, status> create(std::span<cl_context_properties> context_properties, std::span<cl_device_id> devices) noexcept {
			return ocl::context{};
		}
		template<cl_device_type device_type = CL_DEVICE_TYPE_ALL>
		always_inline static std::expected<ocl::context, status> create(std::span<cl_context_properties> context_properties = {}) noexcept {
			return ocl::context{};
		}
		always_inline std::expected<std::vector<cl_device_id>, status> get_devices() const noexcept {
			return std::vector<cl_device_id>{0};
		}
		always_inline std::expected<std::vector<ocl::queue>, status> create_queues() noexcept {
			return std::vector<ocl::queue>{ocl::queue{}};
		}
		always_inline std::expected<ocl::program, status> create_program(std::string_view source) noexcept {
			return ocl::program{};
		}
		always_inline std::expected<ocl::program, status> create_program(std::span<char> source) noexcept {
			return ocl::program{};
		}
		always_inline static std::expected<buffer, status> from_gl(ocl::context context, cl_mem_flags flags, cl_uint bufobj) {
			return ocl::buffer{};
		}
		always_inline std::expected<buffer, status> from_gl(cl_mem_flags flags, cl_uint bufobj) {
			return ocl::buffer{};
		}
		always_inline static std::expected<std::vector<ocl::buffer>, status> from_gl(ocl::context context, cl_mem_flags flags, std::span<cl_uint> bufobjs) {
			return std::vector<ocl::buffer>{};
		}
		always_inline std::expected<std::vector<ocl::buffer>, status> from_gl(cl_mem_flags flags, std::span<cl_uint> bufobjs) {
			return std::vector<ocl::buffer>{};
		}
		always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, size_t size) noexcept {
			auto ptr{ malloc(size) };
			memset(ptr, 0, size);
			return ocl::buffer{ ptr };
		}

		template<typename T>
		always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, std::span<T> host) noexcept {
			return ocl::buffer{ .host_ptr{host.data()} };
		}

		template<typename T>
		always_inline std::expected<ocl::buffer, status> create_buffer(cl_mem_flags flags, std::vector<T>& host) noexcept {
			return ocl::buffer{ .host_ptr{host.data()} };
		}

	};

};

#undef always_inline
#undef constant
#undef restrict
#undef global
