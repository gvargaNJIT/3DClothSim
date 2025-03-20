#include "clothsim.h"

void Cloth::applySpringForces(std::vector<Particle>& particles, const std::vector<Spring>& springs, float stiffness, float damping) {
    for (const Spring& s : springs) {
        Particle& p1 = particles[s.p1];
        Particle& p2 = particles[s.p2];
        glm::vec3 delta = p2.position - p1.position;
        float currentLength = glm::length(delta);

        if (currentLength == 0.0f) continue;

        glm::vec3 direction = delta / currentLength;

        float displacement = currentLength - s.restLength;
        glm::vec3 force = -stiffness * displacement * direction;
        p1.force += force;
        p2.force -= force;
        glm::vec3 velocity1 = (p1.position - p1.previousPosition);
        glm::vec3 velocity2 = (p2.position - p2.previousPosition);
        glm::vec3 relativeVelocity = velocity2 - velocity1;
        glm::vec3 dampingForce = -damping * relativeVelocity;
        p1.force += dampingForce;
        p2.force -= dampingForce;
    }
}

void Cloth::updateParticles(std::vector<Particle>& particles, float deltaTime) {
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
    for (size_t i = 0; i < particles.size(); i++) {
        particles[i].force += gravity;
    }
    applySpringForces(particles, deltaTime);
    updateParticles(particles, deltaTime);
}

void Cloth::applymouseconstraint(glm::vec2 mousePos, bool mousePressed) {
    if (!mousePressed) return;

    for (int i = 0; i < width; i++) {
        Particle& p = particles[i]; 
        p.position = glm::vec3(mousePos.x, mousePos.y, 0.0f);
        p.previousPosition = p.position;
    }
}