#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include "clothsim.h"

bool gravityEnabled = false;

class ClothTest : public ::testing::Test {
protected:
    Cloth cloth; 
    ClothTest() : cloth(10, 10, 1.0f, 100.0f, 0.1f) {};
    std::vector<Particle> particles;
    std::vector<Spring> springs;

    void SetUp() override {
        particles.resize(2);
        particles[0].position = glm::vec3(0.0f, 0.0f, 0.0f);
        particles[1].position = glm::vec3(1.2f, 0.0f, 0.0f); 
        
        particles[0].mass = 1.0f;
        particles[1].mass = 1.0f;
    
        particles[0].force = glm::vec3(0.0f);
        particles[1].force = glm::vec3(0.0f);
        
        springs.push_back(Spring(0, 1, 1.0f, cloth.stiffness));
        
        std::cout << "Setup Complete - Particles & Springs Initialized\n";
    }
};     

TEST_F(ClothTest, ApplySpringForcesTest) {
    std::cout << "Initial Particle 0 Force: " << glm::to_string(particles[0].force) << std::endl;
    std::cout << "Initial Particle 1 Force: " << glm::to_string(particles[1].force) << std::endl;

    std::cout << "Stiffness: " << cloth.stiffness << ", Damping: " << cloth.damping << std::endl;

    cloth.springforces(particles, springs, cloth.stiffness, cloth.damping);

    std::cout << "After Spring Forces - Particle 0 Force: " << glm::to_string(particles[0].force) << std::endl;
    std::cout << "After Spring Forces - Particle 1 Force: " << glm::to_string(particles[1].force) << std::endl;

    EXPECT_GT(glm::length(particles[0].force), 0.0f);
    EXPECT_GT(glm::length(particles[1].force), 0.0f);
}

TEST_F(ClothTest, UpdateParticlesTest) {
    glm::vec3 initialPos = particles[0].position;
    particles[0].force = glm::vec3(1.0f, 0.0f, 0.0f);
    cloth.updateparticles(particles, 0.1f);
    EXPECT_NE(particles[0].position, initialPos);
}

TEST_F(ClothTest, ApplyGravityTest) {

    glm::vec3 initialForce = particles[0].force;

    float deltaTime = 1.0f;
    cloth.applygravity(particles, deltaTime);

    EXPECT_NE(particles[0].force, initialForce);
}