#version 460

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec4 vertexColor;
layout (location = 0) uniform vec2 device_dim;

out gl_PerVertex {
	vec4 gl_Position;
};

layout (location = 0) out vec4 line_color;

void main() {
	vec2 scaled_pos = ((vertexPosition / device_dim) * 2.0 - 1.0) * vec2(1.0, -1.0);
	gl_Position = vec4(scaled_pos.xy, 0.0, 1.0);
	line_color = vertexColor; 
}