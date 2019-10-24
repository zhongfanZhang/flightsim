#version 330

in vec3 vertex;

uniform mat4 projection;
uniform mat4 HUDTranslate;

void main(void)
{  
    gl_Position = projection * HUDTranslate * vec4(vertex, 1.0);
}
