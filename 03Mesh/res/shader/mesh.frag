#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

layout(push_constant, std430) uniform PushConstants {
    int bufferID;
    int textureID;
};

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    fragColor = vec4(texture(textures[textureID], fragUV).rgb, 1);
}
