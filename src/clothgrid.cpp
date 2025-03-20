#include "clothgrid.h"

ParticleGrid::ParticleGrid(int w, int h, float space) 
    : width(w), height(h), spacing(space) {
    createGrid();
    addsprings();
}

void ParticleGrid::createGrid() {
    particles.clear();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Particle p;
            p.x = x * spacing;
            p.y = y * spacing;
            p.vx = 0.0f;
            p.vy = 0.0f;
            p.mass = 1.0f;
            
            particles.push_back(p);
        }
    }
}

void ParticleGrid::printGrid() {
    for (const auto& p : particles) {
        std::cout << "Particle at (" << p.x << ", " << p.y << ")" << std::endl;
    }
}

void ParticleGrid::addsprings() {

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
