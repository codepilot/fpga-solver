#version 460 core

#ifdef _DEBUG
#ifdef GL_EXT_debug_printf
#extension GL_EXT_debug_printf : require
#endif
#endif

#ifdef GL_EXT_subgroup_uniform_control_flow
#extension GL_EXT_subgroup_uniform_control_flow : require
#endif

#ifdef GL_EXT_maximal_reconvergence
#extension GL_EXT_maximal_reconvergence : require
#endif

layout(std430, binding = 0) readonly buffer ssbo_source {
   uint source[ ];
};

layout(std430, binding = 1) buffer ssbo_destination {
   uint destination[ ];
};

layout(push_constant) uniform pc_args {
   uint multiplicand;
};

#ifndef WORD_GROUP_SIZE_X
#define WORD_GROUP_SIZE_X 256
#endif

#ifndef WORD_GROUP_SIZE_Y
#define WORD_GROUP_SIZE_Y 1
#endif

#ifndef WORD_GROUP_SIZE_Z
#define WORD_GROUP_SIZE_Z 1
#endif

layout (constant_id = 0) const uint OFFSET = 0;

layout (local_size_x = WORD_GROUP_SIZE_X, local_size_y = WORD_GROUP_SIZE_Y, local_size_z = WORD_GROUP_SIZE_Z) in;

void main()
#ifdef GL_EXT_subgroup_uniform_control_flow
[[subgroup_uniform_control_flow]]
#endif
#ifdef GL_EXT_maximal_reconvergence
[[maximally_reconverges]]
#endif
{
   uint index = gl_GlobalInvocationID.x;  
   destination[index] = source[index] * multiplicand + OFFSET;
#ifdef _DEBUG
#ifdef GL_EXT_debug_printf
   debugPrintfEXT("index: %u, multiplicand: %u, OFFSET: %u\n", index, multiplicand, OFFSET);
#endif
#endif
}
