#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform PushConstants {
    int bufferID;
    int textureID;
};

layout(binding = 0) uniform Buffer {
    vec2 offset;
    float rotation;
    float scale;
    float aspect;
} buffers[];

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec3 rotate(vec3 p, float a)
{
    float s = sin(a);
    float c = cos(a);

    float x = p.x * c - p.y * s;
    float y = p.x * s + p.y * c;

    return vec3(x, y, p.z);
}

void main()
{
    vec3 offset = vec3(buffers[bufferID].offset, 0);
    float rotation = buffers[bufferID].rotation;
    float scale = buffers[bufferID].scale;
    vec3 aspect = vec3(1, buffers[bufferID].aspect, 1);

    gl_Position = vec4((offset + rotate(position, rotation) * scale) * aspect, 1);
    fragUV = uv;
}
