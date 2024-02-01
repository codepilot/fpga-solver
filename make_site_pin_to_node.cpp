#include "site_pin_to_node.h"

static inline const MemoryMappedFile mmf_v_inverse_wires{ "Inverse_Wires.bin" };
static inline const Inverse_Wires inverse_wires{ mmf_v_inverse_wires.get_span<uint64_t>() };
static inline const DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };

int main() {
	const auto v_inverse_wires{ TimerVal(Site_Pin_to_Node::make(dev.root, inverse_wires)) };
	return 0;
}