#version 330

in vec4 vertex;
in vec3 normal;
in vec2 texcoord;

in vec3 vertex_original;

out vec4 fragColour;

uniform mat4 view;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform int currentObject;
uniform float time; 

//euclidean distance between two points are calculated for use in light attenuation
float euclideanDistance(in vec4 light_pos, in vec4 position)
{
    float distance = (light_pos.x - position.x)*(light_pos.x - position.x);
    distance += (light_pos.y - position.y)*(light_pos.y - position.y);
    distance += (light_pos.z - position.z)*(light_pos.z - position.z);
    distance = sqrt(distance);
    return distance;
}

//function for the sun (directional light)
vec3 blinnphongPointLight(in vec4 position, in vec3 norm)
{
 
    vec4 light_pos;

    //sun is moved across the sky as the day progresses
    if (time < 15){
        light_pos = vec4(-1000*cos(3.1415*(time/15)), 1000.0, 0.0, 1.0);
    }else{
        light_pos = vec4(-1000*cos(3.1415*(time/15)), 1000.0, 0.0, 1.0);
    }

    vec3 light_ambient = vec3(0.2, 0.2, 0.2);

    //sun brightness is changed throughout the day and set to a constant at night
    //redness is changed throughout the day and night to simulate the sunset
    vec3 light_diffuse;
    if (time > 15){
        light_diffuse = vec3(0.2 + abs(cos(3.1415*(time/15)))*0.2, 0.2, 0.2);
    }else{
        light_diffuse = vec3(0.2 + sin(3.1415*(time/15)) + abs(cos(3.1415*(time/15)))*0.2, 0.2 + sin(3.1415*(time/15)), 0.2 + sin(3.1415*(time/15)));
    }

    vec3 light_specular = vec3(0.0, 0.0, 0.0);
    vec3 mtl_ambient = vec3(0.0, 0.0, 0.0);

    //brown colour is used for the plane
    //white colour is used for the light poles, terrain and trees
    vec3 mtl_diffuse;
    if (currentObject == 0){
        mtl_diffuse = vec3(95.0/255.0, 83.0/255.0, 70.0/255.0);
    }else{
        mtl_diffuse = vec3(1.0, 1.0, 1.0);
    }

    vec3 mtl_specular = vec3(0.0, 0.0, 0.0);
    float mtl_shininess = 32.0;

    //incoming light is multiplied by view matrix to make it constant with respect to the camera
    //vertex position is not considered as it directional light
    vec3 s = normalize(vec3(view*light_pos));
    vec3 v = normalize(-position.xyz);
    vec3 r = reflect( -s, norm );
    
    // The ambient component
    vec3 ambient = light_ambient * mtl_ambient;
	
    // The diffuse component
    float sDotN = max( dot(s,norm), 0.0 );
    vec3 diffuse;
    //plane and light poles are not textured
    if (currentObject == 0 || currentObject == 2){
        diffuse = light_diffuse * mtl_diffuse * sDotN;
    //terrain is set to grass, rock or snow depending on the height
    }else{
        if (vertex_original.y >256){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex2, texcoord));
        }else if (vertex_original.y < 150){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex0, texcoord));
        }else{
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex1, texcoord));
        }
    }

    // The specular component
    vec3 h = normalize(s + v);
    vec3 spec = vec3(0.0);
    if ( sDotN > 0.0 ){
		spec = light_specular * mtl_specular * pow( max( dot(norm,h), 0.0 ), 2*mtl_shininess );
    }

    if (gl_FrontFacing){
        return diffuse;
    }else{
        return vec3(0.0, 0.0, 0.0);
    }
}

//function for the plane headlight (point light)
vec3 blinnphongPointLight2(in vec4 position, in vec3 norm)
{

    vec4 light_pos = vec4(0.0, 1.0, 0.0, 1.0);
    vec3 light_ambient = vec3(0.0, 0.0, 0.0);
    vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
    vec3 light_specular = vec3(1.0, 1.0, 1.0);
    vec3 mtl_ambient = vec3(0.0, 0.0, 0.0);

    vec3 mtl_diffuse;
    if (currentObject == 0){
        mtl_diffuse = vec3(95.0/255.0, 83.0/255.0, 70.0/255.0);
    }else{
        mtl_diffuse = vec3(1.0, 1.0, 1.0);
    }

    vec3 mtl_specular = vec3(1.0, 1.0, 1.0);
    float mtl_shininess = 32.0;

    //incoming light is not multiplied by view or model matrix so it follows the plane
    vec3 s = normalize(vec3(light_pos-position));
    vec3 v = normalize(-position.xyz);
    vec3 r = reflect( -s, norm );
    
    // The ambient component
    vec3 ambient = light_ambient * mtl_ambient;
	
    // The diffuse component
    float sDotN = max( dot(s,norm), 0.0 );

    vec3 diffuse;
    //attenuation is done based on the squared distance from the plane
    float distance = euclideanDistance(light_pos, position);
    light_diffuse = (1000*light_diffuse)/(distance*distance);
    if (light_diffuse.x > 1.0){
        light_diffuse = light_diffuse / light_diffuse.x;
    }
    if (light_diffuse.y > 1.0){
        light_diffuse = light_diffuse / light_diffuse.y;
    }
    if (light_diffuse.z > 1.0){
        light_diffuse = light_diffuse / light_diffuse.z;
    }

    if (currentObject == 0 || currentObject == 2){
        diffuse = light_diffuse * mtl_diffuse * sDotN;
    }else{
        if (vertex_original.y >256){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex2, texcoord));
        }else if (vertex_original.y < 150){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex0, texcoord));
        }else{
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex1, texcoord));
        }
    }

    // The specular component
    vec3 h = normalize(s + v);
    vec3 spec = vec3(0.0);
    if ( sDotN > 0.0 ){
		spec = light_specular * mtl_specular * pow( max( dot(norm,h), 0.0 ), 2*mtl_shininess );
    }
    
    if (gl_FrontFacing){
        return diffuse;
    }else{
        return vec3(0.0, 0.0, 0.0);
    }
}

//function for the light poles on runway (spot light)
vec3 blinnphongPointLight3(in vec4 position, in vec3 norm, in vec4 pole_pos)
{

    vec3 light_ambient = vec3(0.0, 0.0, 0.0);
    vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
    vec3 light_specular = vec3(1.0, 1.0, 1.0);
    vec3 mtl_ambient = vec3(0.0, 0.0, 0.0);

    vec3 mtl_diffuse;
    if (currentObject == 0){
        mtl_diffuse = vec3(95.0/255.0, 83.0/255.0, 70.0/255.0);
    }else{
        mtl_diffuse = vec3(1.0, 1.0, 1.0);
    }

    vec3 mtl_specular = vec3(1.0, 1.0, 1.0);
    float mtl_shininess = 32.0;

    vec3 s = normalize(vec3(view*pole_pos-position));
    vec3 v = normalize(-position.xyz);
    vec3 r = reflect( -s, norm );
    
    // The ambient component
    vec3 ambient = light_ambient * mtl_ambient;
	
    // The diffuse component
    float sDotN = max( dot(s,norm), 0.0 );

    vec3 diffuse;
    //attenuation is calculated based on distance
    float distance = euclideanDistance(view*pole_pos, position);
    light_diffuse = (200*light_diffuse)/(distance*distance);
    
    //angle between incoming light and spotlight cone is calculated for spotlight 
    vec3 cone_direction = vec3(view*vec4(0.0, -1.0, 0.0, 0.0));
    vec3 ray_direction = -s;
    float angle = acos(dot(ray_direction, cone_direction));
    
    //attenuation is done for angles outside the spotlight cone
    if (angle > 3.14159/8.0){
        light_diffuse = (0.5/(angle*angle))*light_diffuse;
    }

    if (light_diffuse.x > 1.0){
        light_diffuse = light_diffuse / light_diffuse.x;
    }
    if (light_diffuse.y > 1.0){
        light_diffuse = light_diffuse / light_diffuse.y;
    }
    if (light_diffuse.z > 1.0){
        light_diffuse = light_diffuse / light_diffuse.z;
    }

    if (currentObject == 0 || currentObject == 2){
        diffuse = light_diffuse * mtl_diffuse * sDotN;
    }else{
        if (vertex_original.y >256){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex2, texcoord));
        }else if (vertex_original.y < 150){
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex0, texcoord));
        }else{
            diffuse = light_diffuse * mtl_diffuse * sDotN * vec3(texture(tex1, texcoord));
        }
    }

    // The specular component
    vec3 h = normalize(s + v);
    vec3 spec = vec3(0.0);
    if ( sDotN > 0.0 ){
		spec = light_specular * mtl_specular * pow( max( dot(norm,h), 0.0 ), 2*mtl_shininess );
    }
    
    if (gl_FrontFacing){
        return diffuse;
    }else{
        return vec3(0.0, 0.0, 0.0);
    }
}

void main(void) 
{
    //sun directional light
    fragColour = vec4(blinnphongPointLight(vertex, normalize(normal)), 1.0);

    //plane point light
    fragColour += vec4(blinnphongPointLight2(vertex, normalize(normal)), 1.0);

    //runway spot lights
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(-7.0, 107.13, 230.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(0.0, 107.13, 190.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(0.0, 107.13, 150.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(-3.0, 107.13, 110.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(24.0, 107.13, 110.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(22.0, 107.13, 150.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(25.0, 107.13, 190.0, 1.0)), 1.0);
    fragColour += vec4(blinnphongPointLight3(vertex, normalize(normal), vec4(25.0, 107.13, 230.0, 1.0)), 1.0);

    //capping light to 1.0 in all colours
    if (fragColour.x > 1.0){
        fragColour = fragColour / fragColour.x;
    }
    if (fragColour.y > 1.0){
        fragColour = fragColour / fragColour.y;
    }
    if (fragColour.z > 1.0){
        fragColour = fragColour / fragColour.z;
    }
}
