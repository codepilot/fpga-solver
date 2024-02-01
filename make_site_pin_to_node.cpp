#include "site_pin_to_node.h"

static inline const DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };
static inline const MemoryMappedFile mmf_v_inverse_wires{ "Inverse_Wires.bin" };
static inline const Inverse_Wires inverse_wires{ mmf_v_inverse_wires.get_span<uint64_t>() };
static inline const MemoryMappedFile mmf_wire_idx_to_node_idx{ "wire_idx_to_node_idx.bin" };
static inline const Wire_Idx_to_Node_Idx wire_idx_to_node_idx{ mmf_wire_idx_to_node_idx.get_span<uint32_t>() };

int main() {
	const auto v_inverse_wires{ TimerVal(Site_Pin_to_Node::make(dev.root, inverse_wires, wire_idx_to_node_idx)) };
	return 0;
}