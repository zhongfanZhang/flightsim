#include <iostream>
#include <stdio.h>
#include <math.h>

#include "Viewer.h"
#include "glm/gtc/matrix_transform.hpp"

#define DEG2RAD(x) ((x)*M_PI/180.0) 
#define RAD2DEG(x) ((x)*180.0/M_PI) 

Viewer::Viewer( glm::vec3 eye )
{
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 at = glm::vec3(0.0, 0.0, 0.0);
    viewMtx = glm::lookAt(eye, at, up);
    mouseX = 0.0;
    mouseY = 0.0;
}

const glm::mat4 Viewer::getViewMtx() const
{
    return viewMtx;
}

//camera is updated using the current plane position and direction
float Viewer::update( InputState &Input, glm::vec3 at, float yaw, float pitch, float roll )
{
    float diffX, diffY;
    Input.readDeltaAndReset(&diffY, &diffX);

    //camera is set to move with mouse input when left mouse is pressed
    if (Input.lMousePressed){
        mouseX += diffX;
        mouseY += diffY;
    //camera is set to follow behind plane at 10 degrees above it when left mouse is not pressed
    }else{
        mouseX = 10 + RAD2DEG(-pitch);
        mouseY = RAD2DEG(-yaw);
    }

    //camera is capped to +-90 degrees for pitch
    if (mouseX > 89.0f){
        mouseX = 89.0f;
    }
    if (mouseX < -89.0f){
        mouseX = -89.0f;
    }
    
    float sinY, cosY, sinX, cosX, sinZ, cosZ, eyeX, eyeY, eyeZ;

    sinY = -glm::cos(DEG2RAD(mouseY));
    cosY = -glm::sin(DEG2RAD(mouseY));
    sinX = glm::sin(DEG2RAD(mouseX));
    cosX = glm::cos(DEG2RAD(mouseX));

    //camera position is updated using pitch and yaw
    eyeX = at[0] + 10.0*cosX*cosY;
    eyeY = at[1] + 10.0*sinX;
    eyeZ = at[2] + 10.0*cosX*sinY;

    glm::vec3 eye(eyeX, eyeY, eyeZ);

    //camera up vector is updated using roll to determine the amount of tilt
    viewMtx = glm::lookAt(eye, at, glm::vec3(glm::tan(roll)*sinY*cosX, 1.0, -glm::tan(roll)*cosY*cosX));

}
