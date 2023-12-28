#version 460

layout(location = 0) out vec4 diffuseColor;
layout (location = 0) uniform vec4 line_color;

void main() {
  diffuseColor = line_color;
}