#pragma once

class Dev {
public:
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

    std::vector<uint32_t> tile_strIndex_to_tile;
    std::vector<uint8_t> tile_strIndex_to_tileType;
    std::vector<uint32_t> wire_to_node;
    std::unordered_map<std::string_view, int32_t> stringMap;

    inline void tile_svg() {
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
        uint16_t minCol{ UINT16_MAX };
        uint16_t minRow{ UINT16_MAX };
        uint16_t maxCol{};
        uint16_t maxRow{};
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            const auto col{ tile.getCol() };
            const auto row{ tile.getRow() };
            minCol = (col < minCol) ? col : minCol;
            minRow = (row < minRow) ? row : minRow;
            maxCol = (col > maxCol) ? col : maxCol;
            maxRow = (row > maxRow) ? row : maxRow;
            if(tile.getType() == 56)
            ostrm << std::format(R"(    <rect id="{}" x="{}" y="{}" data-type="{}" />
)",
                strList[tileName].cStr(),
                col * 20i32,
                row * 20i32,
                strList[tileTypes[tile.getType()].getName()].cStr());

            tileIdx++;
        }
        uint16_t numCol = maxCol - minCol + 1ui16;
        uint16_t numRow = maxRow - minRow + 1ui16;
        /*ostrm << std::format(R"(<set attributeName="viewBox" to="{} {} {} {}" />
)", minCol * 20i32, minRow * 20i32, numCol * 20i32, numRow * 20i32);*/

        ostrm << "</svg>\n";
    }

    inline static std::unordered_map<std::string_view, int32_t> list_of_strings_map(decltype(strList) list_of_strings) {
        int32_t strIdx{ 0 };
        std::unordered_map<std::string_view, int32_t> stringMap{};
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

    inline static std::vector<uint32_t> make_tile_strIndex_to_tile(size_t strList_size, decltype(tiles) tiles) {
        std::vector<uint32_t> ret(strList_size);
        uint32_t tileIdx{};
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            ret[tileName] = tileIdx;
            tileIdx++;
        }
        return ret;
    }

    inline static std::vector<uint8_t> make_tile_strIndex_to_tileType(size_t strList_size, decltype(tiles) tiles) {
        std::vector<uint8_t> ret(strList_size);
        uint32_t tileIdx{};
        for (auto&& tile : tiles) {
            const auto tileName{ tile.getName() };
            ret[tileName] = tile.getType();
            tileIdx++;
        }
        return ret;
    }

    inline static std::vector<uint32_t> make_wire_to_node(size_t wires_size, decltype(nodes) nodes) {
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

    Dev():
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
        tile_strIndex_to_tileType(make_tile_strIndex_to_tileType(strList_size, tiles) ),
        wire_to_node(make_wire_to_node(wires.size(), nodes)),
        stringMap{ list_of_strings_map(strList)}
    {
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
          //  for (auto &&tile: tiles) {
            //}

	}

};