#ifndef CLOTHSIM_H
#define CLOTHSIM_H

#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "clothgrid.h"

class Cloth {
    private:
    inline const glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    public:
    void springforces();
    void updateparticles();
    void applygravity();
    void applymouseconstraint();
}