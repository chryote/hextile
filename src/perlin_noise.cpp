#include "perlin_noise.h"
#include <cmath>
#include <random>
#include <numeric>
#include <algorithm>

double PerlinNoise::fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y) {
    int h = hash & 7;
    switch (h) {
        case 0: return x;
        case 1: return -x;
        case 2: return y;
        case 3: return -y;
        case 4: return x + y;
        case 5: return -x + y;
        case 6: return x - y;
        case 7: return -x - y;
    }
    return 0;
}

PerlinNoise::PerlinNoise(unsigned int seed) {
    reseed(seed);
}

void PerlinNoise::reseed(unsigned int seed) {
    std::mt19937 engine(seed);
    std::vector<int> permutation(256);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::shuffle(permutation.begin(), permutation.end(), engine);
    for (int i = 0; i < 256; i++) {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }
}

double PerlinNoise::noise(double x, double y) {
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    double u = fade(x);
    double v = fade(y);
    int A = p[X] + Y;
    int B = p[X + 1] + Y;
    return lerp(v, lerp(u, grad(p[A], x, y),
                           grad(p[B], x - 1, y)),
                   lerp(u, grad(p[A + 1], x, y - 1),
                           grad(p[B + 1], x - 1, y - 1)));
}

double PerlinNoise::octave_noise(double x, double y, int octaves, double persistence, double lacunarity) {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return total / maxValue;
}
