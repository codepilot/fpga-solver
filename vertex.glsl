#version 460

layout (location = 0) in vec2 vertexPosition;

void main() {
  gl_Position = vec4(vertexPosition.x / 670.0 * 2.0 - 1.0, (vertexPosition.y / 311.0 * 2.0 - 1.0) * -1.0, 0.0, 1.0);
}