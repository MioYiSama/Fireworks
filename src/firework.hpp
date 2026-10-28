#pragma once

#include "canvas.hpp"
#include "random.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Fixed-pixel flowers unfold in rings; their sparse outer streaks disappear before the core.
class FireworkShow {
public:
    void update(float dt, Canvas& sky, Random& rng);
    void draw(Canvas& frame) const;

    struct Sprite {
        int width = 0;
        int height = 0;
        std::vector<Rgb> pixels;
    };

    struct FlowerPixel {
        float appearsAt; // seconds after the burst
        float disappearsAt;
        std::int16_t dx;
        std::int16_t dy;
        Rgb color;
    };

private:
    enum class Phase { Idle, Rising, Bursting };

    struct GlowPixel {
        std::uint16_t r = 0;
        std::uint16_t g = 0;
        std::uint16_t b = 0;
    };

    struct Slot {
        Phase phase = Phase::Idle;
        int rocketColor = 0;
        float rocketX = 0.f; // head centre, px
        float rocketY = 0.f;
        float speed = 0.f;   // px/s before the final slow climb
        float peakY = 0.f;
        float climb = 0.f;
        int radius = 0;
        int palette = 0;
        float elapsed = 0.f;
        int centerX = 0;
        int centerY = 0;
        std::vector<FlowerPixel> flower; // sorted by appearance time
        std::vector<std::uint32_t> deathOrder;
        std::vector<GlowPixel> raster; // 3x3 filtered live pixels, accumulated only on changes
        std::size_t born = 0;
        std::size_t dead = 0;
    };

    static void launch(Slot& slot, const Canvas& sky, Random& rng, int cue);
    static void rise(Slot& slot, float dt, Canvas& sky, Random& rng);
    static void bloom(Slot& slot, float dt);
    static void changePixel(Slot& slot, const FlowerPixel& pixel, int sign);
    static void drawBurst(const Slot& slot, Canvas& frame);

    std::array<Slot, 40> slots_;
    std::size_t cue_ = 0;
    float elapsed_ = 0.f;
    float launchTimer_ = 10.1f;
    float erodeRemainder_ = 0.f;
};
