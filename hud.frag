#version 330

out vec4 fragColour;
uniform int display;

void main(void)
{
    vec4 colour;
    //cyan colour for altitude bar
    if (display == 0){
        colour = vec4(1.0, 0.0, 0.0, 1.0);
    //red colour for power bar
    }else{
        colour = vec4(0.0, 1.0, 1.0, 1.0);
    }

    fragColour = colour; 	
}
