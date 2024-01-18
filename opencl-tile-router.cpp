#include "ocl_tile_router.h"

int main() {
	auto sts{
	OCL_Tile_Router::make({
		// CL_GL_CONTEXT_KHR, 0,
		// CL_WGL_HDC_KHR, 0,
		0, 0,
	}) };
	if(sts.has_value()) return 0;
	std::cout << std::format("status: {}\n", static_cast<int32_t>(sts.error()));
	return 1;
}