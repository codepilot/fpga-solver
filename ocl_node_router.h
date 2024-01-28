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

class OCL_Node_Router {
public:
    static inline constexpr uint32_t beam_width{ 128ul };
    static inline constexpr uint32_t max_tile_count{ 5884ul };
    static inline constexpr uint32_t tt_body_count{ 4293068ul };
    static inline constexpr size_t largest_ocl_counter_max{ 1024ull };
    static inline constexpr uint32_t series_id_max{ 1ul };

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
    std::vector<ocl::kernel> cloned_kernels;
    std::vector<ocl::buffer> from_gl_buffers;

    std::vector<ocl::buffer> v_buf_dirty;
    std::vector<ocl::buffer> v_buf_routed_lines;
    std::vector<ocl::buffer> v_buf_drawIndirect;
    std::vector<ocl::buffer> v_buf_heads;
    std::vector<ocl::buffer> v_buf_explored;
    std::vector<ocl::buffer> v_buf_stubs;

    std::vector<ocl::buffer> v_buf_pip_offset_count;
    std::vector<ocl::buffer> v_buf_pip_tile_body;

    std::vector<std::array<uint32_t, 4>> v_host_dirty{};

    DeviceResources::Device::Reader dev;
    PhysicalNetlist::PhysNetlist::Reader phys;
    uint32_t netCount;
    uint32_t netCountAligned;
    size_t total_workgroup_count;
    std::vector<size_t> workgroup_counts;
    std::vector<size_t> workgroup_offsets;

    uint32_t ocl_counter_max;
    struct net_pair_t {
        net_reader net;
        branch_reader source;
        branch_reader stub;
    };

    std::vector<net_pair_t> net_pairs;
    std::map<std::string_view, std::array<uint16_t, 2>> site_locations;

    cl_uint max_workgroup_size;

    std::expected<void, ocl::status> step_all(const uint32_t series_id) {

        std::cout << std::format("ocl_counter_max: {}, step {} of {}, ", ocl_counter_max, series_id + 1, series_id_max);
        each(queues, [&](uint64_t queue_index, ocl::queue& queue) {
            decltype(auto) cloned_kernel{ cloned_kernels[queue_index] };
            cloned_kernel.set_arg_t(0, series_id);
            queue.enqueue<1>(cloned_kernel, { workgroup_offsets[queue_index]}, {max_workgroup_size * workgroup_counts[queue_index]}, {max_workgroup_size}).value();
        });
        each(queues, [&](uint64_t queue_index, ocl::queue& queue) {
            queue.enqueueRead<uint32_t>(v_buf_dirty[queue_index], true, 0, v_host_dirty[queue_index]);
            queue.enqueueFillBuffer<uint32_t>(v_buf_dirty[queue_index], 0u);
            std::cout << std::format(" {},{},{},{} ", v_host_dirty[queue_index][0], v_host_dirty[queue_index][1], v_host_dirty[queue_index][2], v_host_dirty[queue_index][3]);
        });
        std::cout << "\n";

        return std::expected<void, ocl::status>();
    }

    std::expected<void, ocl::status> do_all() {
#ifdef _DEBUG
        std::cout << std::format("ocl_counter_max: {}\n", ocl_counter_max);
#endif
        //uint32_t previous_dirty_count{};
        for (uint32_t series_id{}; series_id < series_id_max; series_id++) {
            step_all(series_id).value();

            // if(!host_dirty.front()) break;
            //if (previous_dirty_count != host_dirty.front()) std::cout << "\n";
            //previous_dirty_count = host_dirty.front();
        }
        for (auto&& queue : queues) {
            queue.finish().value();
        }
        puts("");

        return std::expected<void, ocl::status>();
    }

    template<typename T>
    inline static std::vector<T> read_buffer_group(std::span<ocl::queue> queues, std::span<ocl::buffer> buffers) {
        std::vector<std::array<uint32_t, 4>> v_T(ocl::buffer::sum_size_bytes(buffers).value());
        auto s_T{ std::span(v_T) };
        auto s_ui8{ std::span<uint8_t>(reinterpret_cast<uint8_t *>(s_T.data()), s_T.size_bytes()) };

        size_t offset{};
        each(buffers, [&](uint64_t queue_index, ocl::buffer& buffer) {
            auto buf_size{ buffer.size_bytes().value() };
            queues[queue_index].enqueueRead(buffer, true, 0, s_ui8.subspan(offset, offset + buf_size));
            offset += buf_size;
        });
        return v_T;
    }

    void inspect() {
        auto physStrs{ phys.getStrList() };

        auto v_drawIndirect{ read_buffer_group<std::array<uint32_t, 4>>(queues, v_buf_drawIndirect) };
        v_drawIndirect.resize(netCount);

        ::capnp::MallocMessageBuilder message{ };
        PhysicalNetlist::PhysNetlist::Builder b_phys{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

        b_phys.setPart(phys.getPart());
        b_phys.setPlacements(phys.getPlacements());

        auto r_nets{ phys.getPhysNets() };
        auto b_nets{ b_phys.initPhysNets(r_nets.size()) };

        each(r_nets, [&](uint64_t net_idx, net_reader r_net) {
            b_nets.setWithCaveats(net_idx, r_net);
        });
#if 0
        each(v_drawIndirect, [&](uint64_t di_index, std::array<uint32_t, 4>& di) {
                if (di[3] != 1) return;
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

            std::cout << std::format("{}\t{}\t{}\t{}\t{}\n",
                di[0], di[3],
                src_site_names.front().c_str(),
                dst_site_names.front().c_str(),
                physStrs[net.getName()].cStr()
            );

        });
#endif

        b_phys.setPhysCells(phys.getPhysCells());
#if 1
        b_phys.setStrList(phys.getStrList());
#else
        auto devStrs{ dev.getStrList() };
        std::vector<uint32_t> extra_dev_strIdx;
        auto b_strs{ b_phys.initStrList(static_cast<uint32_t>(physStrs.size() + extra_dev_strIdx.size())) };

        each(physStrs, [&](uint64_t strIdx, ::capnp::Text::Reader r_str) {
            b_strs.set(strIdx, r_str);
        });

        each(extra_dev_strIdx, [&](uint64_t extraIdx, uint32_t dev_strIdx) {
            b_strs.set(physStrs.size() + extraIdx, devStrs[dev_strIdx]);
        });
#endif

        b_phys.setSiteInsts(phys.getSiteInsts());
        b_phys.setProperties(phys.getProperties());
        b_phys.setNullNet(phys.getNullNet());

        InterchangeGZ<PhysicalNetlist::PhysNetlist>::write(message, Z_DEFAULT_COMPRESSION);
    }

    std::expected<void, ocl::status> gl_step(const uint32_t series_id) {
        each(queues, [&](uint64_t queue_index, ocl::queue& queue) {
            decltype(auto) cloned_kernel{ cloned_kernels[queue_index] };
            cloned_kernel.set_arg_t(0, series_id);
            auto result{ queue.useGL(from_gl_buffers, [&]()->std::expected<void, ocl::status> {
                auto result{ queue.enqueue<1>(cloned_kernel, { workgroup_offsets[queue_index]}, {max_workgroup_size * workgroup_counts[queue_index]}, {max_workgroup_size}) };
                return result;
            }) };
            if (!result.has_value()) return result;
        });
        return std::expected<void, ocl::status>();
    }

    static void get_site_pins(branch_reader src, std::set<uint32_t> &dst) {
        if (src.getRouteSegment().isSitePin()) dst.insert(src.getRouteSegment().getSitePin().getSite());
        for (auto&& subbranch : src.getBranches()) {
            get_site_pins(subbranch, dst);
        }
    }

    static void get_site_pin_nodes(branch_reader src, std::set<uint32_t>& dst, std::span<const uint32_t> physStrs_to_devStrs, std::unordered_map<uint64_t, uint32_t> &site_pin_to_node) {
        if (src.getRouteSegment().isSitePin()) {
            auto sitePin{ src.getRouteSegment().getSitePin() };
            std::array<uint32_t, 2> site_pin_key{ physStrs_to_devStrs[sitePin.getSite()], physStrs_to_devStrs[sitePin.getPin()] };
            dst.insert(site_pin_to_node.at(std::bit_cast<uint64_t>(site_pin_key)));
        }
        for (auto&& subbranch : src.getBranches()) {
            get_site_pin_nodes(subbranch, dst, physStrs_to_devStrs, site_pin_to_node);
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

    static std::vector<uint32_t> make_wire_idx_to_node_idx(wire_list_reader wires, node_list_reader nodes) {
        std::vector<uint32_t> wire_idx_to_node_idx(static_cast<size_t>(wires.size()), UINT32_MAX);
        each(nodes, [&](uint64_t node_idx, node_reader node) {
            for (auto wire_idx : node.getWires()) {
                wire_idx_to_node_idx[wire_idx] = static_cast<uint32_t>(node_idx);
            }
        });

        return wire_idx_to_node_idx;
    }

    static std::unordered_map<uint64_t, uint32_t> make_site_pin_to_node(tile_list_reader tiles, tile_type_list_reader tile_types, site_type_list_reader site_types, std::span<const uint32_t> wire_idx_to_node_idx) {
        std::unordered_map<uint64_t, uint32_t> site_pin_to_node;
        for (auto&& tile : tiles) {
            auto tile_str_idx{ tile.getName() };
            auto sites{ tile.getSites() };
            auto tileType{ tile_types[tile.getType()] };
            auto tileTypeSiteTypes{ tileType.getSiteTypes() };

            for (auto&& site : tile.getSites()) {
                auto phys_site_name{ site.getName() };
                auto siteTypeInTile{ tileTypeSiteTypes[site.getType()] };
                auto siteType{ site_types[siteTypeInTile.getPrimaryType()] };
                auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
                each(siteType.getPins(), [&](uint64_t pin_index, auto& pin) {
                    auto phys_pin_name{ pin.getName() };
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

    static std::tuple<std::vector<std::array<uint32_t, 4>>, std::vector<beam_t>, std::vector<std::array<uint32_t, 4>>, std::vector<net_pair_t>> make_vecs(
        const uint32_t netCount,
        const uint32_t netCountAligned,
        const uint32_t ocl_counter_max,
        net_list_reader nets,
        std::unordered_map<uint64_t, uint32_t> &site_pin_to_node,
        auto physStrs,
        std::map<std::string_view, std::array<uint16_t, 2>> site_locations,
        std::span<const uint32_t> physStrs_to_devStrs
    ) {
        std::atomic<uint32_t> global_stub_index{};
        std::vector<net_pair_t> net_pairs;
        net_pairs.resize(netCount);

        beam_t a_head{};
        std::array<uint32_t, 4> a_head_item{ UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
        a_head.fill(a_head_item);

        auto v_stubs{ std::vector<std::array<uint32_t, 4>>(static_cast<size_t>(netCountAligned)) };
        auto v_heads{ std::vector<beam_t>(static_cast<size_t>(netCountAligned), a_head) };
        auto v_drawIndirect{ std::vector<std::array<uint32_t, 4>>(static_cast<size_t>(netCountAligned), fill_draw_commands) };

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
                get_site_pin_nodes(source, src_nodes, physStrs_to_devStrs, site_pin_to_node);
                get_site_pin_nodes(stub, dst_nodes, physStrs_to_devStrs, site_pin_to_node);
                if (!src_nodes.size()) return;
                if (!dst_nodes.size()) return;

                std::vector<std::string_view> src_site_names, dst_site_names;
                for (uint32_t site : src_sites) src_site_names.emplace_back(physStrs[site].cStr());
                for (uint32_t site : dst_sites) dst_site_names.emplace_back(physStrs[site].cStr());
                auto posA{ site_locations.at(src_site_names.front()) };
                auto posB{ site_locations.at(dst_site_names.front()) };
                std::array<uint32_t, 4> stub_n{
                    posB[0], //x
                    posB[1], //y
                    *dst_nodes.begin(), // node_idx
                    static_cast<uint32_t>(net_index), //net_idx
                };
                v_stubs[stub_index] = stub_n;
                auto count_offset{ s_pip_count_offset[*src_nodes.begin()] };
                auto pip_offset{ count_offset[1] };
                auto infos{ s_pip_tile_body.subspan(pip_offset, count_offset[0]) };
                if (infos.size() > v_heads[stub_index].size()) {
                    DebugBreak();
                }
                each(infos, [&](uint64_t info_idx, std::array<uint32_t, 4>& info) {
                    if (info_idx >= v_heads[stub_index].size()) return;
                    v_heads[stub_index][info_idx] = std::array<uint32_t, 4>{
                        0, //cost
                            0, //height
                            UINT32_MAX, //parent
                            pip_offset + static_cast<uint32_t>(info_idx), //pip_idx
                    };
                    });

                if (!v_drawIndirect.empty()) {
                    v_drawIndirect[stub_index] = {
                        0,//count
                        1,//instanceCount
                        static_cast<uint32_t>(stub_index) * ocl_counter_max, // first
                        0,// baseInstance
                    };
                }
            });
        });
        if (global_stub_index != netCount) {
            printf("global_stub_index(%u) != netCount(%u)\n", global_stub_index.load(), netCount);
            abort();
        }
        return std::make_tuple( std::move(v_stubs), std::move(v_heads), std::move(v_drawIndirect), std::move(net_pairs) );
    }

    static void block_site_pin(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin, std::span<uint32_t> node_nets, std::span<const uint32_t> physStrs_to_devStrs, std::unordered_map<uint64_t, uint32_t> &site_pin_to_node) {
        auto ps_source_site{ sitePin.getSite() };
        auto ps_source_pin{ sitePin.getPin() };
        // OutputDebugStringA(std::format("block_site_pin({}, {})\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr()).c_str());
        auto ds_source_site{ physStrs_to_devStrs[ps_source_site] };
        auto ds_source_pin{ physStrs_to_devStrs[ps_source_pin] };
        // OutputDebugStringA(std::format("block_site_pin({}, {})\n", dev.strList[ds_source_site].cStr(), dev.strList[ds_source_pin].cStr()).c_str());

        auto source_node_idx{ site_pin_to_node.at(std::bit_cast<uint64_t>(std::array<std::uint32_t, 2>{ds_source_site, ds_source_pin})) };
        node_nets[source_node_idx] = net_idx;
    }

    static void block_pip(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip, std::span<uint32_t> node_nets, std::span<const uint32_t> physStrs_to_devStrs, std::span<uint32_t> inverse_nodes) {
        auto ps_tile{ pip.getTile() };
        auto ps_wire0{ pip.getWire0() };
        auto ps_wire1{ pip.getWire1() };

        auto ds_tile{ physStrs_to_devStrs[ps_tile] };
        auto ds_wire0{ physStrs_to_devStrs[ps_wire0] };
        auto ds_wire1{ physStrs_to_devStrs[ps_wire1] };

        auto wire0_idx{ inverse_wires.at(ds_tile, ds_wire0) };
        auto wire1_idx{ inverse_wires.at(ds_tile, ds_wire1) };

        auto node0_idx{ inverse_nodes[wire0_idx] };
        auto node1_idx{ inverse_nodes[wire1_idx] };

        node_nets[node0_idx] = net_idx;
        node_nets[node1_idx] = net_idx;
    }

    static void block_source_resource(uint32_t net_idx, PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch, std::span<uint32_t> node_nets, std::span<const uint32_t> physStrs_to_devStrs, std::unordered_map<uint64_t, uint32_t> &site_pin_to_node, std::span<uint32_t> inverse_nodes) {
        auto rs{ branch.getRouteSegment() };
        switch (rs.which()) {
        case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: {
            auto belPin{ rs.getBelPin() };
            // block_bel_pin(net_idx, belPin);
            break;
        }
        case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: {
            auto sitePin{ rs.getSitePin() };
            block_site_pin(net_idx, sitePin, node_nets, physStrs_to_devStrs, site_pin_to_node);
            break;
        }
        case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: {
            auto pip{ rs.getPip() };
            block_pip(net_idx, pip, node_nets, physStrs_to_devStrs, inverse_nodes);
            break;
        }
        case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: {
            auto sitePip{ rs.getSitePIP() };
            // block_site_pip(net_idx, sitePip);
            break;
        }
        default:
            abort();
        }
        for (auto&& sub_branch : branch.getBranches()) {
            block_source_resource(net_idx, sub_branch, node_nets, physStrs_to_devStrs, site_pin_to_node, inverse_nodes);
        }
    }


    static void block_resources(uint32_t net_idx, PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet, std::span<uint32_t> node_nets, std::span<const uint32_t> physStrs_to_devStrs, std::unordered_map<uint64_t, uint32_t> &site_pin_to_node, std::span<uint32_t> inverse_nodes) {
        for (auto&& src_branch : physNet.getSources()) {
            block_source_resource(net_idx, src_branch, node_nets, physStrs_to_devStrs, site_pin_to_node, inverse_nodes);
        }
        for (auto&& stub_branch : physNet.getStubs()) {
            block_source_resource(net_idx, stub_branch, node_nets, physStrs_to_devStrs, site_pin_to_node, inverse_nodes);
        }
    }

    static OCL_Node_Router make(
        DeviceResources::Device::Reader dev,
        PhysicalNetlist::PhysNetlist::Reader phys,
        ocl::context context,
        std::vector<cl_uint> gl_buffers = {}//,
    ) {

        auto nets{ phys.getPhysNets() };
        auto physStrs{ phys.getStrList() };
        auto devStrs{ dev.getStrList() };
        const uint32_t netCount{ count_stubs(nets) };
        const uint32_t netCountAligned{ (((netCount + 255ul) >> 8ul) << 8ul) };

        // auto req_devices{ std::ranges::contains(context_properties, CL_GL_CONTEXT_KHR) ? ocl::device::get_gl_devices(context_properties).value() : std::vector<cl_device_id>{} };
        // ocl::context context{ req_devices.size() ? ocl::context::create(context_properties, req_devices).value(): ocl::context::create<CL_DEVICE_TYPE_GPU>(context_properties).value() };
        auto device_ids{ context.get_devices().value() };
        auto dev_max_mem_alloc_size{ ocl::device::get_info_integral<cl_ulong>(device_ids.front(), CL_DEVICE_MAX_MEM_ALLOC_SIZE).value() };
        auto generic_workgroup_size{ ocl::device::get_info_integral<uint64_t>(device_ids.front(), CL_DEVICE_MAX_WORK_GROUP_SIZE).value_or(0) };
        auto amd_preferred_workgroup_size{ ocl::device::get_info_integral<uint64_t>(device_ids.front(), CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_AMD) };
        auto mem_base_addr_align{ ocl::device::get_info_integral<uint64_t>(device_ids.front(), CL_DEVICE_MEM_BASE_ADDR_ALIGN).value_or(0) };
        puts(std::format("mem_base_addr_align: {}", mem_base_addr_align).c_str());
        cl_uint max_workgroup_size{ static_cast<cl_uint>(amd_preferred_workgroup_size.value_or(generic_workgroup_size)) };

        auto queues{ context.create_queues().value() };
        size_t total_workgroup_count{ netCountAligned / max_workgroup_size };
        auto workgroup_counts{ std::vector<size_t>(queues.size(), 0) };
        auto workgroup_offsets{ std::vector<size_t>(queues.size(), 0) };

        for (size_t wc{}; wc < total_workgroup_count; wc++) {
            workgroup_counts[wc % queues.size()]++;
        }

        for (size_t wo{ 1 }; wo < workgroup_offsets.size(); wo++) {
            workgroup_offsets[wo] = workgroup_offsets[wo - 1] + workgroup_counts[wo - 1];
        }

        each(queues, [&](uint64_t queue_index, ocl::queue& queue) {
            std::cout << std::format("workgroup_count: {}, workgroup_offset: {} of total_workgroup_count: {}\n",
            workgroup_counts[queue_index],
            workgroup_offsets[queue_index],
            total_workgroup_count);
        });

        MemoryMappedFile source{ "../kernels/node_router.cl" };
        ocl::program program{ context.create_program(source.get_span<char>()).value() };
        
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
        std::vector<ocl::buffer> from_gl_buffers{ context.from_gl(CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, gl_buffers).value() };

#ifdef _DEBUG
        puts("allocating device buffers");
#endif

        auto site_locations{ TimerVal(make_site_locations(dev)) };
        const auto devStrs_map{ TimerVal(make_devStrs_map(dev)) };

        const auto string_interchange{ TimerVal(make_string_interchange(devStrs_map, devStrs, physStrs)) };
        const auto physStrs_to_devStrs{ std::span(string_interchange.at(0)) };
        const auto devStrs_to_physStrs{ std::span(string_interchange.at(1)) };

        const auto wire_idx_to_node_idx{ TimerVal(make_wire_idx_to_node_idx(dev.getWires(), dev.getNodes())) };

        auto site_pin_to_node{ TimerVal(make_site_pin_to_node(dev.getTileList(), dev.getTileTypeList(), dev.getSiteTypeList(), wire_idx_to_node_idx)) };
 
        auto inverse_nodes{ make_inverse_nodes(dev.getWires(), dev.getNodes() ) };

#ifdef _DEBUG
        puts("making stub locations");
#endif

        auto vecs{ TimerVal(make_vecs(netCount, netCountAligned, ocl_counter_max, nets, site_pin_to_node, physStrs, site_locations, physStrs_to_devStrs)) };
        auto s_drawIndirect{ std::span(std::get<2>(vecs)) };
        auto s_heads{ std::span(std::get<1>(vecs)) };
        auto s_stubs{ std::span(std::get<0>(vecs)) };

#ifdef _DEBUG
        puts("setting kernel args");
#endif



        auto history_item{ std::array<uint32_t, 2>{ UINT32_MAX, UINT32_MAX } };
        history_t a_history;
        a_history.fill(history_item);
        auto v_explored{ std::vector<history_t>(netCountAligned, a_history) };
        auto s_explored{ std::span(v_explored) };

        std::array<uint32_t, 4> a_dirty{};
        std::vector<ocl::buffer> v_buf_dirty;
        std::vector<std::array<uint32_t, 4>> v_host_dirty{};
        
        auto cloned_kernels{ std::move(kernels.front().clone(queues.size()).value()) };

        auto v_node_nets{ std::vector<uint32_t>(static_cast<size_t>(dev.getNodes().size()), UINT32_MAX) };

        std::vector<ocl::buffer> v_buf_routed_lines;
        std::vector<ocl::buffer> v_buf_drawIndirect;
        std::vector<ocl::buffer> v_buf_heads;
        std::vector<ocl::buffer> v_buf_explored;
        std::vector<ocl::buffer> v_buf_stubs;
        std::vector<ocl::buffer> v_buf_pip_offset_count;
        std::vector<ocl::buffer> v_buf_pip_tile_body;
        std::vector<ocl::buffer> v_buf_node_nets;

        const size_t wg_routed_lines{ static_cast<size_t>(ocl_counter_max) * sizeof(std::array<uint16_t, 4>) * max_workgroup_size };
        const size_t wg_drawIndirect{ sizeof(std::array<uint32_t, 4>) * max_workgroup_size };
        const size_t wg_heads{ sizeof(beam_t) * max_workgroup_size };
        const size_t wg_explored{ sizeof(history_t) * max_workgroup_size };
        const size_t wg_stubs{ sizeof(std::array<uint32_t, 4>) * max_workgroup_size };

#if 1
        each(phys.getPhysNets(), [&](uint64_t net_idx, net_reader net) {
            block_resources(net_idx, net, v_node_nets, physStrs_to_devStrs, site_pin_to_node, inverse_nodes);
        });
#endif

        each(cloned_kernels, [&](uint64_t cloned_kernel_index, ocl::kernel &cloned_kernel) {
            v_host_dirty.emplace_back(a_dirty);

            auto wo{ workgroup_offsets[cloned_kernel_index] };
            auto wc{ workgroup_counts[cloned_kernel_index] };

            auto region_routed_lines{ cl_buffer_region {.origin{wo * wg_routed_lines}, .size{wc * wg_routed_lines}} };
            auto region_drawIndirect{ cl_buffer_region {.origin{wo * wg_drawIndirect}, .size{wc * wg_drawIndirect}} };
            auto region_heads{ cl_buffer_region {.origin{wo * wg_heads}, .size{wc * wg_heads}} };
            auto region_explored{ cl_buffer_region {.origin{wo * wg_explored}, .size{wc * wg_explored}} };
            auto region_stubs{ cl_buffer_region {.origin{wo * wg_stubs}, .size{wc * wg_stubs}} };

            decltype(auto) buf_dirty{ v_buf_dirty.emplace_back(context.create_buffer<uint32_t>(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, a_dirty).value()) };
            decltype(auto) buf_routed_lines{ v_buf_routed_lines.emplace_back(context.create_buffer(CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS, region_routed_lines.size).value()) };
            decltype(auto) buf_drawIndirect{ v_buf_drawIndirect.emplace_back(context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, s_drawIndirect.subspan(wo * max_workgroup_size, wc * max_workgroup_size)).value()) };
            decltype(auto) buf_heads{ v_buf_heads.emplace_back(context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, s_heads.subspan(wo * max_workgroup_size, wc * max_workgroup_size)).value()) };
            decltype(auto) buf_explored{ v_buf_explored.emplace_back(context.create_buffer(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, s_explored.subspan(wo * max_workgroup_size, wc * max_workgroup_size)).value()) };
            decltype(auto) buf_stubs{ v_buf_stubs.emplace_back(context.create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, s_stubs.subspan(wo * max_workgroup_size, wc * max_workgroup_size)).value()) };
            decltype(auto) buf_pip_offset_count{ v_buf_pip_offset_count.emplace_back(context.create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, s_pip_count_offset).value()) };
            decltype(auto) buf_pip_tile_body{ v_buf_pip_tile_body.emplace_back(context.create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, s_pip_tile_body).value()) };
            decltype(auto) buf_node_nets{ v_buf_node_nets.emplace_back(context.create_buffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, v_node_nets).value()) };

            //decltype(auto) sub_buf_routed_lines{ v_sub_buf_routed_lines.emplace_back(buf_routed_lines.sub_region(region_routed_lines).value()) };
            //decltype(auto) sub_buf_drawIndirect{ v_sub_buf_drawIndirect.emplace_back(buf_drawIndirect.sub_region(region_drawIndirect).value()) };
            //decltype(auto) sub_buf_heads{ v_sub_buf_heads.emplace_back(buf_heads.sub_region(region_heads).value()) };
            //decltype(auto) sub_buf_explored{ v_sub_buf_explored.emplace_back(buf_explored.sub_region(region_explored).value()) };
            //decltype(auto) sub_buf_stubs{ v_sub_buf_stubs.emplace_back(buf_stubs.sub_region(region_stubs).value()) };

            cloned_kernel.set_arg_t(0, 0).value();                     // 0 ro <duplicate>  const    uint                   series_id,
            cloned_kernel.set_arg(1, buf_dirty).value();               // 1 rw <duplicate>  global   uint*         restrict dirty
            cloned_kernel.set_arg(2, buf_routed_lines).value();        // 2 wo <sub-divide> global   routed_lines* restrict routed,
            cloned_kernel.set_arg(3, buf_drawIndirect).value();        // 3 rw <sub-divide> global   uint4*        restrict drawIndirect,
            cloned_kernel.set_arg(4, buf_heads).value();               // 4 rw <sub-divide> global   beam_t*       restrict heads, //cost height parent pip_idx
            cloned_kernel.set_arg(5, buf_explored).value();            // 5 wo <sub-divide> global   history_t*    restrict explored, //pip_idx parent
            cloned_kernel.set_arg(6, buf_stubs).value();               // 6 ro <sub-divide> constant uint4*        restrict stubs, // x, y, node_idx, net_idx
            cloned_kernel.set_arg(7, buf_pip_offset_count).value();    // 7 ro <share>      constant uint2*        restrict pip_count_offset, // count offset
            cloned_kernel.set_arg(8, buf_pip_tile_body).value();       // 8 ro <share>      constant uint4*        restrict pip_tile_body, // x, y, node0_idx, node1_idx
            cloned_kernel.set_arg(9, buf_node_nets).value();           // 9 ro <share>      constant uint* restrict node_nets

            std::array<ocl::buffer, 9> cloned_kernel_buffers{
                buf_dirty,
                buf_routed_lines,
                buf_drawIndirect,
                buf_heads,
                buf_explored,
                buf_stubs,
                buf_pip_offset_count,
                buf_pip_tile_body,
                buf_node_nets,
            };

            queues[cloned_kernel_index].enqueueMigrate(cloned_kernel_buffers).value();
        });

        return OCL_Node_Router{
            .context{std::move(context)},
            .device_ids{std::move(device_ids)},
            .queues{std::move(queues)},
            .program{std::move(program)},
            .kernels{std::move(kernels)},
            .cloned_kernels{std::move(cloned_kernels)},
            .from_gl_buffers{std::move(from_gl_buffers)},
            .v_buf_dirty{std::move(v_buf_dirty)},
            .v_buf_routed_lines{std::move(v_buf_routed_lines)},
            .v_buf_drawIndirect{std::move(v_buf_drawIndirect)},
            .v_buf_heads{std::move(v_buf_heads)},
            .v_buf_explored{std::move(v_buf_explored)},
            .v_buf_stubs{std::move(v_buf_stubs)},
            .v_buf_pip_offset_count{std::move(v_buf_pip_offset_count)},
            .v_buf_pip_tile_body{std::move(v_buf_pip_tile_body)},
            .v_host_dirty{std::move(v_host_dirty)},
            .dev{ std::move(dev) },
            .phys{ std::move(phys) },
            .netCount{ netCount },
            .netCountAligned{ netCountAligned },
            .total_workgroup_count{ total_workgroup_count },
            .workgroup_counts{workgroup_counts},
            .workgroup_offsets{workgroup_offsets},
            .ocl_counter_max{ ocl_counter_max },
            .net_pairs{ std::move(std::get<3>(vecs)) },
            .site_locations{ std::move(site_locations) },
            .max_workgroup_size{max_workgroup_size},
        };
    }
};