#include "clothgrid.h"

Particle::Particle(const glm::vec3& pos, const glm::vec3& prevPos, float m)
        : position(pos), previousPosition(prevPos), mass(m), force(glm::vec3(0.0f, 0.0f, 0.0f)) {}

ParticleGrid::ParticleGrid(int w, int h, float space)
        : width(w), height(h), spacing(space), stiffness(0.5f) {}

void ParticleGrid::createGrid() {
    particles.clear();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            glm::vec3 pos(x * spacing, y * spacing, 0.0f);
            glm::vec3 prevPos = pos;

            Particle p(pos, prevPos, 1.0f);
            particles.push_back(p);
        }
    }
}

void ParticleGrid::addsprings() {
    springs.clear();

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
                springs.push_back(Spring(index, index + 2, spacing * 2, stiffness));

            if (y < height - 2) 
                springs.push_back(Spring(index, index + width * 2, spacing * 2, stiffness));
        }
    }
}

Spring::Spring(int a, int b, float rest, float k) 
        : p1(a), p2(b), restLength(rest), stiffness(k) {}

void ParticleGrid::printGrid() {
    std::cout << "Grid dimensions: " << width << "x" << height << std::endl;
    std::cout << "Number of particles: " << particles.size() << std::endl;
    std::cout << "Number of springs: " << springs.size() << std::endl;
}