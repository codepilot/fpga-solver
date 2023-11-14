#version 460

layout (location = 0) in vec2 vertexPosition;
layout (location = 0) uniform vec2 device_dim;

void main() {
	vec2 scaled_pos = ((vertexPosition / device_dim) * 2.0 - 1.0) * vec2(1.0, -1.0);
	gl_Position = vec4(scaled_pos.xy, 0.0, 1.0);
}