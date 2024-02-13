#version 460 core

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

layout (local_size_x = WORD_GROUP_SIZE_X, local_size_y = WORD_GROUP_SIZE_Y, local_size_z = WORD_GROUP_SIZE_Z) in;

void main() 
{
   uint index = gl_GlobalInvocationID.x;  
   destination[index] = source[index] * multiplicand;
}