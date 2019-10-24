#version 330

layout (location = 0) in vec3 a_vertex;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 vertex;
out vec3 normal;
out vec2 texcoord;

out vec3 vertex_original;

void main(void)
{
    vertex = view * model * vec4(a_vertex, 1.0);

    mat3 normal_matrix = mat3(view * model);
	normal = normalize(normal_matrix * a_normal);  

    texcoord = a_texcoord;

    //vertices in world space
    vertex_original = a_vertex;

	gl_Position = projection * vertex;
}
