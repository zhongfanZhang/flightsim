#version 330
layout (location = 0) in vec3 a_vertex;

out vec3 texcoord;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    texcoord = a_vertex;
    gl_Position = projection * view * vec4(a_vertex, 1.0);
} 
