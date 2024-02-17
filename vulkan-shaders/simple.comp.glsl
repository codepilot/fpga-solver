#version 460 core

#extension GL_EXT_buffer_reference : require
// #extension GL_EXT_buffer_reference2 : require

#ifdef _DEBUG
#ifdef GL_EXT_debug_printf
#extension GL_EXT_debug_printf : require
#endif
#endif

// #ifdef GL_EXT_subgroup_uniform_control_flow
// #extension GL_EXT_subgroup_uniform_control_flow : require
// #endif

#ifdef GL_EXT_maximal_reconvergence
#extension GL_EXT_maximal_reconvergence : require
#endif

layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer ssbo_source {
    restrict uint source[];
};

layout(std430, buffer_reference, buffer_reference_align = 4) writeonly buffer ssbo_destination {
    restrict uint destination[];
};

layout(push_constant) uniform pc_args {
   restrict ssbo_source src_buf;
   restrict ssbo_destination dst_buf;
   uint multiplicand;
};

layout(local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;
layout (constant_id = 3) const uint OFFSET = 0;

// layout (local_size_x = WORD_GROUP_SIZE_X, local_size_y = WORD_GROUP_SIZE_Y, local_size_z = WORD_GROUP_SIZE_Z) in;

void main()
// #ifdef GL_EXT_subgroup_uniform_control_flow
// [[subgroup_uniform_control_flow]]
// #endif
#ifdef GL_EXT_maximal_reconvergence
[[maximally_reconverges]]
#endif
{
   uint global_width = gl_NumWorkGroups.x * gl_WorkGroupSize.x;
   uint index = gl_GlobalInvocationID.x + gl_WorkGroupID.y * global_width;

   dst_buf.destination[index] = src_buf.source[index] * multiplicand + OFFSET;
#ifdef _DEBUG
#ifdef GL_EXT_debug_printf
   // debugPrintfEXT("index: %u, multiplicand: %u, OFFSET: %u\n", index, multiplicand, OFFSET);
#endif
#endif
}
