#include "ocl_tile_router.h"

int main() {
	auto sts{
	OCL_Tile_Router::make({
		// CL_GL_CONTEXT_KHR, 0,
		// CL_WGL_HDC_KHR, 0,
		0, 0,
	}) };
	return 1;
}