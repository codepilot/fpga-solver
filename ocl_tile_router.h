#pragma once

#include "ocl.h"
#include "MemoryMappedFile.h"

class OCL_Tile_Router {
public:
    ocl::context context;
    std::vector<ocl::queue> queues;
    ocl::program program;
    std::vector<ocl::kernel> kernels;
    std::vector<ocl::buffer> buffers;
    cl_uint max_workgroup_size{ 256 };

    std::expected<void, ocl::status> step() {
        return queues.at(0).useGL(buffers, [&]() {
            queues.at(0).enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * 64 }, { max_workgroup_size }).value();
        });
    }
    static std::expected<OCL_Tile_Router, ocl::status> make(
        std::vector<cl_context_properties> context_properties = {},
        std::vector<GLuint> gl_buffers = {}//,
    ) {
        MemoryMappedFile source{ "../kernels/draw_wires.cl" };
        return ocl::context::create<CL_DEVICE_TYPE_GPU>(context_properties).and_then([&](ocl::context context)-> std::expected<OCL_Tile_Router, ocl::status> {
            return context.create_queues().and_then([&](std::vector<ocl::queue> queues)->std::expected<OCL_Tile_Router, ocl::status> {
                return context.create_program(source.get_span<char>()).and_then([&](ocl::program program)->std::expected<OCL_Tile_Router, ocl::status> {
                    auto build_status{ program.build() };
                    if (!build_status.has_value()) {
                        for (auto &&str : program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value()) {
                            OutputDebugStringA(str.c_str());
                            OutputDebugStringA("\n");
                        }
                        DebugBreak();
                    }
                    return program.build().and_then([&]()-> std::expected<OCL_Tile_Router, ocl::status> {
                        return program.create_kernels().and_then([&](std::vector<ocl::kernel> kernels)->std::expected<OCL_Tile_Router, ocl::status> {
                            return ocl::buffer::from_gl(context.context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).and_then([&](std::vector<ocl::buffer> buffers) {
                                decltype(auto) kernel{ kernels.at(0) };

                                kernel.set_arg(0, buffers[0].mem).value();
                                kernel.set_arg(1, buffers[1].mem).value();
                                kernel.set_arg(2, buffers[2].mem).value();
                                kernel.set_arg(3, buffers[3].mem).value();

                                return std::expected<OCL_Tile_Router, ocl::status>(OCL_Tile_Router{
                                    .context{context},
                                    .queues{queues},
                                    .program{program},
                                    .kernels{kernels},
                                    .buffers{buffers},
                                });
                            });
                        });
                    });
                });
            });
        });
    }
};