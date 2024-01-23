#include "ocl_tile_router.h"

int main() {
	DevFlat dev{ "_deps/device-file-src/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
	auto ocltr{ OCL_Tile_Router::make(dev.root, phys.root) };
	ocltr.step_all(0).value();
	for (auto&& queue : ocltr.queues) {
		queue.finish().value();
	}
	return 0;
}