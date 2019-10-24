#ifndef PHYSICS_H
#define PHYSICS_H

#include "InputState.h"
#include "glm/glm.hpp"

class Physics
{
    private:

        //plane position
        glm::vec3 position;

        //radians
        //negative = left, positive = right
        float yaw;
    
        //radians
        //negative = down, positive = up
        float pitch;

        //radians
        //negative = left, positive = right
        float roll;

        float speed;

        //from 0 (no power) to 1 (max power)
        float power;

    public:

        Physics();

        //based on power, yaw, pitch, roll
        void updateSpeed(float deltaTime);

        //based on speed, yaw, pitch, roll
        void updatePosition(float deltaTime);

        //updated from main.cpp using time between frames
        //to maintain constant speed
        void updateYaw(float yawInput, InputState &Input);
        void updatePitch(float pitchInput, InputState &Input);
        void updateRoll(float rollInput, InputState &Input);
        void updatePower(float powerInput, InputState &Input);
        
        //collision detection functions
        void setCollision();
        void setLanding();
        void setCeiling();

        //sent to main.cpp
        float getYaw();
        float getPitch();
        float getRoll();
        glm::vec3 getPosition();
        float getPower();
        float getSpeed();
};

#endif
