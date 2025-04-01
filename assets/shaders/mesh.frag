#version 460
#extension GL_EXT_nonuniform_qualifier : require

//shader input
layout (location = 0) in vec2 uv;
layout (location = 1) flat in int diffuse;
layout (set = 1, binding = 1) uniform sampler2D samplers[];
//output write
layout (location = 0) out vec4 fragColor;

void main() 
{
	fragColor = texture(samplers[diffuse], uv);
}
