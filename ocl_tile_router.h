#pragma once

#include "ocl.h"
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
    std::vector<std::array<uint64_t, 16>> v_stubLocations;
    std::vector<std::array<uint16_t, 2>> v_allSourcePos;
    ocl::buffer stubLocations;
    ocl::buffer tile_tile_offset_count;
    ocl::buffer dest_tile;
    PhysicalNetlist::PhysNetlist::Reader phys;
    uint32_t netCount;
    uint32_t netCountAligned;
    uint32_t workgroup_count;
    uint32_t ocl_counter_max;

    inline static constexpr cl_uint max_workgroup_size{ 256 };
    // inline static constexpr cl_uint workgroup_count{ 1 };
    // inline static constexpr cl_uint total_group_size{ max_workgroup_size * workgroup_count };

    std::expected<void, ocl::status> step(ocl::queue &queue) {
#ifdef _DEBUG
        queue.enqueueRead<std::array<uint64_t, 16>>(stubLocations.mem, true, 0, v_stubLocations);
#endif
        auto result{ queue.enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
        return result;
    }

    std::expected<void, ocl::status> step_all(const uint32_t count) {
        kernels.at(0).set_arg_t(0, count);

        for (auto&& queue : queues) {
            auto result{ queue.enqueue_no_event<1>(kernels.at(0), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
            if (!result.has_value()) return result;
            break;
        }
        return std::expected<void, ocl::status>();
    }

    std::expected<void, ocl::status> do_all() {
        std::cout << std::format("ocl_counter_max: {}\n", ocl_counter_max);
        for (uint32_t count{}; count < ocl_counter_max; count++) {
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
        auto v_allSourcePos(std::vector<std::array<uint16_t, 2>>(static_cast<size_t>(netCountAligned), std::array<uint16_t, 2>{}));

        auto req_devices{ context_properties.size() ? ocl::device::get_gl_devices(context_properties).value() : std::vector<cl_device_id>{} };
        ocl::context context{ req_devices.size() ? ocl::context::create(context_properties, req_devices).value(): ocl::context::create<CL_DEVICE_TYPE_DEFAULT>().value() };
        auto device_ids{ context.get_devices().value() };
#ifdef _DEBUG
        for(auto &&device_id: device_ids) {
            ocl::device::log_info(device_id);
        }
#endif
        std::vector<ocl::queue> queues{ context.create_queues().value() };
        MemoryMappedFile source{ "../kernels/draw_wires.cl" };
        ocl::program program{ context.create_program(source.get_span<char>()).value() };
        size_t possible_ocl_counter_max{ ocl::device::get_info_integral<cl_ulong>(device_ids.at(0), CL_DEVICE_MAX_MEM_ALLOC_SIZE).value() / (static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)) };
        const uint32_t ocl_counter_max{ (possible_ocl_counter_max > 1024ull) ? 1024ul: static_cast<uint32_t>(possible_ocl_counter_max) };
        auto build_result{ program.build(std::format("-cl-mad-enable -cl-no-signed-zeros -Werror -cl-std=CL1.2 -cl-kernel-arg-info -g -D ocl_counter_max={}", ocl_counter_max))};
        auto build_logs{ program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value() };
        for (auto&& build_log : build_logs) {
            puts(build_log.c_str());
        }
        if (!build_result.has_value()) {
            abort();
        }
        build_result.value();
        std::vector<ocl::kernel> kernels{ program.create_kernels().value() };
        std::vector<ocl::buffer> buffers{ context.from_gl(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).value() };

        puts("allocating device buffers");
        if (buffers.size() < 2) {
            buffers.emplace_back(context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, static_cast<size_t>(ocl_counter_max) * static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)).value());
            buffers.emplace_back(context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint32_t, 4>)).value());
        }

        decltype(auto) kernel{ kernels.at(0) };

        puts("making stub locations");
        std::vector<std::array<uint64_t, 16>> v_stubLocations(static_cast<size_t>(netCountAligned), std::array<uint64_t, 16>{
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
        });

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

        each(v_stubLocations, [&](uint64_t index, std::array<uint64_t, 16> &stubLocation) {
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
            v_allSourcePos[index] = posA;
            v_stubLocations[index] = std::array<uint64_t, 16>{ 
                word_0,     UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
                UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
            };
        });

        puts("moving stub locations");
        std::span s_stubLocations{v_stubLocations};
        auto buf_stubLocations{ context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_READ_ONLY, v_stubLocations).value() };

        puts("create buf_tile_tile_offset_count");
        auto buf_tile_tile_offset_count{ context.create_buffer<std::array<uint32_t, 2>>(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, span_tt_count_offset).value() };

        puts("create buf_dest_tile");
        auto buf_dest_tile{ context.create_buffer<std::array<uint16_t, 2>>(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, span_tt_body).value() };

        puts("create buf_allSourcePos");
        auto buf_allSourcePos{ context.create_buffer<std::array<uint16_t, 2>>(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, v_allSourcePos).value() };

        puts("setting kernel args");
        kernel.set_arg(1, buffers.at(0)).value();
        kernel.set_arg(2, buffers.at(1)).value();
        kernel.set_arg(3, buf_stubLocations).value();
        kernel.set_arg(4, buf_tile_tile_offset_count).value();
        kernel.set_arg(5, buf_dest_tile).value();
        kernel.set_arg(6, buf_allSourcePos).value();

        return OCL_Tile_Router{
            .context{context},
            .device_ids{device_ids},
            .queues{queues},
            .program{program},
            .kernels{kernels},
            .buffers{buffers},
            .v_stubLocations{v_stubLocations},
            .v_allSourcePos{v_allSourcePos},
            .stubLocations{buf_stubLocations},
            .tile_tile_offset_count{buf_tile_tile_offset_count},
            .dest_tile{buf_dest_tile},
            .phys{ phys },
            .netCount{ netCount },
            .netCountAligned{ netCountAligned },
            .workgroup_count{ netCountAligned / max_workgroup_size },
            .ocl_counter_max{ ocl_counter_max },
        };
    }
};