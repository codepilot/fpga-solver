#version 460

layout (location = 0) in vec4 line_color;
layout (location = 0) out vec4 diffuseColor;

void main() {
  diffuseColor = line_color;
}