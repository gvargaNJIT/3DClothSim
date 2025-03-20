#include "clothgrid.h"

ParticleGrid::ParticleGrid(int w, int h, float space) 
    : width(w), height(h), spacing(space) {
    createGrid();
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
