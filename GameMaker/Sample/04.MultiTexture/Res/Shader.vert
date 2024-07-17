#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoords;

void main()
{
	gl_Position = vec4(inPosition, 1.0f);
	outColor = inColor;
	outTexCoords = inTexCoords;
}