#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.25, 0.88, 0.82, 1.0);
}