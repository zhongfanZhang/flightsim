#ifndef _VIEWER_H_
#define _VIEWER_H_

#include "InputState.h"
#include "glm/glm.hpp"

class Viewer 
{
private:
    glm::mat4 viewMtx;
    float mouseX;
    float mouseY;

public:
    Viewer( glm::vec3 eye );

    const glm::mat4 getViewMtx() const;

    float update( InputState &Input, glm::vec3 at, float yaw, float pitch, float roll );
};

#endif
