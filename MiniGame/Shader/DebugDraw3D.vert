#version 460 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * inPosition;
	outColor = inColor;
}