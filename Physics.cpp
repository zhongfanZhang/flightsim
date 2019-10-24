#include <iostream>
#include <stdio.h>
#include <math.h>

#include "Physics.h"

#define DEG2RAD(x) ((x)*M_PI/180.0) 
#define RAD2DEG(x) ((x)*180.0/M_PI)

Physics::Physics()
{
    //position of the start of the runway
    position = glm::vec3(14.0, 96.0, 250.0);
    yaw = M_PI;
    pitch = 0.0;
    roll = 0.0;
    speed = 0.0;
    power = 0.0;
}

//speed is a scaled version of the plane's power output
void Physics::updateSpeed(float deltaTime)
{
    speed = 50 * power * deltaTime;
}

//position is updated using yaw and pitch only
//we did not succeed in implementing roll
void Physics::updatePosition(float deltaTime)
{
    position.x += speed * glm::sin(-yaw) * glm::cos(pitch);
    //gravity is included which drops the plane's altitude based on the current speed
    position.y += speed * glm::sin(pitch) - (1/(10*speed + 1));
    position.z += speed * glm::cos(-yaw) * glm::cos(pitch);   
}

void Physics::setCollision()
{
    power = 0;
    position = glm::vec3(14.0, 96.0, 220.0);
    yaw = M_PI;
    pitch = 0.0;
    roll = 0.0;
}

void Physics::setLanding()
{
    pitch = 0.0;
    position.y = 96.0;
}

void Physics::setCeiling()
{
    pitch = 0.0;
    position.y = 400.0;
}

float Physics::getYaw()
{
    return yaw;
}

float Physics::getPitch()
{
    return pitch;
}

float Physics::getRoll()
{
    return roll;
}

glm::vec3 Physics::getPosition()
{
    return position;
}

float Physics::getPower()
{
    return power;
}

float Physics::getSpeed()
{
    return speed;
}

void Physics::updateYaw(float yawInput, InputState &Input)
{
    if (Input.qPressed){
        yaw -= 2 * yawInput;
    }
    if (Input.ePressed){
        yaw += 2 * yawInput;
    }
}

void Physics::updatePitch(float pitchInput, InputState &Input)
{
    if (Input.wPressed){
        pitch -= 2 * pitchInput;
    }
    if (Input.sPressed){
        pitch += 2 * pitchInput;
    }
}

void Physics::updateRoll(float rollInput, InputState &Input)
{
    if (Input.aPressed){
        roll -= 2 * rollInput;
    }
    if (Input.dPressed){
        roll += 2 * rollInput;
    }
}

void Physics::updatePower(float powerInput, InputState &Input)
{
    if (Input.spacePressed){
        power += 0.5 * powerInput;
    }
    else{
        power -= 0.1 * powerInput;
    }
    if (Input.altPressed){
        power -= 0.5 * powerInput;
    }

    if (power > 1.0){
        power = 1.0;
    }
    if (power < 0.0){
        power = 0.0;
    }
}
