#pragma once

#include <cstdint>
#include <numbers>
#include <random>

class Random {
public:
    explicit Random(std::uint32_t seed) : engine_(seed) {}

    float uniform(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(engine_); }

    // Inclusive on both ends.
    int range(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(engine_); }

    bool chance(float probability) { return uniform(0.f, 1.f) < probability; }

    float angle() { return uniform(0.f, 2.f * std::numbers::pi_v<float>); }

private:
    std::mt19937 engine_;
};
