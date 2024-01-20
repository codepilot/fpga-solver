#include "ocl_tile_router.h"

int main() {
	auto ocltr{ OCL_Tile_Router::make() };
	ocltr.step_all().value();
	for (auto&& queue : ocltr.queues) {
		queue.finish().value();
	}
	return 0;
}