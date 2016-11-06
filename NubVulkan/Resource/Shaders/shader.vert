#version 450
#extension GL_ARB_separate_shader_objects : enable

// our UBO location
// ooh, if you want to specify the descriptor set index, do layout(set = 0...)
// binding is like layout, but for attributes
layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

// our vertex attributes and their respective locations
// unlike the tutorial, I'm gonna use a vec3 for position
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}