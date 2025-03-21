#ifndef CLOTHGRID_H
#define CLOTHGRID_H

#include <vector>
#include <iostream>
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 force;
    glm::vec3 position;
    glm::vec3 previousPosition;
    float mass;
    Particle(const glm::vec3& pos, const glm::vec3& prevPos, float m);
    Particle() 
        : position(glm::vec3(0.0f)), previousPosition(glm::vec3(0.0f)), mass(1.0f), force(glm::vec3(0.0f)) {};
};

struct Spring {
    int p1, p2;
    float restLength;
    float stiffness;
    Spring(int a, int b, float rest, float k);
};

class ParticleGrid {
    private:
        std::vector<Particle> particles;
        std::vector<Spring> springs;
    
    public:
        int width, height;
        ParticleGrid(int w, int h, float space);
        void createGrid();
        void addsprings();
        void printGrid();
        float spacing = 0.1f;
        float stiffness = 0.5f;
};

#endif
