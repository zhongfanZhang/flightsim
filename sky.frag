#version 330
out vec4 fragColour;

in vec3 texcoord;

uniform samplerCube skybox;
uniform float time;

void main()
{   
    //during the night only the red component of the skybox changes to simulate sunset and sunrise
    if(time > 15.0 ){   
        fragColour = vec4(0.2+ abs(cos(3.1415*(time/15)))*0.2, 0.2, 0.2, 1.0) * texture(skybox, texcoord);
    }
    //during the day the brightness of the skybox changes to simulate the sun
    else{
        fragColour = vec4(0.2 + sin(3.1415*(time/15)) + abs(cos(3.1415*(time/15)))*0.2, 0.2 + sin(3.1415*(time/15)), 0.2 + sin(3.1415*(time/15)), 1.0) * 
                        texture(skybox, texcoord);
    }    
}

