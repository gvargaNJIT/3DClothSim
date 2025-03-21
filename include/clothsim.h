#ifndef CLOTHSIM_H
#define CLOTHSIM_H

#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "clothgrid.h"

class Cloth {
private:
    static const glm::vec3 gravity;
    std::vector<Particle> particles;
    std::vector<Spring> springs;

public:
    void springforces(std::vector<Particle>& particles, const std::vector<Spring>& springs, float stiffness, float damping);
    void updateparticles(std::vector<Particle>& particles, float deltaTime);
    void applygravity(std::vector<Particle>& particles, float deltaTime);
    void applymouseconstraint(glm::vec2 mousePos, bool mousePressed);
    void update(float deltaTime);  // New comprehensive update method
    
    // Getter methods for OpenGL renderer
    const std::vector<Particle>& getParticles() const;
    const std::vector<Spring>& getSprings() const;
    
    Cloth(int width, int height, float spacing, float stiff, float damp);
    float stiffness;
    float damping;
    int width;
    int height;
    float spacing;
};

#endif
