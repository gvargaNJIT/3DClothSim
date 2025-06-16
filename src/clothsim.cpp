#include "clothsim.h"
#include <glm/gtx/string_cast.hpp>

const glm::vec3 Cloth::gravity = glm::vec3(0.0f, -1.0f, 0.0f);
extern bool gravityEnabled;

Cloth::Cloth(int width, int height, float spacing, float stiff, float damp)
    : width(width), height(height), spacing(spacing), stiffness(stiff), damping(damp) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            glm::vec3 pos(x * spacing, y * spacing, 0.0f);
            glm::vec3 prevPos = pos;
            particles.push_back(Particle(pos, prevPos, 1.0f));
        }
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;

            if (x < width - 1) 
                springs.push_back(Spring(index, index + 1, spacing, stiffness));

            if (y < height - 1) 
                springs.push_back(Spring(index, index + width, spacing, stiffness));

            if (x < width - 1 && y < height - 1) 
                springs.push_back(Spring(index, index + width + 1, spacing * 1.41f, stiffness));

            if (x > 0 && y < height - 1) 
                springs.push_back(Spring(index, index + width - 1, spacing * 1.41f, stiffness));

            if (x < width - 2) 
                springs.push_back(Spring(index, index + 2, spacing * 2.0f, stiffness * 0.5f));

            if (y < height - 2) 
                springs.push_back(Spring(index, index + width * 2, spacing * 2.0f, stiffness * 0.5f));
        }
    }
    
    particles[0].mass = 0.0f;
    particles[width - 1].mass = 0.0f;
}

void Cloth::springforces(std::vector<Particle>& particles, const std::vector<Spring>& springs, float stiffness, float damping) {
    for (const Spring& s : springs) {
        Particle& p1 = particles[s.p1];
        Particle& p2 = particles[s.p2];
        
        if (p1.mass == 0.0f && p2.mass == 0.0f) continue;
        
        glm::vec3 delta = p2.position - p1.position;
        float currentLength = glm::length(delta);
    
        if (currentLength < 1e-6f) continue; 
    
        glm::vec3 direction = delta / currentLength;
    
        float displacement = currentLength - s.restLength;
        glm::vec3 force = stiffness * displacement * direction;
    
        if (p1.mass > 0.0f) p1.force += force;
        if (p2.mass > 0.0f) p2.force -= force;
    
        glm::vec3 velocity1 = (p1.position - p1.previousPosition);
        glm::vec3 velocity2 = (p2.position - p2.previousPosition);
        glm::vec3 relativeVelocity = velocity2 - velocity1;
        
        float velocityAlongSpring = glm::dot(relativeVelocity, direction);
        glm::vec3 dampingForce = damping * velocityAlongSpring * direction;
   
        if (p1.mass > 0.0f) p1.force += dampingForce;
        if (p2.mass > 0.0f) p2.force -= dampingForce;
    }
}

void Cloth::updateparticles(std::vector<Particle>& particles, float deltaTime) {
    const float timeStep = 0.016f;
    
    for (Particle& p : particles) {
        if (p.mass == 0.0f) continue;
        

        glm::vec3 acceleration = p.force / p.mass;
        
        glm::vec3 temp = p.position;
        p.position = p.position * 2.0f - p.previousPosition + acceleration * timeStep * timeStep;
        p.previousPosition = temp;
        
        p.force = glm::vec3(0.0f);
        
        if (p.position.y < 0.0f) {
            p.position.y = 0.0f;
            glm::vec3 velocity = p.position - p.previousPosition;
            p.previousPosition = p.position - velocity * 0.1f;
        }
    }
}

void Cloth::applygravity(std::vector<Particle>& particles, float deltaTime) {
    for (Particle& p : particles) {
        if (p.mass > 0.0f) {
            p.force += gravity * p.mass;
        }
    }
}

void Cloth::update(float deltaTime) {
    if (gravityEnabled) {
        applygravity(particles, deltaTime);
    }

    springforces(particles, springs, stiffness, damping);
    
    updateparticles(particles, deltaTime);
}

void Cloth::applymouseconstraint(glm::vec2 mousePos, bool mousePressed) {
    if (!mousePressed) return;

    float simWidth = width * spacing;
    float simHeight = height * spacing;
    
    float normalizedX = (mousePos.x / 800.0f) * simWidth;
    float normalizedY = ((600.0f - mousePos.y) / 600.0f) * simHeight;
    glm::vec3 mousePoint(normalizedX, normalizedY, 0.0f);
    
    float minDist = 1000.0f;
    int closestIndex = -1;
    
    for (size_t i = 0; i < particles.size(); i++) {
        float dist = glm::distance(particles[i].position, mousePoint);
        if (dist < minDist) {
            minDist = dist;
            closestIndex = i;
        }
    }
    
    if (closestIndex >= 0 && minDist < spacing * 2.0f) {
        particles[closestIndex].position = mousePoint;
        particles[closestIndex].previousPosition = mousePoint;
    }
}

const std::vector<Particle>& Cloth::getParticles() const {
    return particles;
}

const std::vector<Spring>& Cloth::getSprings() const {
    return springs;
}