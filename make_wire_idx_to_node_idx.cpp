#include "wire_idx_to_node_idx.h"

static inline const DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };

int main() {
	TimerVal(Wire_Idx_to_Node_Idx::make(dev.root));
	return 0;
}