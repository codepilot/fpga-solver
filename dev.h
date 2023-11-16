#pragma once

class Dev {
public:
    struct tile_info {
        int64_t minCol;
        int64_t minRow;
        int64_t maxCol;
        int64_t maxRow;
        int64_t numCol;
        int64_t numRow;
        constexpr static tile_info make() noexcept {
            return {
                .minCol{ INT64_MAX },
                .minRow{ INT64_MAX },
                .maxCol{ INT64_MIN },
                .maxRow{ INT64_MIN },
                .numCol{},
                .numRow{},
            };
        }
        __forceinline constexpr size_t size() const noexcept {
            return numCol * numRow;
        }
        __forceinline constexpr void set_tile(std::span<uint32_t> sp_tile_drawing, int64_t col, int64_t row, uint32_t val) const noexcept {
            sp_tile_drawing[col + row * numCol] = val;
        }
    };

    MMF_READONLY mmf;
    MMF_READER<DeviceResources::Device> dev;

    decltype(dev.reader.getStrList()) strList;
    size_t strList_size;

    decltype(dev.reader.getTileList()) tiles;
    size_t tiles_size;

    decltype(dev.reader.getTileTypeList()) tileTypes;
    size_t tileTypes_size;

    decltype(dev.reader.getWires()) wires;
    size_t wires_size;

    decltype(dev.reader.getNodes()) nodes;
    size_t nodes_size;

    std::vector<std::unordered_set<uint32_t>> inbound_node_tiles;
    std::vector<std::unordered_set<uint32_t>> outbound_node_tiles;

    std::vector<uint32_t> tile_strIndex_to_tile;
    std::vector<uint32_t> site_strIndex_to_tile;
    std::vector<uint8_t> tile_strIndex_to_tileType;
    std::vector<uint32_t> wire_to_node;
    std::unordered_map<std::string_view, uint32_t> stringMap;
    tile_info tileInfo;
    std::vector<uint32_t> tile_drawing;
    std::span<uint32_t> sp_tile_drawing;
    //std::unordered_map<uint64_t, uint64_t> tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low;

    std::vector<uint32_t> wire_drawing;
    std::span<uint32_t> sp_wire_drawing;

    __forceinline static tile_info get_tile_info(decltype(tiles) tiles) noexcept {
        auto ret{ tile_info::make() };
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            const auto col{ tile.getCol() };
            const auto row{ tile.getRow() };
            ret.minCol = (col < ret.minCol) ? col : ret.minCol;
            ret.minRow = (row < ret.minRow) ? row : ret.minRow;
            ret.maxCol = (col > ret.maxCol) ? col : ret.maxCol;
            ret.maxRow = (row > ret.maxRow) ? row : ret.maxRow;
        }
        ret.numCol = ret.maxCol - ret.minCol + 1ui16;
        ret.numRow = ret.maxRow - ret.minRow + 1ui16;
        return ret;
    }

    __forceinline static std::vector<uint32_t> get_tile_drawing(decltype(tiles) tiles, tile_info tileInfo) noexcept {
        std::vector<uint32_t> ret(tileInfo.size(), 0);
        std::span<uint32_t> sp_tile_drawing{ ret };;
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            const auto tileType{ tile.getType() };
            const auto col{ tile.getCol() };
            const auto row{ tile.getRow() };
            if (tileType == 56) {
                tileInfo.set_tile(sp_tile_drawing, col, row, 0xffffff00ui32);
            }
        }
        return ret;
    }

    __forceinline void tile_svg() {
        std::ofstream ostrm("vu3p_tiles.svg", std::ios::binary);
        ostrm << R"(<svg style="background-color: white" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 13400 6220">
)";
        ostrm << R"(<defs>
  <style type="text/css"><![CDATA[
    rect { width: 20px; height: 20px; rx: 5px; fill: grey; }
  ]]></style>
  </defs>
)";

        uint32_t tileIdx{};

        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            const auto col{ tile.getCol() };
            const auto row{ tile.getRow() };
            if(tile.getType() == 56)
            ostrm << std::format(R"(    <rect id="{}" x="{}" y="{}" data-type="{}" />
)",
                strList[tileName].cStr(),
                col * 20i32,
                row * 20i32,
                strList[tileTypes[tile.getType()].getName()].cStr());

            tileIdx++;
        }

        /*ostrm << std::format(R"(<set attributeName="viewBox" to="{} {} {} {}" />
)", minCol * 20i32, minRow * 20i32, numCol * 20i32, numRow * 20i32);*/

        ostrm << "</svg>\n";
    }

    __forceinline static std::unordered_map<std::string_view, uint32_t> list_of_strings_map(decltype(strList) list_of_strings) {
        uint32_t strIdx{ 0 };
        std::unordered_map<std::string_view, uint32_t> stringMap{};
        // std::ofstream ostrm("vu3p_strList.json", std::ios::binary);
        //ostrm << "[\n";
        for (auto&& str : list_of_strings) {
            stringMap.insert({ str.cStr(), strIdx });
            //if (strIdx + 1 == list_of_strings.size()) {
            //    auto strout{ std::format("  \"{}\"\n", str.cStr()) };
            //    ostrm << strout;
            //}
            //else {
            //    auto strout{ std::format("  \"{}\",\n", str.cStr()) };
            //    ostrm << strout;
            //}
            strIdx++;
        }
        //ostrm << "]";
        return stringMap;
    }

    __forceinline static std::vector<uint32_t> make_tile_strIndex_to_tile(size_t strList_size, decltype(tiles) tiles) {
        std::vector<uint32_t> ret(strList_size);
        uint32_t tileIdx{};
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            ret[tileName] = tileIdx;
            tileIdx++;
        }
        return ret;
    }
    __forceinline static std::vector<uint32_t> make_site_strIndex_to_tile(size_t strList_size, decltype(tiles) tiles) {
        std::vector<uint32_t> ret(strList_size);
        uint32_t tileIdx{};
        for (auto&& tile : tiles) {
            for (auto&& site : tile.getSites()) {
                const auto siteName{ site.getName() };
                ret[siteName] = tileIdx;
            }
            tileIdx++;
        }
        return ret;
    }

    __forceinline static std::vector<uint8_t> make_tile_strIndex_to_tileType(size_t strList_size, decltype(tiles) tiles) {
        std::vector<uint8_t> ret(strList_size);
        uint32_t tileIdx{};
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            ret[tileName] = tile.getType();
            tileIdx++;
        }
        return ret;
    }

    __forceinline static std::vector<uint32_t> make_wire_to_node(size_t wires_size, decltype(nodes) nodes) {
        std::vector<uint32_t> ret(wires_size);
        uint32_t nodeIdx{};
        for (auto&& node : nodes) {
            for (auto&& wire : node.getWires()) {
                ret[wire] = nodeIdx;
            }
            nodeIdx++;
        }
        return ret;
    }

    class cached_node_lookup {
    public:
        Trivial_Span<const __m128i> indirect;
        Trivial_Span_cu64 direct;
        Trivial_Span_cu64 direct_value;
        __forceinline static uint64_t pow2_roundup(uint64_t v) noexcept {
            return 1ui64 << (63ui64 - (_lzcnt_u64(v) - ((_mm_popcnt_u64(v) > 1i64) ? 1ui64 : 0ui64)));
        }

        static bool update_used_slots(std::vector<bool> temp_used_slots, std::span<uint64_t> idks) noexcept {
            const uint64_t used_slot_mask{ temp_used_slots.size() - 1ui64 };
            __m128i ko{};
            _rdseed64_step(ko.m128i_u64 + 0);
            _rdseed64_step(ko.m128i_u64 + 1);

            for (auto&& idk : idks) {
                const auto h0{ constexpr_siphash::make(ko, idk) };

                uint64_t slot{ used_slot_mask & idk };
                if (temp_used_slots[slot]) {
                    return false;
                }
                temp_used_slots[slot] = true;
            }

            return true;
        }

        DECLSPEC_NOINLINE static void make_cached_node_lookup(decltype(nodes) nodes, decltype(wires) wires) noexcept {
            {
                uint64_t wire_count{ wires.size() };
                uint64_t wire_count_roundup{ pow2_roundup(wire_count) };
                uint64_t indirect_key_count{ wire_count_roundup >> 3ui64 };

                const uint64_t slot_count{ wire_count_roundup << 2ui64 };
                const uint64_t indirect_mask{ indirect_key_count - 1ui64 };
                const uint64_t used_slot_mask{ slot_count - 1ui64 };
                MemoryMappedFile mmf_indirect{ L"indirect.bin", indirect_key_count * sizeof(__m128i) };
                MemoryMappedFile mmf_direct{ L"direct.bin", slot_count * sizeof(uint64_t) };
                MemoryMappedFile mmf_direct_value{ L"direct_value.bin", slot_count * sizeof(uint64_t) };

                std::vector<bool> used_slots(slot_count, false);
                std::vector<std::vector<uint64_t>> indirect_keys(indirect_key_count);
                auto indirect_key_offsets{ mmf_indirect.get_span< __m128i>() };
                uint64_t indirect_key_offset_position{};
                auto slot_value{ mmf_direct.get_span<uint64_t>() };

                //tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.reserve(wires_size);
    #if 0
                {
                    uint32_t nodeIdx{};
                    for (auto&& node : nodes) {
                        auto node_wires{ node.getWires() };
                        for (auto&& wireA_Idx : node_wires) {
                            auto wireA{ wires[wireA_Idx] };
                            auto wireA_tile_strIdx{ wireA.getTile() };
                            auto wireA_wire_strIdx{ wireA.getWire() };

                            ULARGE_INTEGER k{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                            ULARGE_INTEGER v{ .u{.LowPart{nodeIdx}, .HighPart{wireA_Idx}} };
                            //tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart);
                        }
                        nodeIdx++;
                    }
                }
    #endif
                {
                    // uint32_t wireA_Idx{};
                    for (auto&& wireA : wires) {
                        auto wireA_tile_strIdx{ wireA.getTile() };
                        auto wireA_wire_strIdx{ wireA.getWire() };

                        ULARGE_INTEGER k0{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                        __m128i hk0{};
                        const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
                        uint64_t id{ h0.hash & indirect_mask };
                        indirect_keys[id].emplace_back(k0.QuadPart);
                        // ULARGE_INTEGER v{ .u{.LowPart{UINT32_MAX}, .HighPart{wireA_Idx}} };
                        // auto result{ tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart) };
                        //wireA_Idx++;
                        }
                    }
                uint64_t retry_count{};
                {
                    uint64_t ko_index{};
                    for (auto&& idks : indirect_keys) {
                        for (;;) {
                            if ([&]() -> bool {
                                __m128i ko{};
                                _rdseed64_step(ko.m128i_u64 + 0);
                                _rdseed64_step(ko.m128i_u64 + 1);

    #if 0
                                std::vector<bool> temp_used_slots{ used_slots };

                                for (auto&& idk : idks) {
                                    const auto h0{ constexpr_siphash::make(ko, idk) };

                                    uint64_t slot{ used_slot_mask & h0.hash };
                                    if (temp_used_slots[slot]) {
                                        OutputDebugStringA(std::format("indirect_key_offsets.size: {} retry\n", indirect_key_offsets.size()).c_str());
                                        return false;
                                    }
                                    temp_used_slots[slot] = true;
                                }
                                used_slots = temp_used_slots;
    #else
                                std::vector<uint64_t> local_used_slots{};
                                local_used_slots.reserve(idks.size());
                                for (auto&& idk : idks) {
                                    const auto h0{ constexpr_siphash::make(ko, idk) };

                                    uint64_t slot{ used_slot_mask & h0.hash };
                                    if (used_slots[slot] || std::ranges::count(local_used_slots, slot) > 0) {
                                        // OutputDebugStringA(std::format("indirect_key_offsets.size: {} retry\n", indirect_key_offsets.size()).c_str());
                                        retry_count++;
                                        return false;
                                    }
                                    local_used_slots.emplace_back(slot);
                                }
                                uint64_t slot_index{};
                                for (auto&& local_used_slot : local_used_slots) {
                                    used_slots[local_used_slot] = true;
                                    slot_value[local_used_slot] = idks[slot_index];
                                    slot_index++;
                                }

    #endif
                                indirect_key_offsets[indirect_key_offset_position] = ko;
                                indirect_key_offset_position++;
                                return true;
                            }()) break;
                        }
                        if ((indirect_key_offset_position % 100000) == 0)
                            OutputDebugStringA(std::format("indirect_key_offsets.size: {}, retry_count: {}, retry_ratio: {}\n", indirect_key_offset_position, retry_count, static_cast<double_t>(indirect_key_offset_position) / static_cast<double_t>(retry_count)).c_str());
                    }
                }
                OutputDebugStringA(std::format("indirect_key_offsets.size: {}, retry_count: {}, retry_ratio: {}\n", indirect_key_offset_position, retry_count, static_cast<double_t>(indirect_key_offset_position) / static_cast<double_t>(retry_count)).c_str());
            }
            ExitProcess(0);
            //DebugBreak();


        }
        __forceinline static cached_node_lookup open_cached_node_lookup(MemoryMappedFile& mmf_indirect, MemoryMappedFile& mmf_direct, MemoryMappedFile& mmf_direct_value) noexcept {
            return {
                .indirect{mmf_indirect.get_trivial_span<const __m128i>()},
                .direct{mmf_direct.get_trivial_span<const uint64_t>()},
                .direct_value{mmf_direct_value.get_trivial_span<const uint64_t>()},
            };
        }
        DECLSPEC_NOINLINE __forceinline constexpr uint64_t at(uint32_t wire_strIdx, uint32_t tile_strIdx) const noexcept {
            const uint64_t indirect_mask{ indirect.size - 1ui64 };
            const uint64_t direct_mask{ direct.size - 1ui64 };
            const ULARGE_INTEGER k0{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
            __m128i hk0{};
            const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
            __m128i hk1{ indirect.data[h0.hash & indirect_mask] };
            const auto h1{ constexpr_siphash::make(hk1, k0.QuadPart) };

            const auto direct_index{ h1.hash & direct_mask };
            const auto found_key{ direct.data[h1.hash & direct_mask] };
            if (found_key == k0.QuadPart) {
                return direct_value.data[direct_index];
            }
            OutputDebugStringA(std::format("foundAt: {}, v:{}\n", found_key, k0.QuadPart).c_str());

            DebugBreak();
            std::unreachable();
        }
#if 0
        __forceinline constexpr void set_at(uint32_t wire_strIdx, uint32_t tile_strIdx, uint64_t v) noexcept {
            const uint64_t indirect_mask{ indirect.size - 1ui64 };
            const uint64_t direct_mask{ direct.size - 1ui64 };
            const ULARGE_INTEGER k0{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
            __m128i hk0{};
            const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
            __m128i hk1{ indirect.data[h0.hash & indirect_mask] };
            const auto h1{ constexpr_siphash::make(hk1, k0.QuadPart) };

            const auto direct_index{ h1.hash & direct_mask };
            const auto found_key{ direct.data[h1.hash & direct_mask] };
            if (found_key == k0.QuadPart) {
                direct_value.data[direct_index] = v;
                return;
            }
            OutputDebugStringA(std::format("foundAt: {}, v:{}\n", found_key, k0.QuadPart).c_str());

            DebugBreak();
            std::unreachable();

        }
#endif
    };
    static_assert(std::is_trivial_v<cached_node_lookup>&& std::is_standard_layout_v<cached_node_lookup>);

    MemoryMappedFile mmf_indirect;
    MemoryMappedFile mmf_direct;
    MemoryMappedFile mmf_direct_value;
    cached_node_lookup cnl;

    DECLSPEC_NOINLINE Dev():
        mmf{ L"C:\\Users\\root\\Desktop\\validate-routing\\vu3p-unzipped.canon.msg.device" },
        dev{ mmf },

        strList{ dev.reader.getStrList() },
        strList_size{strList.size()},

        tiles{ dev.reader.getTileList() },
        tiles_size{ tiles.size() },

        tileTypes{ dev.reader.getTileTypeList() },
        tileTypes_size{ tileTypes.size() },

        wires{ dev.reader.getWires() },
        wires_size{ wires.size() },

        nodes{ dev.reader.getNodes() },
        nodes_size{ nodes.size() },

        tile_strIndex_to_tile{ make_tile_strIndex_to_tile(strList_size, tiles) },
        site_strIndex_to_tile{ make_site_strIndex_to_tile(strList_size, tiles) },
        tile_strIndex_to_tileType(make_tile_strIndex_to_tileType(strList_size, tiles) ),
        wire_to_node(make_wire_to_node(wires.size(), nodes)),
        stringMap{ list_of_strings_map(strList)},
        tileInfo{ get_tile_info(tiles) },
        tile_drawing{ get_tile_drawing(tiles, tileInfo) },
        sp_tile_drawing{ tile_drawing },
        mmf_indirect{ L"indirect.bin" },
        mmf_direct{ L"direct.bin" },
        mmf_direct_value{ L"direct_value.bin" },
        cnl{ cached_node_lookup::open_cached_node_lookup(mmf_indirect, mmf_direct, mmf_direct_value) }
    {
        // cached_node_lookup::make_cached_node_lookup(nodes, wires);
#if 0
        {
            std::vector<uint32_t> tileTypeCounts(static_cast<size_t>(tileTypes.size()), 0);
            for (auto&& tile : tiles) {
                tileTypeCounts[tile.getType()]++;
            }
            {
                int32_t tileTypeIdx{};
                for (auto&& tileType : tileTypes) {
                    if (tileType.getPips().size()) {
                        OutputDebugStringA(std::format("{}: {}\n", strList[tileType.getName()].cStr(), tileTypeCounts[tileTypeIdx]).c_str());
                    }
                    tileTypeIdx++;
                }
            }
        }
#endif
#if 0
        std::vector<bool> node_cover(static_cast<size_t>(nodes.size()), false);
        std::vector<bool> tile_cover(static_cast<size_t>(tiles.size()), false);
        std::vector<uint32_t> tileType_Cover(static_cast<size_t>(tileTypes.size()), 0ui32);
        std::vector<uint32_t> tileType_Totals(static_cast<size_t>(tileTypes.size()), 0ui32);
        {
            uint32_t wireIdx{};
            for (auto&& wire : wires) {
                const auto tileType{ tile_strIndex_to_tileType[wire.getTile()] };
                const auto nodeIdx{ wire_to_node[wireIdx] };
                if (tileType == 56) {
                    if (!node_cover[nodeIdx]) {
                        for (auto&& node_wireIdx : nodes[nodeIdx].getWires()) {
                            auto node_wire{ wires[node_wireIdx] };
                            auto node_wire_idx{ tile_strIndex_to_tile[node_wire.getTile()] };
                            tile_cover[node_wire_idx] = true;
                        }
                    }
                    node_cover[nodeIdx] = true;
                }
                else if (nodes[nodeIdx].getWires().size() == 1) {
                    node_cover[nodeIdx] = true;
                }
                wireIdx++;
            }
        }
        {
            uint32_t covered_nodes{};
            for (auto&& nc : node_cover) {
                covered_nodes += static_cast<uint32_t>(nc);
            }
            std::print("nodes({}) covered_nodes({}) uncovered_nodes({})\n", nodes.size(), covered_nodes, nodes.size() - covered_nodes);
            uint32_t covered_tiles{};
            uint32_t tileIdx{};
            for (auto&& nc : tile_cover) {
                auto tile{ tiles[tileIdx] };
                tileType_Totals[tile.getType()]++;
                auto tile_type{ tileTypes[tile.getType()] };
                if (nc) {
                    covered_tiles++;
                    tileType_Cover[tile.getType()]++;
                }
                else if (tileTypes[tiles[tileIdx].getType()].getWires().size() == 0) {
                    covered_tiles++;
                    tileType_Cover[tile.getType()]++;
                }
                else {
                    auto row{ tile.getRow() };
                    auto col{ tile.getCol() };
                    auto sites{ tile.getSites().size() };
                    auto pips{ tile_type.getPips().size() };
                    std::string_view rclk{ "RCLK_" };
                    std::string_view type_name{ strList[tile_type.getName()].cStr() };
                    auto found4 = std::ranges::search(type_name, rclk);
                    if (row == 310 || row == 0 || sites == 0 || !found4.empty()) {
                        tileType_Cover[tile.getType()]++;
                    }
                    else {
                        std::print("col({}) row({}) uncovered {}:{}, wires({})\n", col, row, strList[tile.getName()].cStr(), strList[tile_type.getName()].cStr(), tile_type.getWires().size());
                    }
                }
                //covered_tiles += static_cast<uint32_t>(nc);
                tileIdx++;
            }
            std::print("tiles({}) covered_tiles({}) uncovered_tiles({})\n", tiles.size(), covered_tiles, tiles.size() - covered_tiles);
            uint32_t tileTypeIdx{};
            for (auto&& tc : tileType_Cover) {
                auto tile_type{ tileTypes[tileTypeIdx] };
                auto total{ tileType_Totals[tileTypeIdx] };
                //std::string_view rclk{ "RCLK_" };
                //std::string_view type_name{ strList[tile_type.getName()].cStr() };
                //auto found4 = std::ranges::search(type_name, rclk);

                if (total == tc) {
                }
                else {
                    std::print("{}:wires({}), cover({}) of {}\n", strList[tile_type.getName()].cStr(), tile_type.getWires().size(), tc, total);
                }
                tileTypeIdx++;
            }
        }
#endif
        //memset(mmf_direct_value.fp, 0xff, mmf_direct_value.fsize);
        {

            //tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.reserve(wires_size);
#if 0
            if (false) {
                uint32_t nodeIdx{};
                for (auto&& node : nodes) {
                    auto node_wires{ node.getWires() };
                    for (auto&& wireA_Idx : node_wires) {
                        auto wireA{ wires[wireA_Idx] };
                        auto wireA_tile_strIdx{ wireA.getTile() };
                        auto wireA_wire_strIdx{ wireA.getWire() };

                        //auto wireA_tile{ tiles[tile_strIndex_to_tile[wires[wireA].getTile()]] };
                        //auto wireA_col{ wireA_tile.getCol() };
                        //auto wireA_row{ wireA_tile.getRow() };
                        ULARGE_INTEGER k{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                        ULARGE_INTEGER v{ .u{.LowPart{nodeIdx}, .HighPart{wireA_Idx}} };
                        // tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart);

#if 0
                        cnl.set_at(wireA_wire_strIdx, wireA_tile_strIdx, v.QuadPart);
#else
                        auto foundAt{ cnl.at(wireA_wire_strIdx, wireA_tile_strIdx) };
                        if (foundAt != v.QuadPart) {
                            OutputDebugStringA(std::format("nodeIdx: {}, foundAt: {}, v:{}\n", nodeIdx, foundAt, v.QuadPart).c_str());
                            //DebugBreak();
                        }
#endif

                    }
                    if (!(nodeIdx % 1000000ui32)) {
                        OutputDebugStringA(std::format("nodeIdx: {} of {}\n", nodeIdx, nodes_size).c_str());
                    }
                    nodeIdx++;
                }
            }
            if(false) {
                uint32_t wireA_Idx{};
                for (auto&& wireA : wires) {
                    auto wireA_tile_strIdx{ wireA.getTile() };
                    auto wireA_wire_strIdx{ wireA.getWire() };

                    //auto wireA_tile{ tiles[tile_strIndex_to_tile[wires[wireA].getTile()]] };
                    //auto wireA_col{ wireA_tile.getCol() };
                    //auto wireA_row{ wireA_tile.getRow() };
                    ULARGE_INTEGER k{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                    ULARGE_INTEGER v{ .u{.LowPart{UINT32_MAX}, .HighPart{wireA_Idx}} };
                    //auto result{ tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart) };
                    //if (result.second) {
                        // auto foundAt{ cnl.at(wireA_wire_strIdx, wireA_tile_strIdx) };
                        //if (foundAt != k.QuadPart) {
                        //    OutputDebugStringA(std::format("foundAt: {}, v:{}\n", foundAt, v.QuadPart).c_str());
                            // DebugBreak();
                        //}
//                        if (cnl.at(wireA_wire_strIdx, wireA_tile_strIdx) != v.QuadPart) {
//                            DebugBreak();
//                        }
                    //}
                    if(cnl.at(wireA_wire_strIdx, wireA_tile_strIdx) == UINT64_MAX) {
#if 0
                        std::string_view tileStr{ strList[wireA_tile_strIdx].cStr() };
                        std::string_view wireStr{ strList[wireA_wire_strIdx].cStr() };
                        OutputDebugStringA(std::format("wireA_Idx({}) tile({}) wire({})\n", wireA_Idx, tileStr, wireStr).c_str());
#endif
#if 0
                        cnl.set_at(wireA_wire_strIdx, wireA_tile_strIdx, v.QuadPart);
#else
                        auto foundAt{ cnl.at(wireA_wire_strIdx, wireA_tile_strIdx) };
                        if (foundAt != v.QuadPart) {
                            OutputDebugStringA(std::format("foundAt: {}, v:{}\n", foundAt, v.QuadPart).c_str());
                            DebugBreak();
                        }
#endif
                    }
                    if (!(wireA_Idx % 1000000ui32)) {
                        OutputDebugStringA(std::format("wireA_Idx: {} of {}\n", wireA_Idx, wires_size).c_str());
                    }

                    wireA_Idx++;
                }
                ExitProcess(0);
            }
#endif
            // OutputDebugStringA(std::format("wires.size({})\n", tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size()).c_str());
            // OutputDebugStringA(std::format("tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size({})\n", tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size()).c_str());


#if 0
            std::vector<bool> used_nodes(nodes_size, false);
            uint32_t remaining_nodes{ static_cast<uint32_t>(nodes_size) };

            inbound_node_tiles.resize(nodes_size);
            outbound_node_tiles.resize(nodes_size);
            {
                uint32_t tileIndex{};
                for (auto&& tile : tiles) {
                    auto tileStrIdx{ tile.getName() };
                    std::string_view tileStr{ strList[tileStrIdx].cStr() };
                    auto tileTypeIdx{ tile.getType() };
                    auto tileType{ tileTypes[tileTypeIdx] };
                    auto tileTypeWireStringIndcies{ tileType.getWires() };
                    std::vector<uint64_t> tileTypeNodes; tileTypeNodes.reserve(static_cast<size_t>(tileTypeWireStringIndcies.size()));
#if 1
                    for (auto&& tileTypeWireStrIdx : tileTypeWireStringIndcies) {
                        std::string_view wireStr{ strList[tileTypeWireStrIdx].cStr() };
                        //ULARGE_INTEGER k{ .u{.LowPart{tileTypeWireStrIdx}, .HighPart{tileStrIdx}} };
                        auto v{ cnl.at(tileTypeWireStrIdx, tileStrIdx) };
                        tileTypeNodes.emplace_back(v);
                    }

                    for (auto&& pip : tileType.getPips()) {
                        ULARGE_INTEGER wire0_v{ .QuadPart{ tileTypeNodes[pip.getWire0()]} };
                        ULARGE_INTEGER wire1_v{ .QuadPart{ tileTypeNodes[pip.getWire1()]} };
                        auto wire0_nodeIdx{ wire0_v.LowPart };
                        auto wire1_nodeIdx{ wire1_v.LowPart };

                        if (pip.getDirectional()) {
                            if (wire0_nodeIdx != UINT32_MAX) {
                                outbound_node_tiles[wire0_nodeIdx].insert(tileIndex);
                            }
                            if (wire1_nodeIdx != UINT32_MAX) {
                                inbound_node_tiles[wire1_nodeIdx].insert(tileIndex);
                            }

                            if (wire0_nodeIdx != UINT32_MAX && !used_nodes[wire0_nodeIdx]) {
                                used_nodes[wire0_nodeIdx] = true;
                                remaining_nodes--;
                            }
                            if (wire1_nodeIdx != UINT32_MAX && !used_nodes[wire1_nodeIdx]) {
                                used_nodes[wire1_nodeIdx] = true;
                                remaining_nodes--;
                            }
                        }
                        else {
                            if (wire0_nodeIdx != UINT32_MAX) {
                                inbound_node_tiles[wire0_nodeIdx].insert(tileIndex);
                                outbound_node_tiles[wire0_nodeIdx].insert(tileIndex);
                            }
                            if (wire1_nodeIdx != UINT32_MAX) {
                                inbound_node_tiles[wire1_nodeIdx].insert(tileIndex);
                                outbound_node_tiles[wire1_nodeIdx].insert(tileIndex);
                            }

                            if (wire0_nodeIdx != UINT32_MAX && !used_nodes[wire0_nodeIdx]) {
                                used_nodes[wire0_nodeIdx] = true;
                                remaining_nodes--;
                            }
                            if (wire1_nodeIdx != UINT32_MAX && !used_nodes[wire1_nodeIdx]) {
                                used_nodes[wire1_nodeIdx] = true;
                                remaining_nodes--;
                            }

                        }
                    }
                    tileIndex++;
#endif
                }
            }
            OutputDebugStringA(std::format("remaining_nodes {}\n", remaining_nodes).c_str());
#endif
            wire_drawing.resize((tileInfo.numCol - 1i64)* (tileInfo.numRow - 1i64));
            sp_wire_drawing = wire_drawing;
            {
#if 0
                for (auto&& wireB : node_wires) {
                    auto wireB_tile{ tiles[tile_strIndex_to_tile[wires[wireB].getTile()]] };
                    auto wireB_col{ wireB_tile.getCol() };
                    auto wireB_row{ wireB_tile.getRow() };
                    if (wireA_col == wireB_col) {
                        if (wireA_row == wireB_row) {
                            continue;
                        }
                        else if (wireA_row == wireB_row + 1ui16) {
                            //east
                        }
                        else if (wireA_row + 1ui16 == wireB_row) {
                            //west
                        }
            }
                    else if (wireA_row == wireB_row) {

                    }
        }
#endif

            }
        }
	}

};