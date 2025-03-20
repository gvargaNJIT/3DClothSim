#include "clothsim.h"

void Cloth::applySpringForces(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& velocities, const std::vector<Spring>& springs, float stiffness, float damping) {
    for (const Spring& s : springs) {
        glm::vec3& p1 = positions[s.p1];
        glm::vec3& p2 = positions[s.p2];

        glm::vec3 delta = p2 - p1;
        float currentLength = glm::length(delta);

        if (currentLength == 0.0f) continue;

        glm::vec3 direction = delta / currentLength;

        float displacement = currentLength - s.restLength;
        glm::vec3 force = -stiffness * displacement * direction;

        velocities[s.p1] += force;
        velocities[s.p2] -= force;

        glm::vec3 relativeVelocity = velocities[s.p2] - velocities[s.p1];
        glm::vec3 dampingForce = -damping * relativeVelocity;

        velocities[s.p1] += dampingForce;
        velocities[s.p2] -= dampingForce;
    }
}

void Cloth::updateParticles(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& velocities, float deltaTime) {
    for (size_t i = 0; i < positions.size(); i++) {
        velocities[i] *= 0.99f;
        positions[i] += velocities[i] * deltaTime;
    }
}

void Cloth::applygravity(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& velocities, const std::vector<Spring>& springs, float stiffness, float damping, float deltaTime) {
    for (size_t i = 0; i < positions.size(); i++) {
        velocities[i] += gravity * deltaTime;
    }
    applySpringForces(positions, velocities, springs, stiffness, damping);
}
