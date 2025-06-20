#include "clothsim.h"
#include <glm/gtx/string_cast.hpp>

const glm::vec3 Cloth::gravity = glm::vec3(0.0f, -3.0f, 0.0f);
const glm::vec3 Cloth::wind = glm::vec3(3.0f, 0.0f, 0.0f);
bool gravityEnabled = false;

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

void Cloth::handleSelfCollision() {
    float minDistance = spacing * 0.6f;
    
    for (size_t i = 0; i < particles.size(); ++i) {
        if (particles[i].mass == 0.0f) continue;
        
        for (size_t j = i + 1; j < particles.size(); ++j) {
            if (particles[j].mass == 0.0f) continue;
            
            if (areNeighbors(i, j)) continue;
            
            glm::vec3 diff = particles[i].position - particles[j].position;
            float distance = glm::length(diff);
            
            if (distance < minDistance && distance > 0.001f) {
                glm::vec3 normal = glm::normalize(diff);
                float overlap = minDistance - distance;
                
                float totalMass = particles[i].mass + particles[j].mass;
                float ratio1 = particles[j].mass / totalMass;
                float ratio2 = particles[i].mass / totalMass;

                particles[i].position += normal * (overlap * ratio1);
                particles[j].position -= normal * (overlap * ratio2);
                
                glm::vec3 vel1 = particles[i].position - particles[i].previousPosition;
                glm::vec3 vel2 = particles[j].position - particles[j].previousPosition;
                
                particles[i].previousPosition = particles[i].position - vel1;
                particles[j].previousPosition = particles[j].position - vel2;
            }
        }
    }
}

bool Cloth::areNeighbors(int i, int j) const {
    int row1 = i / width, col1 = i % width;
    int row2 = j / width, col2 = j % width;
    
    int rowDiff = abs(row1 - row2);
    int colDiff = abs(col1 - col2);
    
    return (rowDiff <= 1 && colDiff <= 1) ||
           (rowDiff <= 2 && colDiff == 0) ||
           (rowDiff == 0 && colDiff <= 2);
}

void Cloth::update(float deltaTime) {
    if (gravityEnabled) {
        applygravity(particles, deltaTime);
    }

    const int solverIterations = 8;
    for (int i = 0; i < solverIterations; ++i) {
        springforces(particles, springs, stiffness, damping);
        
        if (i % 2 == 0) {
            handleSelfCollision();
        }
    }
    
    updateparticles(particles, deltaTime);
}

void Cloth::applymouseconstraint(glm::vec2 mousePos, bool mousePressed) {
    if (!mousePressed) return;

    float simWidth = width * spacing;
    float simHeight = height * spacing;

    float normalizedX = (mousePos.x / 800.0f) * simWidth;
    float normalizedY = ((600.0f - mousePos.y) / 600.0f) * simHeight;
    glm::vec3 mousePoint(normalizedX, normalizedY, 0.0f);

    float radius = spacing * 4.0f;
    float freezeRadius = spacing * 3.5f;
    
    int closestParticle = -1;
    float minDistance = radius;
    
    for (size_t i = 0; i < particles.size(); i++) {
        Particle& p = particles[i];
        if (p.mass > 0.0f) {
            float distance = glm::distance(p.position, mousePoint);
            if (distance < minDistance) {
                minDistance = distance;
                closestParticle = i;
            }
        }
    }
    
    if (closestParticle != -1) {
        Particle& anchor = particles[closestParticle];
        glm::vec3 anchorOriginalPos = anchor.position;
        
        glm::vec3 targetMovement = mousePoint - anchorOriginalPos;
        glm::vec3 smoothedMovement = targetMovement * 0.8f;
        
        anchor.previousPosition = anchor.position;
        anchor.position = anchorOriginalPos + smoothedMovement;
        
        glm::vec3 movement = smoothedMovement;
        
        for (size_t i = 0; i < particles.size(); i++) {
            if (i == closestParticle) continue;
            
            Particle& p = particles[i];
            if (p.mass > 0.0f) {
                float distance = glm::distance(p.position, anchorOriginalPos);
                if (distance < freezeRadius) {
                    p.previousPosition = p.position;
                    p.position += movement;
                }
            }
        }
    }
}

const std::vector<Particle>& Cloth::getParticles() const {
    return particles;
}

const std::vector<Spring>& Cloth::getSprings() const {
    return springs;
}

void Cloth::applywind(float deltaTime) {
    for (Particle& p : particles) {
        if (p.mass > 0.0f) {
            float randomness = 0.5f + static_cast<float>(rand()) / RAND_MAX;
            glm::vec3 localWind = wind * randomness;
            p.force += localWind * p.mass;
        }
    }
}

void Cloth::reset() {
    particles.clear();
    springs.clear();

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
}

