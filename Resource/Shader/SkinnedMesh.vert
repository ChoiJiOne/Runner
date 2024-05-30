#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec4 inWeights;
layout(location = 4) in ivec4 inJoints;

layout(location = 0) out vec2 outTexCoords;
layout(location = 1) out vec3 outWorldPosition;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec4 outWorldPositionInLightSpace;

const int MAX_SIZE = 120;

uniform mat4 world;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightView;
uniform mat4 lightProjection;

uniform mat4 bindPose[MAX_SIZE];
uniform mat4 invBindPose[MAX_SIZE];

void main()
{
    mat4 m0 = (bindPose[inJoints.x] * invBindPose[inJoints.x]) * inWeights.x;
    mat4 m1 = (bindPose[inJoints.y] * invBindPose[inJoints.y]) * inWeights.y;
    mat4 m2 = (bindPose[inJoints.z] * invBindPose[inJoints.z]) * inWeights.z;
    mat4 m3 = (bindPose[inJoints.w] * invBindPose[inJoints.w]) * inWeights.w;

    mat4 skin = m0 + m1 + m2 + m3;
    
    gl_Position = projection * view * world * skin * vec4(inPosition, 1.0f);

    outTexCoords = inTexCoords;
    outWorldPosition = vec3(world * skin * vec4(inPosition, 1.0f));
    outNormal = mat3(transpose(inverse(world * skin))) * inNormal;
    outWorldPositionInLightSpace = lightProjection * lightView * vec4(outWorldPosition, 1.0f);
}