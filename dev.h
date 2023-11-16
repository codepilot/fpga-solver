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
    std::unordered_map<uint64_t, uint64_t> tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low;

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
        sp_tile_drawing{ tile_drawing }
    {
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
        {

            tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.reserve(wires_size);
            {
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
                        tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart);
                    }
                    nodeIdx++;
                }
            }
            {
                uint32_t wireA_Idx{};
                for (auto&& wireA : wires) {
                    auto wireA_tile_strIdx{ wireA.getTile() };
                    auto wireA_wire_strIdx{ wireA.getWire() };

                    //auto wireA_tile{ tiles[tile_strIndex_to_tile[wires[wireA].getTile()]] };
                    //auto wireA_col{ wireA_tile.getCol() };
                    //auto wireA_row{ wireA_tile.getRow() };
                    ULARGE_INTEGER k{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                    ULARGE_INTEGER v{ .u{.LowPart{UINT32_MAX}, .HighPart{wireA_Idx}} };
                    auto result{ tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart) };
#if 0
                    if (result.second) {
                        std::string_view tileStr{ strList[wireA_tile_strIdx].cStr() };
                        std::string_view wireStr{ strList[wireA_wire_strIdx].cStr() };
                        OutputDebugStringA(std::format("wireA_Idx({}) tile({}) wire({})\n", wireA_Idx, tileStr, wireStr).c_str());
                    }
#endif
                    wireA_Idx++;
                }
            }
            OutputDebugStringA(std::format("wires.size({})\n", tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size()).c_str());
            OutputDebugStringA(std::format("tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size({})\n", tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.size()).c_str());

            std::vector<bool> used_nodes(nodes_size, false);
            uint32_t remaining_nodes{ static_cast<uint32_t>( nodes_size )};

            inbound_node_tiles.resize(nodes_size);
            outbound_node_tiles.resize(nodes_size);

            wire_drawing.resize((tileInfo.numCol - 1i64) * (tileInfo.numRow - 1i64));
            sp_wire_drawing = wire_drawing;
#if 1
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
                        ULARGE_INTEGER k{ .u{.LowPart{tileTypeWireStrIdx}, .HighPart{tileStrIdx}} };
                        auto v{ tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.at(k.QuadPart) };
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
#endif
            OutputDebugStringA(std::format("remaining_nodes {}\n", remaining_nodes).c_str());
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