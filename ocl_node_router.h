#pragma once

#ifdef USE_CPP_INSTEAD_OF_OPENCL
#include "cpp_node_router.h"
#else
#include "ocl.h"
#endif

#include "MemoryMappedFile.h"
#include "InterchangeGZ.h"
#include "interchange_types.h"
#include "inverse_wires.h"
#include "Timer.h"

// #define SVM_FINE

class OCL_Node_Router {
public:
    static inline constexpr uint32_t beam_width{ 128ul };
    static inline constexpr uint32_t max_tile_count{ 5884ul };
    static inline constexpr uint32_t tt_body_count{ 4293068ul };
    static inline constexpr size_t largest_ocl_counter_max{ 64ull };
    static inline constexpr uint32_t series_id_max{ 1024ul };

    using beam_t = std::array<std::array<uint32_t, 4>, beam_width>;
    using history_t = std::array<std::array<uint32_t, 2>, largest_ocl_counter_max>;

    static inline const MemoryMappedFile mmf_v_pip_count_offset{ "pip_count_offset.bin" };
    static inline const MemoryMappedFile mmf_v_pip_tile_body{ "pip_tile_body.bin" };
    static inline const MemoryMappedFile mmf_v_pip_body{ "pip_body.bin" };
    static inline const MemoryMappedFile mmf_v_inverse_wires{ "Inverse_Wires.bin" };
    static inline const std::span<std::array<uint32_t, 2>> s_pip_count_offset{ mmf_v_pip_count_offset.get_span<std::array<uint32_t, 2>>() };
    static inline const std::span<std::array<uint32_t, 4>> s_pip_tile_body{ mmf_v_pip_tile_body.get_span<std::array<uint32_t, 4>>() };
    static inline const std::span<std::array<uint32_t, 4>> s_pip_body{ mmf_v_pip_body.get_span<std::array<uint32_t, 4>>() };
    static inline const Inverse_Wires inverse_wires{ mmf_v_inverse_wires.get_span<uint64_t>() };

    static inline constexpr std::array<uint32_t, 4> fill_draw_commands {
        0,//count
        0,//instanceCount
        0, // first
        1,// baseInstance
    };

    typedef struct {
        uint64_t tile_col : 10;
        uint64_t tile_row : 9;
        uint64_t tt_id : 20;
        uint64_t previous : 12;
        uint64_t cost : 13;
    } front_info;

    ocl::context context;
    std::vector<cl_device_id> device_ids;
    std::vector<ocl::queue> queues;
    ocl::program program;
    std::vector<ocl::kernel> kernels;
    std::vector<ocl::buffer> buffers;
    ocl::svm<std::array<uint16_t, 4>> svm_routed_lines;
    ocl::svm<std::array<uint32_t, 4>> svm_drawIndirect;
    ocl::svm<beam_t> svm_heads;
    ocl::svm<history_t> svm_explored;
    ocl::svm<std::array<uint32_t, 2>> svm_pip_offset_count;
    ocl::svm<std::array<uint32_t, 4>> svm_pip_tile_body;
    ocl::svm<std::array<uint32_t, 4>> svm_stubs;
    ocl::svm<uint32_t> svm_dirty;
    PhysicalNetlist::PhysNetlist::Reader phys;
    uint32_t netCount;
    uint32_t netCountAligned;
    uint32_t workgroup_count;
    uint32_t ocl_counter_max;
    std::vector<ocl::svm<uint8_t>> all_svm;
    struct net_pair_t {
        net_reader net;
        branch_reader source;
        branch_reader stub;
    };

    std::vector<net_pair_t> net_pairs;
    std::map<std::string_view, std::array<uint16_t, 2>> site_locations;

    cl_uint max_workgroup_size;
    // inline static constexpr cl_uint workgroup_count{ 1 };
    // inline static constexpr cl_uint total_group_size{ max_workgroup_size * workgroup_count };

    std::expected<void, ocl::status> step(ocl::queue &queue) {
        auto result{ queue.enqueue<1>(kernels.front(), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
        return result;
    }

    std::expected<void, ocl::status> step_all(const uint32_t series_id) {
        kernels.front().set_arg_t(0, series_id);

        for (auto&& queue : queues) {
            auto result{ queue.enqueue<1>(kernels.front(), { 0 }, { max_workgroup_size * workgroup_count }, { max_workgroup_size }) };
            if (!result.has_value()) return result;
        }
        return std::expected<void, ocl::status>();
    }

    std::expected<void, ocl::status> do_all() {
#ifdef _DEBUG
        std::cout << std::format("ocl_counter_max: {}\n", ocl_counter_max);
#endif
        uint32_t previous_dirty_count{};
        for (uint32_t series_id{}; series_id < series_id_max; series_id++) {
            std::cout << std::format("ocl_counter_max: {}, step {} of {}, ", ocl_counter_max, series_id + 1, series_id_max);
            queues.front().enqueueSVMMemFill<uint32_t>(svm_dirty, 0u);
            step_all(series_id).value();
            std::vector<uint32_t> host_dirty(svm_dirty.size(), 0);
            queues.front().enqueueSVMMemcpy<uint32_t>(false, host_dirty, svm_dirty);
            queues.front().finish().value();

            std::cout << std::format("result {} {} {} {}\r", host_dirty[0], host_dirty[1], host_dirty[2], host_dirty[3]);
            if(!host_dirty.front()) break;
            if (previous_dirty_count != host_dirty.front()) std::cout << "\n";
            previous_dirty_count = host_dirty.front();
        }
        for (auto&& queue : queues) {
            queue.finish().value();
        }
        puts("");

        return std::expected<void, ocl::status>();
    }

    void inspect() {
#ifndef SVM_FINE
        queues.front().enqueueSVMMap<uint8_t>(CL_MAP_READ, all_svm, [&]() {
#endif
            auto physStrs{ phys.getStrList() };
            each(svm_drawIndirect.subspan(0, netCount), [&](uint64_t di_index, std::array<uint32_t, 4>& di) {
                // if (di[3] == 1) return;
                auto net = net_pairs[di_index].net;
                auto source = net_pairs[di_index].source;
                auto stub = net_pairs[di_index].stub;
                std::set<uint32_t> src_sites, dst_sites;
                get_site_pins(source, src_sites);
                get_site_pins(stub, dst_sites);
                if (!src_sites.size()) return;
                if (!dst_sites.size()) return;
                std::vector<std::string> src_site_names, dst_site_names;
                for (uint32_t site : src_sites) src_site_names.emplace_back(physStrs[site].cStr());
                for (uint32_t site : dst_sites) dst_site_names.emplace_back(physStrs[site].cStr());
                auto posA{ site_locations.at(src_site_names.front()) };
                auto posB{ site_locations.at(dst_site_names.front()) };

                printf("%u\t%u\t%u\t%u\t%s\t%s\t%s\n", di[0], di[1], di[2], di[3],
                    physStrs[net.getName()].cStr(),
                    src_site_names.front().c_str(),
                    dst_site_names.front().c_str()
                );
            });
#ifndef SVM_FINE
        });
#endif
    }

    std::expected<void, ocl::status> gl_step(const uint32_t series_id) {
        kernels.front().set_arg_t(0, series_id);

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

    static void get_site_pin_nodes(branch_reader src, std::set<uint32_t>& dst, std::unordered_map<uint64_t, uint32_t> &site_pin_to_node) {
        if (src.getRouteSegment().isSitePin()) {
            auto sitePin{ src.getRouteSegment().getSitePin() };
            std::array<uint32_t, 2> site_pin_key{ sitePin.getSite(), sitePin.getPin() };
            dst.insert(site_pin_to_node.at(std::bit_cast<uint64_t>(site_pin_key)));
        }
        for (auto&& subbranch : src.getBranches()) {
            get_site_pin_nodes(subbranch, dst, site_pin_to_node);
        }
    }

    static uint32_t count_stubs(net_list_reader nets) {
        std::atomic<uint32_t> ret{};
        jthread_each(nets, [&](uint64_t net_index, net_reader &net) {
            if (net.getSources().size() == 1) {
                ret += net.getStubs().size();
            }
        });
        return ret;
    }

    static std::map<std::string_view, std::array<uint16_t, 2>> make_site_locations(DeviceResources::Device::Reader dev) {
        auto devStrs{ dev.getStrList() };
        std::map<std::string_view, std::array<uint16_t, 2>> site_locations;
        for (auto&& tile : dev.getTileList()) {
            std::array<uint16_t, 2> pos{ tile.getCol(), tile.getRow() };
            for (auto&& site : tile.getSites()) {
                site_locations.insert({ devStrs[site.getName()].cStr(), pos });
            }
        }
        return site_locations;
    }

    static std::unordered_map<std::string_view, uint32_t> make_devStrs_map(DeviceResources::Device::Reader dev) {
        std::unordered_map<std::string_view, uint32_t> devStrs_map;
        auto devStrs{ dev.getStrList() };
        devStrs_map.reserve(devStrs.size());

        each(devStrs, [&](uint64_t dev_strIdx, capnp::Text::Reader dev_str) {
            std::string_view str{ dev_str.cStr() };
            devStrs_map.insert({ str, static_cast<uint32_t>(dev_strIdx) });
        });
        return devStrs_map;
    }

    static std::array<std::vector<uint32_t>, 2> make_string_interchange(const std::unordered_map<std::string_view, uint32_t> &devStrs_map, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs) {
        std::vector<uint32_t> physStrs_to_devStrs(static_cast<size_t>(physStrs.size()), UINT32_MAX);
        std::vector<uint32_t> devStrs_to_physStrs(static_cast<size_t>(devStrs.size()), UINT32_MAX);
        each(physStrs, [&](uint64_t phys_strIdx, capnp::Text::Reader phys_str) {
            std::string_view str{ phys_str.cStr() };
            if (!devStrs_map.contains(str)) return;
            auto dev_strIdx{ devStrs_map.at(str) };
            physStrs_to_devStrs[phys_strIdx] = dev_strIdx;
            devStrs_to_physStrs[dev_strIdx] = phys_strIdx;
        });
        return { physStrs_to_devStrs , devStrs_to_physStrs };
    }

#if 0
    static std::unordered_map<uint64_t, uint32_t> make_tile_wire_to_wire_idx(wire_list_reader wires) {
        std::unordered_map<uint64_t, uint32_t> tile_wire_to_wire_idx;
        tile_wire_to_wire_idx.reserve(wires.size());

        each(wires, [&](uint64_t wire_idx, wire_reader wire) {
            auto tw_key{ std::bit_cast<uint64_t>(std::array<uint32_t, 2>{ wire.getTile(), wire.getWire() }) };
            tile_wire_to_wire_idx.insert({ tw_key, wire_idx });
        });
        return tile_wire_to_wire_idx;
    }
#endif

    static std::vector<uint32_t> make_wire_idx_to_node_idx(wire_list_reader wires, node_list_reader nodes) {
        std::vector<uint32_t> wire_idx_to_node_idx(static_cast<size_t>(wires.size()), UINT32_MAX);
        each(nodes, [&](uint64_t node_idx, node_reader node) {
            for (auto wire_idx : node.getWires()) {
                wire_idx_to_node_idx[wire_idx] = static_cast<uint32_t>(node_idx);
            }
        });

        return wire_idx_to_node_idx;
    }

    static std::unordered_map<uint64_t, uint32_t> make_site_pin_to_node(tile_list_reader tiles, tile_type_list_reader tile_types, site_type_list_reader site_types, std::span<const uint32_t> devStrs_to_physStrs, std::span<const uint32_t> wire_idx_to_node_idx) {
        std::unordered_map<uint64_t, uint32_t> site_pin_to_node;
        for (auto&& tile : tiles) {
            auto tile_str_idx{ tile.getName() };
            auto sites{ tile.getSites() };
            auto tileType{ tile_types[tile.getType()] };
            auto tileTypeSiteTypes{ tileType.getSiteTypes() };

            for (auto&& site : tile.getSites()) {
                auto phys_site_name{ devStrs_to_physStrs[site.getName()] };
                if (phys_site_name == UINT32_MAX) continue;
                auto siteTypeInTile{ tileTypeSiteTypes[site.getType()] };
                auto siteType{ site_types[siteTypeInTile.getPrimaryType()] };
                auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
                each(siteType.getPins(), [&](uint64_t pin_index, auto& pin) {
                    auto phys_pin_name{ devStrs_to_physStrs[pin.getName()] };
                    if (phys_pin_name == UINT32_MAX) return;
                    auto wire_str_idx{ tile_wires[pin_index] };
                    auto wire_idx{ inverse_wires.at(tile_str_idx, wire_str_idx) };
                    auto node_idx{ wire_idx_to_node_idx[wire_idx] };
                    auto sp_key{ std::bit_cast<uint64_t>(std::array<uint32_t, 2>{phys_site_name, phys_pin_name}) };
                    site_pin_to_node.insert({ sp_key, node_idx });
                });
            }
        }

        return site_pin_to_node;
    }

    static std::vector<uint32_t> make_inverse_nodes(wire_list_reader wires, node_list_reader nodes) {
        std::vector<uint32_t> inverse_nodes(static_cast<size_t>(wires.size()), UINT32_MAX);
        jthread_each(nodes, [&](uint64_t node_idx, node_reader& node) {
            auto node_wires = node.getWires();
            for (uint32_t wire_idx : node_wires) {
                inverse_nodes[wire_idx] = static_cast<uint32_t>(node_idx);
            }
        });

        return inverse_nodes;
    }

    static OCL_Node_Router make(
        DeviceResources::Device::Reader dev,
        PhysicalNetlist::PhysNetlist::Reader phys,
        std::vector<cl_context_properties> context_properties = {},
        std::vector<cl_uint> gl_buffers = {}//,
    ) {

        std::vector<ocl::svm<uint8_t>> all_svm;
        auto nets{ phys.getPhysNets() };
        auto physStrs{ phys.getStrList() };
        auto devStrs{ dev.getStrList() };
        const uint32_t netCount{ count_stubs(nets) };
        const uint32_t netCountAligned{ (((netCount + 255ul) >> 8ul) << 8ul) };

        auto req_devices{ std::ranges::contains(context_properties, CL_GL_CONTEXT_KHR) ? ocl::device::get_gl_devices(context_properties).value() : std::vector<cl_device_id>{} };
        ocl::context context{ req_devices.size() ? ocl::context::create(context_properties, req_devices).value(): ocl::context::create<CL_DEVICE_TYPE_DEFAULT>(context_properties).value() };
        auto device_ids{ context.get_devices().value() };
        auto device_svm_caps{ ocl::device::get_info_integral<cl_device_svm_capabilities>(device_ids.front(), CL_DEVICE_SVM_CAPABILITIES).value_or(0) };
        bool has_svm_fine_grain_buffer{ (device_svm_caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) == CL_DEVICE_SVM_FINE_GRAIN_BUFFER };
#ifdef SVM_FINE
        cl_svm_mem_flags maybe_fine_grain{ has_svm_fine_grain_buffer ? CL_MEM_SVM_FINE_GRAIN_BUFFER : 0ul };
#else
        cl_svm_mem_flags maybe_fine_grain{ 0ul };
#endif
        std::vector<ocl::queue> queues{ context.create_queues().value() };
        decltype(auto) primary_queue{ queues.front() };
        MemoryMappedFile source{ "../kernels/node_router.cl" };
        ocl::program program{ context.create_program(source.get_span<char>()).value() };
        auto dev_max_mem_alloc_size{ocl::device::get_info_integral<cl_ulong>(device_ids.front(), CL_DEVICE_MAX_MEM_ALLOC_SIZE).value()};
        auto generic_workgroup_size{ ocl::device::get_info_integral<uint64_t>(device_ids.front(), CL_DEVICE_MAX_WORK_GROUP_SIZE).value_or(0) };
        auto amd_preferred_workgroup_size{ ocl::device::get_info_integral<uint64_t>(device_ids.front(), CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_AMD) };
        cl_uint max_workgroup_size{ static_cast<cl_uint>( amd_preferred_workgroup_size.value_or(generic_workgroup_size) ) };
#ifdef _DEBUG
        puts(std::format("dev_max_mem_alloc_size: {} MiB", std::scalbln(static_cast<double>(dev_max_mem_alloc_size), -20)).c_str());
#endif
        size_t possible_ocl_counter_max{ dev_max_mem_alloc_size / (static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 2>)) };
        const uint32_t ocl_counter_max{ (possible_ocl_counter_max > largest_ocl_counter_max) ? static_cast<uint32_t>(largest_ocl_counter_max) : static_cast<uint32_t>(possible_ocl_counter_max) };
        auto options{ std::format("-cl-mad-enable -cl-no-signed-zeros -Werror -cl-std=CL1.2 -cl-kernel-arg-info -g -D ocl_counter_max={}u -D netCountAligned={}u -D beam_width={}u -D max_tile_count={}u -D tt_body_count={}u -D max_workgroup_size={} -D count_of_pip_count_offset={} -D count_pip_tile_body={}",
            ocl_counter_max,
            netCountAligned,
            beam_width,
            max_tile_count,
            tt_body_count,
            max_workgroup_size,
            s_pip_count_offset.size(),
            s_pip_body.size())
        };
        auto build_result{ TimerVal(program.build(options)) };
        auto build_logs{ program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value() };
#ifdef _DEBUG
        for (auto&& build_log : build_logs) {
            puts(build_log.c_str());
        }
#endif
        if (!build_result.has_value()) {
            for (auto&& build_log : build_logs) {
                puts(build_log.c_str());
            }
            abort();
        }
        build_result.value();
        std::vector<ocl::kernel> kernels{ program.create_kernels().value() };
        std::vector<ocl::buffer> buffers{ context.from_gl(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).value() };

#ifdef _DEBUG
        puts("allocating device buffers");
#endif
        ocl::svm<std::array<uint16_t, 4>> svm_routed_lines;
        ocl::svm<std::array<uint32_t, 4>> svm_drawIndirect;
        if (buffers.size() < 2) {
            svm_routed_lines = context.alloc_svm<std::array<uint16_t, 4>>(maybe_fine_grain | CL_MEM_READ_WRITE, static_cast<size_t>(ocl_counter_max) * static_cast<size_t>(netCountAligned) * sizeof(std::array<uint16_t, 4>)).value();
            svm_drawIndirect = context.alloc_svm<std::array<uint32_t, 4>>(maybe_fine_grain | CL_MEM_READ_WRITE, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint32_t, 4>)).value();
        }

        decltype(auto) kernel{ kernels.front() };

        const auto site_locations{ TimerVal(make_site_locations(dev)) };
        const auto devStrs_map{ TimerVal(make_devStrs_map(dev)) };

        const auto string_interchange{ TimerVal(make_string_interchange(devStrs_map, devStrs, physStrs)) };
        const auto physStrs_to_devStrs{ std::span(string_interchange.at(0)) };
        const auto devStrs_to_physStrs{ std::span(string_interchange.at(1)) };

        // const auto tile_wire_to_wire_idx{ TimerVal(make_tile_wire_to_wire_idx(dev.getWires())) };

        const auto wire_idx_to_node_idx{ TimerVal(make_wire_idx_to_node_idx(dev.getWires(), dev.getNodes())) };

        auto site_pin_to_node{ TimerVal(make_site_pin_to_node(dev.getTileList(), dev.getTileTypeList(), dev.getSiteTypeList(), devStrs_to_physStrs, wire_idx_to_node_idx)) };
 
        auto inverse_nodes{ make_inverse_nodes(dev.getWires(), dev.getNodes() ) };

#ifdef _DEBUG
        puts("create buf_tile_tile_offset_count");
#endif
        auto svm_pip_offset_count{ context.alloc_svm<std::array<uint32_t, 2>>(maybe_fine_grain | CL_MEM_READ_ONLY, s_pip_count_offset.size_bytes()).value() };

        primary_queue.enqueueSVMMemcpy(false, svm_pip_offset_count, s_pip_count_offset);

#ifdef _DEBUG
        puts("create buf_dest_tile");
#endif
        auto svm_pip_tile_body{ context.alloc_svm<std::array<uint32_t, 4>>(maybe_fine_grain | CL_MEM_READ_ONLY, s_pip_tile_body.size_bytes()).value() };

        primary_queue.enqueueSVMMemcpy(false, svm_pip_tile_body, s_pip_tile_body);

#ifdef _DEBUG
        puts("create buf_allSourcePos");
#endif
        auto svm_stubs{ context.alloc_svm<std::array<uint32_t, 4>>(maybe_fine_grain | CL_MEM_READ_ONLY, static_cast<size_t>(netCountAligned) * sizeof(std::array<uint32_t, 4>)).value() };

        auto svm_dirty{ context.alloc_svm<uint32_t>(maybe_fine_grain | CL_MEM_WRITE_ONLY, sizeof(uint32_t) * 4).value() };
        primary_queue.enqueueSVMMemFill(svm_dirty, 0u);

#ifdef _DEBUG
        puts("making stub locations");
#endif
        auto svm_heads{ context.alloc_svm<beam_t>(maybe_fine_grain | CL_MEM_READ_WRITE, static_cast<size_t>(netCountAligned) * sizeof(beam_t)).value() };
        auto svm_explored{ context.alloc_svm<history_t>(maybe_fine_grain | CL_MEM_WRITE_ONLY, static_cast<size_t>(netCountAligned) * sizeof(history_t)).value() };

        primary_queue.enqueueSVMMemFill(svm_heads.cast<uint64_t>(), UINT64_MAX);
        primary_queue.enqueueSVMMemFill(svm_drawIndirect, fill_draw_commands);
        primary_queue.enqueueSVMMemFill(svm_explored.cast<uint64_t>(), UINT64_MAX);

        std::atomic<uint32_t> global_stub_index{};

        std::vector<net_pair_t> net_pairs;
        net_pairs.resize(netCount);

        {
            Timer<"upload to gpu {}\n"> t;
            primary_queue.finish();
#ifndef SVM_FINE
            primary_queue.enqueueSVMMap(CL_MAP_WRITE_INVALIDATE_REGION, svm_drawIndirect, [&]() {
                primary_queue.enqueueSVMMap(CL_MAP_WRITE_INVALIDATE_REGION, svm_stubs, [&]() {
                    primary_queue.enqueueSVMMap(CL_MAP_WRITE, svm_heads, [&]() {
#endif
                        jthread_each(nets, [&](uint64_t net_index, net_reader& net) {
                            auto stubs{ net.getStubs() };
                            if (!stubs.size()) return;
                            auto sources{ net.getSources() };
                            if (sources.size() != 1) return;

                            const uint32_t local_stub_index{ global_stub_index.fetch_add(stubs.size()) };
                            auto source{ sources[0] };

                            each(stubs, [&](uint32_t stub_offset, branch_reader& stub) {
                                const uint32_t stub_index{ local_stub_index + stub_offset };
                                net_pairs[stub_index] = { net, source , stub };
                                std::set<uint32_t> src_sites, dst_sites;
                                get_site_pins(source, src_sites);
                                get_site_pins(stub, dst_sites);
                                if (!src_sites.size()) return;
                                if (!dst_sites.size()) return;
                                std::set<uint32_t> src_nodes, dst_nodes;
                                get_site_pin_nodes(source, src_nodes, site_pin_to_node);
                                get_site_pin_nodes(stub, dst_nodes, site_pin_to_node);
                                if (!src_nodes.size()) return;
                                if (!dst_nodes.size()) return;

                                std::vector<std::string_view> src_site_names, dst_site_names;
                                for (uint32_t site : src_sites) src_site_names.emplace_back(physStrs[site].cStr());
                                for (uint32_t site : dst_sites) dst_site_names.emplace_back(physStrs[site].cStr());
                                auto posA{ site_locations.at(src_site_names.front()) };
                                auto posB{ site_locations.at(dst_site_names.front()) };
                                svm_stubs[stub_index] = std::array<uint32_t, 4>{
                                    posB[0], //x
                                        posB[1], //y
                                        * dst_nodes.begin(), // node_idx
                                        static_cast<uint32_t>(net_index) //net_idx
                                };
                                auto count_offset{ s_pip_count_offset[*src_nodes.begin()] };
                                auto pip_offset{ count_offset[1] };
                                auto infos{ s_pip_tile_body.subspan(pip_offset, count_offset[0]) };
                                if (infos.size() > svm_heads[stub_index].size()) {
                                    DebugBreak();
                                }
                                each(infos, [&](uint64_t info_idx, std::array<uint32_t, 4>& info) {
                                    if (info_idx >= svm_heads[stub_index].size()) return;
                                    svm_heads[stub_index][info_idx] = std::array<uint32_t, 4>{
                                        0, //cost
                                            0, //height
                                            UINT32_MAX, //parent
                                            pip_offset + static_cast<uint32_t>(info_idx), //pip_idx
                                    };
                                    });

                                if (!svm_drawIndirect.empty()) {
                                    svm_drawIndirect[stub_index] = {
                                        0,//count
                                        1,//instanceCount
                                        static_cast<uint32_t>(stub_index) * ocl_counter_max, // first
                                        0,// baseInstance
                                    };
                                }
                                });
                            });
#ifndef SVM_FINE
                        }).value();
                    }).value();
                }).value();
#endif
        }
        if (global_stub_index != netCount) {
            printf("global_stub_index(%u) != netCount(%u)\n", global_stub_index.load(), netCount);
            abort();
        }

#ifdef _DEBUG
        puts("setting kernel args");
#endif
        kernel.set_arg_t(0, 0).value(); // uint series_id,
        if (buffers.empty()) {
            kernel.set_arg(1, svm_routed_lines).value();//global routed_lines* restrict routed,
            kernel.set_arg(2, svm_drawIndirect).value();//global uint4* restrict drawIndirect,
            all_svm.emplace_back(svm_routed_lines.cast<uint8_t>());
            all_svm.emplace_back(svm_drawIndirect.cast<uint8_t>());
        }
        else {
            kernel.set_arg(1, buffers.at(0)).value();//global routed_lines* restrict routed,
            kernel.set_arg(2, buffers.at(1)).value();//global uint4* restrict drawIndirect,
        }
        kernel.set_arg(3, svm_heads).value();
        kernel.set_arg(4, svm_explored).value();
        kernel.set_arg(5, svm_pip_offset_count).value();
        kernel.set_arg(6, svm_pip_tile_body).value();
        kernel.set_arg(7, svm_stubs).value();//constant ushort2* restrict allSourcePos
        kernel.set_arg(8, svm_dirty).value(); // global uint* restrict dirty

        all_svm.emplace_back(svm_heads.cast<uint8_t>());
        all_svm.emplace_back(svm_explored.cast<uint8_t>());
        all_svm.emplace_back(svm_pip_offset_count.cast<uint8_t>());
        all_svm.emplace_back(svm_pip_tile_body.cast<uint8_t>());
        all_svm.emplace_back(svm_stubs.cast<uint8_t>());
        all_svm.emplace_back(svm_dirty.cast<uint8_t>());

        primary_queue.enqueueSVMMigrate<uint8_t>(all_svm);

        return OCL_Node_Router{
            .context{std::move(context)},
            .device_ids{std::move(device_ids)},
            .queues{std::move(queues)},
            .program{std::move(program)},
            .kernels{std::move(kernels)},
            .buffers{std::move(buffers)},
            .svm_routed_lines{svm_routed_lines},
            .svm_drawIndirect{svm_drawIndirect},
            .svm_heads{svm_heads},
            .svm_explored{svm_explored},
            .svm_pip_offset_count{std::move(svm_pip_offset_count)},
            .svm_pip_tile_body{std::move(svm_pip_tile_body)},
            .svm_stubs{std::move(svm_stubs)},
            .svm_dirty{std::move(svm_dirty)},
            .phys{ std::move(phys) },
            .netCount{ std::move(netCount) },
            .netCountAligned{ std::move(netCountAligned) },
            .workgroup_count{ netCountAligned / max_workgroup_size },
            .ocl_counter_max{ std::move(ocl_counter_max) },
            .all_svm{ std::move(all_svm) },
            .net_pairs{ std::move(net_pairs) },
            .site_locations{ std::move(site_locations) },
            .max_workgroup_size{max_workgroup_size},
        };
    }
};