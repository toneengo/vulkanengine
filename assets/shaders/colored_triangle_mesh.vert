#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 uv;
layout (location = 1) out int diffuse;

layout(set = 0, binding = 0) uniform CameraData{   
	mat4 view;
	mat4 proj;
} camera;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
    int diffuse;
    int normal;
    int specular;
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{	
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = camera.proj * camera.view * vec4(v.position, 1.0f);
    diffuse = PushConstants.diffuse;
	uv.x = v.uv_x;
	uv.y = v.uv_y;
}
