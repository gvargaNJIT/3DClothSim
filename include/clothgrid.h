#ifndef CLOTHGRID_H
#define CLOTHGRID_H

#include <vector>
#include <iostream>

struct Particle {
    float x, y;
    float vx, vy;
    float mass;
};

class ParticleGrid {
    private:
        int width, height;
        float spacing;
        std::vector<Particle> particles;
    
    public:
        ParticleGrid(int w, int h, float space);
        void createGrid();
        void printGrid();
};

#endif
