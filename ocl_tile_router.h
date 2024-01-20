#pragma once

#include "ocl.h"
#include "MemoryMappedFile.h"
#include "InterchangeGZ.h"

class OCL_Tile_Router {
public:
    ocl::context context;
    std::vector<ocl::queue> queues;
    ocl::program program;
    std::vector<ocl::kernel> kernels;
    std::vector<ocl::buffer> buffers;
    ocl::buffer stubLocations;
    ocl::buffer tile_tile_offset_count;
    ocl::buffer dest_tile;
    PhysGZ phys;

#if 0
    OCL_Tile_Router() = default;

    OCL_Tile_Router(OCL_Tile_Router&& other): 
        context{ std::move(other.context) },
        queues{ std::move(other.queues) },
        program{ std::move(other.program) },
        kernels{ std::move(other.kernels) },
        buffers{ std::move(other.buffers) },
        stubLocations{ std::move(other.stubLocations) },
        tile_tile_offset_count{ std::move(other.tile_tile_offset_count) },
        dest_tile{ std::move(dest_tile) },
        phys{ std::move(phys) }
    {

    }
#endif

    inline static constexpr cl_uint max_workgroup_size{ 256 };
    inline static constexpr cl_uint workgroup_count{ 4096 };
    inline static constexpr cl_uint total_group_size{ max_workgroup_size * workgroup_count };

    std::expected<void, ocl::status> step() {
        return queues.at(0).useGL(buffers, [&]() {
            queues.at(0).enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }).value();
        });
    }
    static OCL_Tile_Router make(
        std::vector<cl_context_properties> context_properties = {},
        std::vector<cl_uint> gl_buffers = {}//,
    ) {
        MemoryMappedFile source{ "../kernels/draw_wires.cl" };
        auto gl_devices{ ocl::device::get_gl_devices(context_properties).value() };
        ocl::context context{ ocl::context::create(context_properties, gl_devices).value() };
        std::vector<ocl::queue> queues{ context.create_queues().value() };
        ocl::program program{ context.create_program(source.get_span<char>()).value() };
        program.build().value();
        std::vector<ocl::kernel> kernels{ program.create_kernels().value() };
        std::vector<ocl::buffer> buffers{ context.from_gl(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).value() };

        decltype(auto) kernel{ kernels.at(0) };

        std::vector<uint16_t> v_stubLocations(static_cast<size_t>(total_group_size * 4), 0);
        for (auto&& stubLocation : v_stubLocations) {
            // _rdrand16_step(&stubLocation);
            stubLocation = std::rand();
        }

        auto buf_stubLocations{ context.create_buffer<uint16_t>(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, v_stubLocations).value() };

#if 0
        std::vector<std::array<uint32_t, 2>> v_tile_tile_offset_count;
        std::vector<std::array<uint16_t, 2>> v_dest_tile;

        v_tile_tile_offset_count.reserve(670ull * 311ull);
        v_dest_tile.reserve(670ull * 311ull * 383ull);

        for (uint16_t row{}; row < 311; row++) {
            OutputDebugStringA(std::format("row: {}\n", row).c_str());
            for (uint16_t col{}; col < 670; col++) {

                uint16_t wire_count{};
                // _rdrand16_step(&wire_count);
                wire_count = std::rand();
                wire_count %= 766;

                v_tile_tile_offset_count.emplace_back(std::array<uint32_t, 2>{static_cast<uint32_t>(v_dest_tile.size()), wire_count});
                for (uint16_t wire{}; wire < wire_count; wire++) {
                    uint16_t dx{};
                    dx = std::rand();
                    //_rdrand16_step(&dx);
                    dx %= 670;

                    uint16_t dy{};
                    dy = std::rand();
                    //_rdrand16_step(&dy);
                    dy %= 311;

                    v_dest_tile.emplace_back(std::array<uint16_t, 2>{dx, dy});
                }
            }
        }

        auto buf_tile_tile_offset_count{ context.create_buffer<std::array<uint32_t, 2>>(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, v_tile_tile_offset_count).value() };
        auto buf_dest_tile{ context.create_buffer<std::array<uint16_t, 2>>(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, v_dest_tile).value() };
#else
        MemoryMappedFile mmf_a{ "tt_count_offset.bin" };
        MemoryMappedFile mmf_b{ "tt_body.bin" };
        auto span_a{ mmf_a.get_span<std::array<uint32_t, 2>>() };
        auto span_b{ mmf_b.get_span<std::array<uint16_t, 2>>() };
        auto buf_tile_tile_offset_count{ context.create_buffer<std::array<uint32_t, 2>>(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, span_a).value() };
        auto buf_dest_tile{ context.create_buffer<std::array<uint16_t, 2>>(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, span_b).value() };

#endif
        kernel.set_arg(0, buffers[0]).value();
        kernel.set_arg(1, buffers[1]).value();
        kernel.set_arg(2, buffers[2]).value();
        kernel.set_arg(3, buffers[3]).value();
        kernel.set_arg(4, buf_stubLocations).value();
        kernel.set_arg(5, buf_tile_tile_offset_count).value();
        kernel.set_arg(6, buf_dest_tile).value();

        return OCL_Tile_Router{
            .context{context},
            .queues{queues},
            .program{program},
            .kernels{kernels},
            .buffers{buffers},
            .stubLocations{buf_stubLocations},
            .tile_tile_offset_count{buf_tile_tile_offset_count},
            .dest_tile{buf_dest_tile},
            .phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" },
        };
    }
};