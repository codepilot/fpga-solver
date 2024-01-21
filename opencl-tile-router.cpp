#include "ocl_tile_router.h"

int main() {
	PhysGZ phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
	auto ocltr{ OCL_Tile_Router::make(phys.root) };
	ocltr.step_all(0).value();
	for (auto&& queue : ocltr.queues) {
		queue.finish().value();
	}
	return 0;
}