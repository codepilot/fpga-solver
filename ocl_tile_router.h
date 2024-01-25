#pragma once

#define SKIP_SVM_MAP

#ifdef USE_CPP_INSTEAD_OF_OPENCL
#include "draw_wires.h"
#else
#include "ocl.h"
#endif

#include "MemoryMappedFile.h"
#include "InterchangeGZ.h"
#include "interchange_types.h"

class OCL_Tile_Router {
public:

    static inline const MemoryMappedFile mmf_tt_count_offset{ "tt_count_offset.bin" };
    static inline const MemoryMappedFile mmf_tt_body{ "tt_body.bin" };
    static inline const std::span<std::array<uint32_t, 2>> span_tt_count_offset{ mmf_tt_count_offset.get_span<std::array<uint32_t, 2>>() };
    static inline const std::span<std::array<uint16_t, 2>> span_tt_body{ mmf_tt_body.get_span<std::array<uint16_t, 2>>() };

    typedef struct {
        uint64_t tile_col : 10;
        uint64_t tile_row : 9;
        uint64_t tt_id : 20;
        uint64_t previous : 12;
        uint64_t cost : 13;
    } front_info;

    static std::array<uint32_t, 2> get_co(uint32_t x, uint32_t y) {
        return span_tt_count_offset[x + y * 670ul];
    }

    static std::span<std::array<uint16_t, 2>> get_s(uint32_t x, uint32_t y) {
        auto co{ get_co(x, y) };
        return span_tt_body.subspan(co.at(1), co.at(0));
    }

    ocl::context context;
    std::vector<cl_device_id> device_ids;
    std::vector<ocl::queue> queues;
    ocl::program program;
    std::vector<ocl::kernel> kernels;
    std::vector<ocl::buffer> buffers;
    ocl::svm<std::array<uint16_t, 2>> svm_routed_lines;
    ocl::svm<std::array<uint32_t, 4>> svm_drawIndirect;
    ocl::svm<std::array<uint64_t, 16>> svm_stubLocations;
    ocl::svm<std::array<uint32_t, 2>> svm_tile_tile_offset_count;
    ocl::svm<std::array<uint16_t, 2>> svm_dest_tile;
    ocl::svm<std::array<uint16_t, 2>> svm_allSourcePos;
    PhysicalNetlist::PhysNetlist::Reader phys;
    uint32_t netCount;
    uint32_t netCountAligned;
    uint32_t workgroup_count;
    uint32_t ocl_counter_max;

    inline static constexpr cl_uint max_workgroup_size{ 256 };
    // inline static constexpr cl_uint workgroup_count{ 1 };
    // inline static constexpr cl_uint total_group_size{ max_workgroup_size * workgroup_count };

    std::expected<void, ocl::status> step(ocl::queue &queue) {
        auto result{ queue.enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
        return result;
    }

    std::expected<void, ocl::status> step_all(const uint32_t count) {
        kernels.at(0).set_arg_t(0, count);

        for (auto&& queue : queues) {
            auto result{ queue.enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
            if (!result.has_value()) return result;
        }
        return std::expected<void, ocl::status>();
    }

    std::expected<void, ocl::status> do_all() {
#ifdef _DEBUG
        std::cout << std::format("ocl_counter_max: {}\n", ocl_counter_max);
#endif
        for (uint32_t count{}; count < ocl_counter_max; count++) {
#ifdef _DEBUG
            std::cout << std::format("step {} of {}\n", count, ocl_counter_max);
#endif
            step_all(count).value();
        }
        for (auto&& queue : queues) {
            queue.finish().value();
        }
        return std::expected<void, ocl::status>();
    }

    std::expected<void, ocl::status> gl_step(const uint32_t count) {
        kernels.at(0).set_arg_t(0, count);

        for (auto&& queue : queues) {
            auto result{ queue.useGL(buffers, [&]()->std::expected<void, ocl::status> {
                return step(queue);
            }) };
            if (!result.has_value()) return result;
        }
        return std::expected<void, ocl::status>();
    }

    static void get_site_pins(branch_reader src, std::set<uint32_t> &dst) {
        if (src.getRouteSegment().isSitePin()) dst.insert(src.getRouteSegment().getSitePin().getSite());
        for (auto&& subbranch : src.getBranches()) {
            get_site_pins(subbranch, dst);
        }
    }

    static OCL_Tile_Router make(
        DeviceResources::Device::Reader dev,
        PhysicalNetlist::PhysNetlist::Reader phys,
        std::vector<cl_context_properties> context_properties = {},
        std::vector<cl_uint> gl_buffers = {}//,
    ) {

        const uint32_t netCount{ phys.getPhysNets().size() };
        const uint32_t netCountAligned{ (((netCount + 255ul) >> 8ul) << 8ul) };

        auto req_devices{ std::ranges::contains(context_properties, CL_GL_CONTEXT_KHR) ? ocl::device::get_gl_devices(context_properties).value() : std::vector<cl_device_id>{} };
        ocl::context context{ req_devices.size() ? ocl::context::create(context_properties, req_devices).value(): ocl::context::create<CL_DEVICE_TYPE_DEFAULT>(context_properties).value() };
        auto device_ids{ context.get_devices().value() };
        auto device_svm_caps{ ocl::device::get_info_integral<cl_device_svm_capabilities>(device_ids.at(0), CL_DEVICE_SVM_CAPABILITIES).value_or(0) };
        bool has_svm_fine_grain_buffer{ (device_svm_caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) == CL_DEVICE_SVM_FINE_GRAIN_BUFFER };
        cl_svm_mem_flags maybe_fine_grain{ has_svm_fine_grain_buffer ? CL_MEM_SVM_FINE_GRAIN_BUFFER : 0ul };

        std::vector<ocl::queue> queues{ context.create_queues().value() };
        MemoryMappedFile source{ "../kernels/draw_wires.cl" };
        ocl::program program{ context.create_program(source.get_span<char>()).value() };
        auto dev_max_mem_alloc_size{ocl::device::get_info_integral<cl_ulong>(device_ids.at(0), CL_DEVICE_MAX_MEM_ALLOC_SIZE).value()};
#ifdef _DEBUG
        puts(std::format("dev_max_mem_alloc_size: {} MiB", std::scalbln(static_cast<double>(dev_max_mem_alloc_size), -20)).c_str());
#endif
        size_t possible_ocl_counter_max{ dev_max_mem_alloc_size / (static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)) };
        const uint32_t ocl_counter_max{ (possible_ocl_counter_max > 256ull) ? 256ul: static_cast<uint32_t>(possible_ocl_counter_max) };
        auto build_result{ program.build(std::format("-cl-mad-enable -cl-no-signed-zeros -Werror -cl-std=CL1.2 -cl-kernel-arg-info -g -D ocl_counter_max={}", ocl_counter_max))};
        auto build_logs{ program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value() };
#ifdef _DEBUG
        for (auto&& build_log : build_logs) {
            puts(build_log.c_str());
        }
#endif
        if (!build_result.has_value()) {
            abort();
        }
        build_result.value();
        std::vector<ocl::kernel> kernels{ program.create_kernels().value() };
        std::vector<ocl::buffer> buffers{ context.from_gl(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).value() };

#ifdef _DEBUG
        puts("allocating device buffers");
#endif
        ocl::svm<std::array<uint16_t, 2>> svm_routed_lines;
        ocl::svm<std::array<uint32_t, 4>> svm_drawIndirect;
        if (buffers.size() < 2) {
#ifdef _DEBUG
            std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
            svm_routed_lines = context.alloc_svm<std::array<uint16_t, 2>>(maybe_fine_grain & CL_MEM_READ_WRITE, static_cast<size_t>(ocl_counter_max) * static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)).value();
#ifdef _DEBUG
            std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
            svm_drawIndirect = context.alloc_svm<std::array<uint32_t, 4>>(maybe_fine_grain & CL_MEM_READ_WRITE, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint32_t, 4>)).value();
#ifdef _DEBUG
            std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
        }

        decltype(auto) kernel{ kernels.at(0) };

        auto nets{ phys.getPhysNets() };
        auto physStrs{ phys.getStrList() };
        auto devStrs{ dev.getStrList() };

        std::map<std::string_view, std::array<uint16_t, 2>> site_locations;
        for (auto&& tile : dev.getTileList()) {
            std::array<uint16_t, 2> pos{ tile.getCol(), tile.getRow() };
            for (auto&& site : tile.getSites()) {
                site_locations.insert({devStrs[site.getName()].cStr(), pos});
            }
        }

#ifdef _DEBUG
        puts("create buf_tile_tile_offset_count");
#endif
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
        auto svm_tile_tile_offset_count{ context.alloc_svm<std::array<uint32_t, 2>>(maybe_fine_grain & CL_MEM_READ_ONLY, span_tt_count_offset.size_bytes()).value() };
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            queues.at(0).enqueueSVMMemcpy(false, svm_tile_tile_offset_count, span_tt_count_offset);
#ifdef SKIP_SVM_MAP
        } else {
            memcpy(svm_tile_tile_offset_count.data(), span_tt_count_offset.data(), span_tt_count_offset.size_bytes());
        }
#endif

#ifdef _DEBUG
        puts("create buf_dest_tile");
#endif
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
        auto svm_dest_tile{ context.alloc_svm<std::array<uint16_t, 2>>(maybe_fine_grain & CL_MEM_READ_ONLY, span_tt_body.size_bytes()).value() };
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            queues.at(0).enqueueSVMMemcpy(false, svm_dest_tile, span_tt_body);
#ifdef SKIP_SVM_MAP
        } else {
            memcpy(svm_dest_tile.data(), span_tt_body.data(), span_tt_body.size_bytes());
        }
#endif

#ifdef _DEBUG
        puts("create buf_allSourcePos");
#endif
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
        auto svm_allSourcePos{ context.alloc_svm<std::array<uint16_t, 2>>(maybe_fine_grain & CL_MEM_READ_ONLY, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)).value() };
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            queues.at(0).enqueueSVMMap(true, CL_MAP_WRITE_INVALIDATE_REGION, svm_allSourcePos).value();
#ifdef SKIP_SVM_MAP
        }
#endif


#ifdef _DEBUG
        puts("making stub locations");
#endif
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif
        auto svm_stubLocations{ context.alloc_svm<std::array<uint64_t, 16>>(maybe_fine_grain & CL_MEM_READ_WRITE, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint64_t, 16>)).value() };
#ifdef _DEBUG
        std::cout << std::format("CL_DEVICE_GLOBAL_FREE_MEMORY_AMD               : {} MiB\n", scalbln(static_cast<double>(ocl::device::get_info_integral<uint64_t>(device_ids.at(0), CL_DEVICE_GLOBAL_FREE_MEMORY_AMD).value_or(0)), -10));
#endif

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            queues.at(0).enqueueSVMMap(true, CL_MAP_WRITE_INVALIDATE_REGION, svm_stubLocations).value();
#ifdef SKIP_SVM_MAP
        }
#endif

        std::ranges::fill(svm_stubLocations, std::array<uint64_t, 16>{
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
        });

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            if (!svm_drawIndirect.empty()) queues.at(0).enqueueSVMMap(true, CL_MAP_WRITE_INVALIDATE_REGION, svm_drawIndirect).value();
#ifdef SKIP_SVM_MAP
        }
#endif

        each(svm_stubLocations, [&](uint64_t index, std::array<uint64_t, 16> &stubLocation) {
            if(svm_drawIndirect.size() > index) svm_drawIndirect[index] = {
                0,//count
                0,//instanceCount
                static_cast<uint32_t>(index) * ocl_counter_max, // first
                1,// baseInstance
            };

            if (index >= nets.size()) return;
            auto net{ nets[index] };
            auto stubs{ net.getStubs() };
            if (!stubs.size()) return;
            auto sources{ net.getSources() };
            if (sources.size() != 1) return;
            std::set<uint32_t> src_sites, dst_sites;
            get_site_pins(sources[0], src_sites);
            get_site_pins(stubs[0], dst_sites);
            if (!src_sites.size()) return;
            if (!dst_sites.size()) return;
            std::vector<std::string_view> src_site_names, dst_site_names;
            for (uint32_t site : src_sites) src_site_names.emplace_back(physStrs[site].cStr());
            for (uint32_t site : dst_sites) dst_site_names.emplace_back(physStrs[site].cStr());
            auto posA{ site_locations.at(src_site_names.at(0)) };
            auto posB{ site_locations.at(dst_site_names.at(0)) };
            uint64_t word_0{ std::bit_cast<uint64_t>(front_info{
                .tile_col{posB[0]},
                .tile_row{posB[1]},
                .tt_id{},
                .previous{},
                .cost{},
            } ) };
            svm_allSourcePos[index] = posA;
            svm_stubLocations[index] = std::array<uint64_t, 16>{
                word_0,     UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            };
            if (svm_drawIndirect.size() > index) svm_drawIndirect[index] = {
                0,//count
                1,//instanceCount
                static_cast<uint32_t>(index) * ocl_counter_max, // first
                0,// baseInstance
            };

        });

#ifdef SKIP_SVM_MAP
        if(dev_max_mem_alloc_size > UINT32_MAX) {
#endif
            queues.at(0).enqueueSVMUnmap(svm_allSourcePos).value();
            queues.at(0).enqueueSVMUnmap(svm_stubLocations).value();
            if (!svm_drawIndirect.empty()) queues.at(0).enqueueSVMUnmap(svm_drawIndirect).value();
#ifdef SKIP_SVM_MAP
        }
#endif

#ifdef _DEBUG
        puts("setting kernel args");
#endif
        
        kernel.set_arg_t(0, 0).value(); // uint count,
        if (buffers.empty()) {
            kernel.set_arg(1, svm_routed_lines).value();//global routed_lines* restrict routed,
            kernel.set_arg(2, svm_drawIndirect).value();//global uint4* restrict drawIndirect,
        }
        else {
            kernel.set_arg(1, buffers.at(0)).value();//global routed_lines* restrict routed,
            kernel.set_arg(2, buffers.at(1)).value();//global uint4* restrict drawIndirect,
        }
        kernel.set_arg(3, svm_stubLocations).value();//global ulong16* restrict stubLocations, // cost: 13 bit, previous: 12, tt_id:20bit, tile:19bit
        kernel.set_arg(4, svm_tile_tile_offset_count).value();//constant uint2* restrict tile_tile_count_offset,
        kernel.set_arg(5, svm_dest_tile).value();//constant ushort2* restrict dest_tile,
        kernel.set_arg(6, svm_allSourcePos).value();//constant ushort2* restrict allSourcePos

        return OCL_Tile_Router{
            .context{std::move(context)},
            .device_ids{std::move(device_ids)},
            .queues{std::move(queues)},
            .program{std::move(program)},
            .kernels{std::move(kernels)},
            .buffers{std::move(buffers)},
            .svm_routed_lines{svm_routed_lines},
            .svm_drawIndirect{svm_drawIndirect},
            .svm_stubLocations{svm_stubLocations},
            .svm_tile_tile_offset_count{std::move(svm_tile_tile_offset_count)},
            .svm_dest_tile{std::move(svm_dest_tile)},
            .svm_allSourcePos{std::move(svm_allSourcePos)},
            .phys{ std::move(phys) },
            .netCount{ std::move(netCount) },
            .netCountAligned{ std::move(netCountAligned) },
            .workgroup_count{ netCountAligned / max_workgroup_size },
            .ocl_counter_max{ std::move(ocl_counter_max) },
        };
    }
};