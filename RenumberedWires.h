#pragma once

class Route_Info {
public:
    union {
        struct {
            uint64_t previous : 32;
            uint64_t past_cost : 16;
            uint64_t future_cost : 16;
        };
        uint64_t value;
    };
    __forceinline uint32_t get_total_cost() const noexcept { return static_cast<uint32_t>(past_cost) + static_cast<uint32_t>(future_cost); }
    __forceinline bool is_used() const noexcept { return value; }
    __forceinline bool is_available() const noexcept { return !value; }
};

static_assert(sizeof(::Route_Info) == 8);

class Route_Info_Comparison {
public:
    std::span<::Route_Info> alt_route_storage;
    Route_Info_Comparison(std::span<::Route_Info> alt_route_storage) : alt_route_storage{ alt_route_storage } {}
    bool operator() (uint32_t left, uint32_t right) {
        return alt_route_storage[left].get_total_cost() > alt_route_storage[right].get_total_cost();
    }
};

class RenumberedWires {
public:
    MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" }; //tested
    MMF_Dense_Sets_u32 alt_nodes{ L"nodes2.bin" }; //tested
    ::MemoryMappedFile wire_to_node_mmf{ L"wire_to_node2.bin" }; //tested
    std::span<uint32_t> alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() }; //tested

    MMF_Dense_Sets_u32 alt_wire_to_pips{ L"wire_to_pips2.bin" };
    MMF_Dense_Sets_u32 alt_node_to_pips{ L"node_to_pips2.bin" };

    ::MemoryMappedFile pips_mmf{ L"pips2.bin" };
    std::span<uint64_t> alt_pips{ pips_mmf.get_span<uint64_t>() };

    MMF_Dense_Sets_u32 alt_site_pin_wires{ L"site_pin_wires2.bin" };
    MMF_Dense_Sets_u32 alt_site_pin_nodes{ L"site_pin_nodes2.bin" };


    ::MemoryMappedFile route_storage_mmf{ L"route_storage2.bin", alt_pips.size() * sizeof(uint64_t)};
    std::span<::Route_Info> alt_route_storage{ pips_mmf.get_span<::Route_Info>() };

    ::Route_Info_Comparison ric{ alt_route_storage };
    using RoutePriorityQueue = std::priority_queue<uint32_t, std::vector<uint32_t>, ::Route_Info_Comparison>;
    RoutePriorityQueue alt_route_options{ ric };

    __forceinline void append(uint32_t pip_idx, uint32_t previous, uint16_t past_cost, uint16_t future_cost) {
        alt_route_storage[pip_idx].previous = previous;
        alt_route_storage[pip_idx].past_cost = past_cost;
        alt_route_storage[pip_idx].future_cost = future_cost;

        alt_route_options.emplace(pip_idx);
    }

    __forceinline void clear_routes() {
        alt_route_options = RoutePriorityQueue{ ric };
        route_storage_mmf.zero();
    }

    __forceinline void init_routes() {

    }

    __forceinline uint32_t get_pip_wire0(uint32_t pip_idx) {
        auto pip_info{ alt_pips[pip_idx] };
        auto wire0{ _bextr_u64(pip_info, 0, 28) };
        return wire0;
    }

    __forceinline uint32_t get_pip_wire1(uint32_t pip_idx) {
        auto pip_info{ alt_pips[pip_idx] };
        auto wire1{ _bextr_u64(pip_info, 32, 28) };
        return wire1;
    }

    __forceinline bool get_pip_directional(uint32_t pip_idx) {
        auto pip_info{ alt_pips[pip_idx] };
        bool directional{ static_cast<bool>(_bextr_u64(pip_info, 63, 1)) };
        return directional;
    }

    __forceinline uint32_t get_pip_node0(uint32_t pip_idx) {
        auto wire0{ get_pip_wire0(pip_idx) };
        return alt_wire_to_node[wire0];
    }

    __forceinline uint32_t get_pip_node1(uint32_t pip_idx) {
        auto wire1{ get_pip_wire1(pip_idx) };
        return alt_wire_to_node[wire1];
    }

    __forceinline uint32_t get_pip_wire0_str(uint32_t pip_idx) {
        auto wire0{ get_pip_wire0(pip_idx) };
        auto wire0_info{ alt_wires.body[wire0] };
        return _bextr_u64(wire0_info, 0, 32);
    }

    __forceinline uint32_t get_pip_wire1_str(uint32_t pip_idx) {
        auto wire1{ get_pip_wire1(pip_idx) };
        auto wire1_info{ alt_wires.body[wire1] };
        return _bextr_u64(wire1_info, 0, 32);
    }

    __forceinline uint32_t get_pip_tile0_str(uint32_t pip_idx) {
        auto wire0{ get_pip_wire0(pip_idx) };
        auto wire0_info{ alt_wires.body[wire0] };
        return _bextr_u64(wire0_info, 32, 32);
    }

    __forceinline static uint32_t find_wire(MMF_Dense_Sets_u64 &alt_wires, uint32_t tile_strIdx, uint32_t wire_strIdx) {
        ULARGE_INTEGER key{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
        auto h{ _mm_crc32_u64(0, key.QuadPart) };
        auto mask{ (alt_wires.size() - 1ui64) };
        auto ret_index{ h & mask };
        auto bucket{ alt_wires[ret_index] };
        for (auto&& q : bucket) {
            if (q == key.QuadPart) {
                uint32_t offset{ static_cast<uint32_t>(&q - alt_wires.body.data()) };
                return offset;
            }
        }
        // DebugBreak();
        return UINT32_MAX;
    }

    __forceinline static uint32_t key_site_pin(uint32_t site_strIdx, uint32_t site_pin_strIdx) {
        ULARGE_INTEGER key{ .u{.LowPart{site_strIdx}, .HighPart{site_pin_strIdx}} };
        return key.QuadPart;
    }

    __forceinline static uint32_t hash_site_pin(uint64_t hash_size, uint64_t key) {
        auto h{ _mm_crc32_u64(0, key) };
        auto mask{ hash_size - 1ui64 };
        auto ret_index{ h & mask };
        return static_cast<uint32_t>(ret_index);
    }

    __forceinline static uint32_t find_site_pin_wire(MMF_Dense_Sets_u32& alt_site_pin_wires, uint32_t site_strIdx, uint32_t site_pin_strIdx) {
        auto key{ key_site_pin(site_strIdx, site_pin_strIdx) };
        auto ret_index{ hash_site_pin(alt_site_pin_wires.size(), key)};
        auto bucket{ alt_site_pin_wires[ret_index] };
        for (auto&& q : bucket) {
            if (q == key) {
                uint32_t offset{ static_cast<uint32_t>(&q - alt_site_pin_wires.body.data()) };
                return offset;
            }
        }
        // DebugBreak();
        return UINT32_MAX;
    }

    __forceinline static uint32_t find_site_pin_node(MMF_Dense_Sets_u32& alt_site_pin_nodes, uint32_t site_strIdx, uint32_t site_pin_strIdx) {
        auto key{ key_site_pin(site_strIdx, site_pin_strIdx) };
        auto ret_index{ hash_site_pin(alt_site_pin_nodes.size(), key) };
        auto bucket{ alt_site_pin_nodes[ret_index] };
        for (auto&& q : bucket) {
            if (q == key) {
                uint32_t offset{ static_cast<uint32_t>(&q - alt_site_pin_nodes.body.data()) };
                return offset;
            }
        }
        // DebugBreak();
        return UINT32_MAX;
    }

    __forceinline uint32_t find_wire(uint32_t tile_strIdx, uint32_t wire_strIdx) {
        return find_wire(alt_wires, tile_strIdx, wire_strIdx);
    }

    __forceinline uint32_t find_site_pin_wire(uint32_t site_strIdx, uint32_t site_pin_strIdx) {
        return find_site_pin_wire(alt_site_pin_wires, site_strIdx, site_pin_strIdx);
    }

    __forceinline uint32_t find_site_pin_node(uint32_t site_strIdx, uint32_t site_pin_strIdx) {
        return find_site_pin_node(alt_site_pin_nodes, site_strIdx, site_pin_strIdx);
    }

    void test_wires(DeviceResources::Device::Reader dev) {
        auto wires{dev.getWires()};
        uint32_t good_wires{};
        uint32_t bad_wires{};

        std::vector<bool> wires_in_nodes(wires.size());

        for (auto&& node : dev.getNodes()) {
            for (auto&& wire : node.getWires()) {
                wires_in_nodes[wire] = true;
            }
        }

        for(uint32_t wire_idx{}; wire_idx < wires.size(); wire_idx++) {

            if (wire_idx && !(wire_idx % 1000000)) {
                // std::print("wire_idx: {}M, good_wires: {}, bad_wires: {}\n", wire_idx / 1000000, good_wires, bad_wires);
            }
            if (!wires_in_nodes.at(wire_idx)) continue;

            auto wire{ wires[wire_idx] };
            auto wire_str{ wire.getWire() };
            auto wire_tile_str{ wire.getTile() };

            auto alt_wire_offset{ find_wire(wire_tile_str, wire_str) };
            if (alt_wire_offset == UINT32_MAX) {
                bad_wires++;
                // std::print("wire_idx:{} not found\n", wire_idx);
                continue;
            }
            ULARGE_INTEGER wire_found{ .QuadPart{alt_wires.body[alt_wire_offset]} };
            auto wire_found_wire_str{ wire_found.LowPart };
            auto wire_found_tile_str{ wire_found.HighPart };

            if (wire_found_wire_str != wire_str) {
                // std::print("wire_idx:{} alt_wire_offset:{} wire_found_wire_str:{}:{} != wire_str:{}:{}\n", wire_idx, alt_wire_offset, wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                // DebugBreak();
                bad_wires++;
                continue;
            }
            if (wire_found_tile_str != wire_tile_str) {
                // std::print("wire_idx:{} alt_wire_offset:{} wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", wire_idx, alt_wire_offset, wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                // DebugBreak();
                bad_wires++;
                continue;
            }
            good_wires++;
        }
        // std::print("good_wires: {}, bad_wires: {}\n", good_wires, bad_wires);
    }

    void test_nodes(DeviceResources::Device::Reader dev) {
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };
        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (nodeIdx && !(nodeIdx % 1000000)) {
                std::print("nodeIdx: {}M\n", nodeIdx / 1000000);
            }

            auto node{ nodes[nodeIdx] };
            auto node_wires{ node.getWires() };
            auto alt_node{ alt_nodes[nodeIdx] };

            if (alt_node.size() != node_wires.size()) {
                DebugBreak();
            }

            for (uint32_t node_wire_idx{}; node_wire_idx < node_wires.size(); node_wire_idx++) {
                auto wire_idx{ node_wires[node_wire_idx] };
                auto wire{ wires[wire_idx] };
                auto alt_wire_offset{ alt_node[node_wire_idx] };
                auto found_nodeIdx{ alt_wire_to_node[alt_wire_offset] };

                if (found_nodeIdx != nodeIdx) {
                    std::print("nodeIdx: {}, found_nodeIdx: {}, node_wire_idx: {}, wire_idx: {}, alt_wire_offset: {} \n",
                        nodeIdx, found_nodeIdx, node_wire_idx, wire_idx, alt_wire_offset);

                    DebugBreak();
                }

                ULARGE_INTEGER wire_found{ .QuadPart{alt_wires.body[alt_wire_offset]} };
                auto wire_found_wire_str{ wire_found.LowPart };
                auto wire_found_tile_str{ wire_found.HighPart };

                auto wire_str{ wire.getWire() };
                auto wire_tile_str{ wire.getTile() };
                if (wire_found_wire_str != wire_str) {
                    // std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_wire_str:{}:{} != wire_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                    DebugBreak();
                }
                if (wire_found_tile_str != wire_tile_str) {
                    // std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                    DebugBreak();
                }
            }
        }
    }

    DECLSPEC_NOINLINE static std::vector<std::vector<uint64_t>> make_wires(DeviceResources::Device::Reader dev) {

        puts("make_wires start");
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };

        std::vector<std::vector<uint64_t>> node_renumber(nodes.size());

#if 0
        std::unordered_map<uint64_t, uint32_t> tile_strIdx_wire_strIdx_to_wire_idx;
        tile_strIdx_wire_strIdx_to_wire_idx.reserve(wires.size());
        for (uint32_t wireIdx{}; wireIdx < wires.size(); wireIdx++) {
            auto wire{ wires[wireIdx] };
            ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
            tile_strIdx_wire_strIdx_to_wire_idx.insert({ key.QuadPart, wireIdx });
        }
        puts("make tile_strIdx_wire_strIdx_to_wire_idx finish\n");
        return tile_strIdx_wire_strIdx_to_wire_idx;
#else
        size_t hash_size{ 1ui64 << (64ui64 - _lzcnt_u64(wires.size())) };
        size_t hash_mask{ hash_size - 1ui64 };
        std::print("hash_size: {}, hash_mask: 0x{:x}\n", hash_size, hash_mask);

        std::vector<std::vector<uint64_t>> ret(hash_size);
        // std::vector<std::vector<uint32_t>> wire_renumber(hash_size);

        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (nodeIdx && !(nodeIdx % 1000000)) {
                std::print("nodeIdx: {}M\n", nodeIdx / 1000000);
            }
            std::vector<uint64_t>& node_renumber_n{ node_renumber.at(nodeIdx) };
            auto node_wires{ nodes[nodeIdx].getWires() };
            node_renumber_n.reserve(node_wires.size());
            for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
                auto wireIdx{ node_wires[node_wireIdx] };
                auto wire{ wires[wireIdx] };

                ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
                auto h{ _mm_crc32_u64(0, key.QuadPart) };
                auto ret_index{ h & hash_mask };
                std::vector<uint64_t>& ret_at{ ret.at(ret_index) };
                ULARGE_INTEGER node_key{ .u{.LowPart{static_cast<uint32_t>(ret_index)}, .HighPart{static_cast<uint32_t>(ret_at.size())}} };
                ret_at.emplace_back(key.QuadPart);
                node_renumber_n.emplace_back(node_key.QuadPart);
            }
        }

#if 0
        for (uint32_t wireIdx{}; wireIdx < wires.size(); wireIdx++) {
            if (!(wireIdx % 1000000)) std::print("wireIdx: {}M\n", wireIdx / 1000000);
            auto wire{ wires[wireIdx] };
            ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
            auto h{ _mm_crc32_u64(0, key.QuadPart) };
            ret.at(h & hash_mask).emplace_back(key.QuadPart);
            wire_renumber.at(h & hash_mask).emplace_back(wireIdx);
        }
#endif
        puts("make_wires finish");

        // MMF_Dense_Sets_u32::make(L"wire_renumber.bin", wire_renumber);
        MMF_Dense_Sets_u64::make(L"wires2.bin", ret);
        return node_renumber;
    }

    DECLSPEC_NOINLINE static void make_node_to_wires(DeviceResources::Device::Reader dev) {

        auto node_renumber{ make_wires(dev) };

        puts("make_node_to_wires start\n");

        auto nodes{ dev.getNodes() };
        auto wires{ dev.getWires() };

        auto alt_wires{ MMF_Dense_Sets_u64{L"wires2.bin"} };

        std::vector<std::vector<uint32_t>> node_final(nodes.size());
        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (!(nodeIdx % 1000000)) std::print("nodeIdx: {}M\n", nodeIdx / 1000000);
            auto node_wires{ node_renumber.at(nodeIdx) };
            decltype(node_final.at(nodeIdx)) node_final_n{ node_final.at(nodeIdx) };
            node_final_n.resize(node_wires.size());

            for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
                auto node_wire_key{ node_wires[node_wireIdx] };
                ULARGE_INTEGER node_key{ .QuadPart{node_wire_key} };
                //.u{ .LowPart{static_cast<uint32_t>(ret_index)}, .HighPart{static_cast<uint32_t>(ret_at.size())} }
                auto ret_index{ node_key.LowPart };
                auto ret_at_size{ node_key.HighPart };
                auto offset{ alt_wires.get_offset(ret_index) + ret_at_size };
                ULARGE_INTEGER wire_found{ .QuadPart{alt_wires.body[offset]} };
                auto wire_found_wire_str{ wire_found.LowPart };
                auto wire_found_tile_str{ wire_found.HighPart };

                auto wire{ wires[nodes[nodeIdx].getWires()[node_wireIdx]] };
                auto wire_str{ wire.getWire() };
                auto wire_tile_str{ wire.getTile() };
                if (wire_found_wire_str != wire_str) {
                    std::print("wire_found_wire_str:{}:{} != wire_str:{}:{}\n", wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                    DebugBreak();
                }
                if (wire_found_tile_str != wire_tile_str) {
                    std::print("wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                    DebugBreak();
                }
                node_final_n[node_wireIdx] = offset;
            }
        }

        MMF_Dense_Sets_u32::make(L"nodes2.bin", node_final);

        puts("make_node_to_wires finish\n");
#endif
    }

    DECLSPEC_NOINLINE static void make_wire_to_node() {
        puts("make_wire_to_node start");

        MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" }; //tested
        MMF_Dense_Sets_u32 alt_nodes{ L"nodes2.bin" }; //tested


        MemoryMappedFile wire_to_node_mmf{ L"wire_to_node2.bin", alt_wires.body.size() * sizeof(uint32_t) };
        auto alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() };
        for (uint32_t node_idx{}; node_idx < alt_nodes.size(); node_idx++) {
            if (node_idx && !(node_idx % 1000000)) {
                std::print("node_idx: {}M\n", node_idx / 1000000);
            }
            for (auto wire_idx : alt_nodes[node_idx]) {
                alt_wire_to_node[wire_idx] = node_idx;
            }
        }

        puts("make_wire_to_node finish");
    }

    DECLSPEC_NOINLINE static void make_wire_to_pips(DeviceResources::Device::Reader dev) {
        puts("start make_wire_to_pips\n");

        MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" }; //tested

        auto alt_wires_body{ alt_wires.body };
        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };

        std::vector<std::vector<uint32_t>> wire_to_pips{ static_cast<size_t>(alt_wires_body.size()) };

        MemoryMappedFile pips_mmf{ L"pips2.bin", 4294967296ui64 };

        auto pips_span{ pips_mmf.get_span<uint64_t>() };

        // std::vector<std::vector<uint64_t>> pips{ 1ui64 };


        uint32_t pips_size{};
        {
            uint32_t pip_idx{};

            for (auto&& tile : tiles) {
                auto tile_strIdx{ tile.getName() };
                auto tileType{ tileTypes[tile.getType()] };
                auto tileType_wires{ tileType.getWires() };
                for (auto&& pip : tileType.getPips()) {
                    if (pip.isPseudoCells()) continue;

                    auto wire0_strIdx{ tileType_wires[pip.getWire0()] };
                    auto wire1_strIdx{ tileType_wires[pip.getWire1()] };


                    // ULARGE_INTEGER key0{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };
                    // ULARGE_INTEGER key1{ .u{.LowPart{wire1_strIdx}, .HighPart{tile_strIdx}} };
                    // auto wire0_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key0.QuadPart) };
                    // auto wire1_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key1.QuadPart) };
                    auto wire0_idx{ find_wire(alt_wires, tile_strIdx, wire0_strIdx) };
                    if (wire0_idx == UINT32_MAX) continue;

                    auto wire1_idx{ find_wire(alt_wires, tile_strIdx, wire1_strIdx) };
                    if (wire1_idx == UINT32_MAX) continue;

                    // ULARGE_INTEGER pipInfo{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };

                    if (pip_idx && !(pip_idx % 1000000)) {
                        std::print("pip_idx: {}M\n", pip_idx / 1000000);
                    }

                    uint32_t pip_idx_forward{ pip_idx | 0x80000000ui32 };
                    uint32_t pip_idx_reverse{ pip_idx };

                    uint64_t pip_info{
                        (static_cast<uint64_t>(wire0_idx) & 0xfffffffui64) |
                        ((static_cast<uint64_t>(wire1_idx) & 0xfffffffui64) << 32ui64) |
                        (pip.getDirectional() << 63ui64) //is directional
                    };

                    pips_span[pip_idx] = pip_info;
                    pip_idx++;

                    if (pip.getDirectional()) {
                        wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                    }
                    else {
                        wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                        wire_to_pips[wire1_idx].push_back(pip_idx_reverse);
                    }
                }
            }
            pips_size = static_cast<uint64_t>(pip_idx) * sizeof(uint64_t);
        }

        auto pips_mmf_shrunk{ pips_mmf.shrink(pips_size) };

        puts("finish make_wire_to_pips\n");

        MMF_Dense_Sets_u32::make(L"wire_to_pips2.bin", wire_to_pips);

    }

    DECLSPEC_NOINLINE static void make_node_to_pips(DeviceResources::Device::Reader dev) {
        puts("start make_node_to_pips");

        ::MemoryMappedFile pips_mmf{ L"pips2.bin" };
        std::span<uint64_t> alt_pips{ pips_mmf.get_span<uint64_t>() };

        MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" }; //tested
        MMF_Dense_Sets_u32 alt_nodes{ L"nodes2.bin" }; //tested

        MemoryMappedFile wire_to_node_mmf{ L"wire_to_node2.bin" };
        auto alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() };

        std::vector<std::vector<uint32_t>> node_to_pips{ alt_nodes.size() };

        for (uint32_t pip_idx{}; pip_idx < alt_pips.size(); pip_idx++) {
            if (pip_idx && !(pip_idx % 1000000)) {
                std::print("pip_idx: {}M\n", pip_idx / 1000000);
            }

            auto pip_info{ alt_pips[pip_idx] };
            auto wire0{ _bextr_u64(pip_info, 0, 28) };
            auto wire1{ _bextr_u64(pip_info, 32, 28) };
            bool directional{ static_cast<bool>(_bextr_u64(pip_info, 63, 1)) };

            auto node0_idx{ alt_wire_to_node[wire0] };
            auto node1_idx{ alt_wire_to_node[wire1] };

            node_to_pips[node0_idx].emplace_back(pip_idx);
            if (!directional) {
                node_to_pips[node1_idx].emplace_back(pip_idx);
            }
        }

        puts("finish make_node_to_pips");

        MMF_Dense_Sets_u32::make(L"node_to_pips2.bin", node_to_pips);
    }

    DECLSPEC_NOINLINE static void make_site_pin_wires(DeviceResources::Device::Reader dev) {
        OutputDebugStringA("make_site_pin_wires start\n");

        MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" }; //tested

        ::MemoryMappedFile wire_to_node_mmf{ L"wire_to_node2.bin" }; //tested
        std::span<uint32_t> alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() }; //tested

        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };
        auto siteTypes{ dev.getSiteTypeList() };
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };

        size_t hash_size{ 1ui64 << (64ui64 - _lzcnt_u64(wires.size())) };
        size_t hash_mask{ hash_size - 1ui64 };
        std::print("hash_size: {}, hash_mask: 0x{:x}\n", hash_size, hash_mask);

        std::vector<std::vector<uint32_t>> site_pin_wires(hash_size);
        std::vector<std::vector<uint32_t>> site_pin_nodes(hash_size);

        for (auto&& tile : tiles) {
            auto tile_strIdx{ tile.getName() };
            auto tileType{ tileTypes[tile.getType()] };
            for (auto&& site : tile.getSites()) {
                auto site_strIdx{ site.getName() };
                auto site_type_idx{ site.getType() };
                auto site_tile_type{ tileType.getSiteTypes()[site_type_idx] };
                auto site_type{ siteTypes[site_tile_type.getPrimaryType()] };
                auto site_pins{ site_type.getPins() };
                auto site_tile_type_wires{ site_tile_type.getPrimaryPinsToTileWires() };
                for (uint32_t pin_idx{}; pin_idx < site_pins.size(); pin_idx++) {
                    auto wire_strIdx{ site_tile_type_wires[pin_idx] };
                    auto sitePin{ site_pins[pin_idx] };
                    auto site_pin_strIdx{ sitePin.getName() };
                    ULARGE_INTEGER site_pin{ .u{.LowPart{site_strIdx}, .HighPart{site_pin_strIdx}} };
                    //ULARGE_INTEGER tile_wire{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
                    // auto wire_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(tile_wire.QuadPart) };
                    auto wire_idx{ find_wire(alt_wires, tile_strIdx, wire_strIdx) };
                    if (wire_idx == UINT32_MAX) {
                        continue;
                    }
                    auto node_idx{ alt_wire_to_node[wire_idx] };

                    auto h{ _mm_crc32_u64(0, site_pin.QuadPart) };
                    auto ret_index{ h & hash_mask };
                    site_pin_wires.at(ret_index).emplace_back(wire_idx);
                    site_pin_nodes.at(ret_index).emplace_back(node_idx);
                }
            }
        }

        OutputDebugStringA("make_site_pin_wires finish\n");
        MMF_Dense_Sets_u32::make(L"site_pin_wires2.bin", site_pin_wires);
        MMF_Dense_Sets_u32::make(L"site_pin_nodes2.bin", site_pin_nodes);
    }

    DECLSPEC_NOINLINE static void test(DeviceResources::Device::Reader dev) {
        RenumberedWires wr;
        wr.test_nodes(dev);
        wr.test_wires(dev);
    }

    DECLSPEC_NOINLINE static void make_pips(DeviceResources::Device::Reader dev) {
        make_node_to_wires(dev);
        make_wire_to_node();
        make_wire_to_pips(dev);
        make_node_to_pips(dev);
        make_site_pin_wires(dev);
        test(dev);
    }

};
