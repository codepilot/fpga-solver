#pragma once

class RenumberedWires {
public:
    MMF_Dense_Sets_u64 alt_wires{ L"wires2.bin" };
    MMF_Dense_Sets_u32 alt_nodes{ L"nodes2.bin" };

    DECLSPEC_NOINLINE uint32_t find_wire(uint32_t tile_strIdx, uint32_t wire_strIdx) {
        ULARGE_INTEGER key{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
        auto h{ _mm_crc32_u64(0, key.QuadPart) };
        auto mask{ (alt_wires.size() - 1ui64) };
        auto ret_index{ h & mask };
        auto bucket{ alt_wires[ret_index] };
        for (auto&& q : bucket) {
            if (q == key.QuadPart) {
                uint32_t offset{ static_cast<uint32_t>(&q - alt_wires.body.data())};
                return offset;
            }
        }
        DebugBreak();
        return UINT32_MAX;
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
                std::print("wire_idx: {}M, good_wires: {}, bad_wires: {}\n", wire_idx / 1000000, good_wires, bad_wires);
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
        std::print("good_wires: {}, bad_wires: {}\n", good_wires, bad_wires);
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

                ULARGE_INTEGER wire_found{ .QuadPart{alt_wires.body[alt_wire_offset]} };
                auto wire_found_wire_str{ wire_found.LowPart };
                auto wire_found_tile_str{ wire_found.HighPart };

                auto wire_str{ wire.getWire() };
                auto wire_tile_str{ wire.getTile() };
                if (wire_found_wire_str != wire_str) {
                    std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_wire_str:{}:{} != wire_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                    DebugBreak();
                }
                if (wire_found_tile_str != wire_tile_str) {
                    std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                    DebugBreak();
                }
            }
        }
    }
};
