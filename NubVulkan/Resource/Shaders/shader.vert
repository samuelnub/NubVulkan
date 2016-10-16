#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

// Oh lookie here, magic numbers, globals, and within the shader itself?
// Blasphemy beyond the highest order!
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, -0.5),
	vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main()
{
	// Giving a magical Z and W component
	// Coolio, the variable gl_VertexIndex contains the current vert index
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}