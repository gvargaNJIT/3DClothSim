#include "clothsim.h"
#include <glm/gtx/string_cast.hpp>

const glm::vec3 Cloth::gravity = glm::vec3(0.0f, -9.81f, 0.0f);

Cloth::Cloth(float stiff, float damp)
    : stiffness(stiff), damping(damp){}

void Cloth::springforces(std::vector<Particle>& particles, const std::vector<Spring>& springs, float stiffness, float damping) {
    for (const Spring& s : springs) {
        Particle& p1 = particles[s.p1];
        Particle& p2 = particles[s.p2];
        glm::vec3 delta = p2.position - p1.position;
        float currentLength = glm::length(delta);
    
        // Avoid division by zero
        if (currentLength < 1e-6f) continue; 
    
        glm::vec3 direction = delta / currentLength;
    
        // Hooke's Law: F = -k * (x - x0)
        float displacement = currentLength - s.restLength;
        glm::vec3 force = -stiffness * displacement * direction;
    
        // Apply spring forces
        p1.force += force;
        p2.force -= force;
    
        // Compute damping forces (relative velocity along spring direction)
        glm::vec3 velocity1 = p1.position - p1.previousPosition;
        glm::vec3 velocity2 = p2.position - p2.previousPosition;
        glm::vec3 relativeVelocity = velocity2 - velocity1;
        glm::vec3 dampingForce = -damping * glm::dot(relativeVelocity, direction) * direction;
    
        // Apply damping forces
        p1.force += dampingForce;
        p2.force -= dampingForce;
    }
}

void Cloth::updateparticles(std::vector<Particle>& particles, float deltaTime) {
    for (size_t i = 0; i < particles.size(); i++) {
        glm::vec3& currentPos = particles[i].position;
        glm::vec3& prevPos = particles[i].previousPosition;

        glm::vec3 acceleration = particles[i].force;
        glm::vec3 newPosition = 2.0f * currentPos - prevPos + acceleration * (deltaTime * deltaTime);
        
        prevPos = currentPos;
        currentPos = newPosition;

        particles[i].force = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void Cloth::applygravity(std::vector<Particle>& particles, float deltaTime) {
    std::cout << "Applying gravity..." << std::endl;
    for (size_t i = 0; i < particles.size(); i++) {
        std::cout << "Before Gravity - Particle " << i << " Force: " << glm::to_string(particles[i].force) << std::endl;
        particles[i].force += gravity;
        std::cout << "After Gravity - Particle " << i << " Force: " << glm::to_string(particles[i].force) << std::endl;
    }
    springforces(particles, springs, stiffness, damping);
    updateparticles(particles, deltaTime);
}


/*void Cloth::applymouseconstraint(glm::vec2 mousePos, bool mousePressed) {
    if (!mousePressed) return;

    for (int i = 0; i < width; i++) {
        Particle& p = particles[i]; 
        p.position = glm::vec3(mousePos.x, mousePos.y, 0.0f);
        p.previousPosition = p.position;
    }
}
    */