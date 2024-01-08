#pragma once

#include <vector>
#include "RenumberedWires.h"
#include "interchange_types.h"

class NodeStorage {
public:
    std::vector<uint32_t> stored_nodes_nets;
    const RenumberedWires& rw;

    inline constexpr NodeStorage(size_t alt_nodes_size, const RenumberedWires& rw) noexcept : rw{ rw } {
        stored_nodes_nets.resize(alt_nodes_size, UINT32_MAX);
    }

    inline constexpr bool is_node_stored(uint32_t net_idx, uint32_t node_idx) const noexcept {
        auto stored_net{ stored_nodes_nets[node_idx] };
        if (stored_net != UINT32_MAX && stored_net != net_idx) return true;
        return false;
    }

    inline bool are_nodes_stored(uint32_t net_idx, branch_builder_map& nodes /* not constexpr */) const noexcept {
        for (auto&& node : nodes) {
            if (is_node_stored(net_idx, node.first)) return true;
        }
        return false;
    }

    inline constexpr bool is_pip_stored(uint32_t net_idx, PIP_Index pip_idx) const noexcept {
        if (pip_idx.is_root()) return false;
        auto pip_node_in{ rw.get_pip_node_in(pip_idx) };
        auto pip_node_out{ rw.get_pip_node_out(pip_idx) };
        if (is_node_stored(net_idx, pip_node_in)) return true;
        if (is_node_stored(net_idx, pip_node_out)) return true;
        return false;
    }

    inline constexpr void store_node(uint32_t net_idx, uint32_t node_idx) noexcept {
        if (stored_nodes_nets[node_idx] != UINT32_MAX && stored_nodes_nets[node_idx] != net_idx) {
            puts(std::format("store net_idx:{} blocking conflict stored_nodes_nets[node_idx:{}]={}", net_idx, node_idx, stored_nodes_nets[node_idx]).c_str());
            abort();
        }

        stored_nodes_nets[node_idx] = net_idx;
    }

    inline constexpr void store_pip(uint32_t net_idx, PIP_Index pip_idx) noexcept {
        auto node_in{ rw.get_pip_node_in(pip_idx) };
        auto node_out{ rw.get_pip_node_out(pip_idx) };

        if (stored_nodes_nets[node_in] != UINT32_MAX && stored_nodes_nets[node_in] != net_idx) {
            puts(std::format("store net_idx:{} blocking conflict f:{} stored_nodes_nets[node_in:{}]={}", net_idx, pip_idx.is_pip_forward(), node_in, stored_nodes_nets[node_in]).c_str());
            abort();
        }

        if (stored_nodes_nets[node_out] != UINT32_MAX && stored_nodes_nets[node_out] != net_idx) {
            puts(std::format("store net_idx:{} blocking conflict f:{} stored_nodes_nets[node_out:{}]={}", net_idx, pip_idx.is_pip_forward(), node_out, stored_nodes_nets[node_out]).c_str());
            abort();
        }
        stored_nodes_nets[node_in] = net_idx;
        stored_nodes_nets[node_out] = net_idx;
    }

};

