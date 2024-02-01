#pragma once

#include <span>
#include <cstdint>
#include "InterchangeGZ.h"
#include "interchange_types.h"
#include "Timer.h"
#include "each.h"

class Wire_Idx_to_Node_Idx: public std::span<uint32_t> {
public:
	static MemoryMappedFile make(device_reader devRoot) noexcept {
        wire_list_reader wires{ devRoot.getWires() };
        node_list_reader nodes{ devRoot.getNodes() };
        MemoryMappedFile mmf_wire_idx_to_node_idx{ "wire_idx_to_node_idx.bin", static_cast<size_t>(wires.size()) * sizeof(uint32_t) };
        auto s_wire_idx_to_node_idx{ mmf_wire_idx_to_node_idx.get_span<uint32_t>() };
        std::ranges::fill(s_wire_idx_to_node_idx, UINT32_MAX);

        jthread_each(nodes, [&](uint64_t node_idx, node_reader node) {
            for (auto wire_idx : node.getWires()) {
                s_wire_idx_to_node_idx[wire_idx] = static_cast<uint32_t>(node_idx);
            }
        });

        jthread_each(nodes, [&](uint64_t node_idx, node_reader node) {
            for (auto wire_idx : node.getWires()) {
                if (s_wire_idx_to_node_idx[wire_idx] != static_cast<uint32_t>(node_idx)) abort();
            }
        });

        return mmf_wire_idx_to_node_idx;
    }
};