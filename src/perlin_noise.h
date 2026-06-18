#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <vector>

class PerlinNoise {
private:
    int p[512];
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y);

public:
    PerlinNoise(unsigned int seed);
    void reseed(unsigned int seed);
    double noise(double x, double y);
    double octave_noise(double x, double y, int octaves, double persistence, double lacunarity);
};

#endif // PERLIN_NOISE_H
