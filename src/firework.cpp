#include "firework.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr int kRocketWidth = 13; // px, including the faint coloured halo
constexpr int kRocketHeight = 38;
constexpr int kRocketHeadX = kRocketWidth / 2;
constexpr int kRocketHeadY = 10;
constexpr float kFlickerRate = 30.f; // sprite changes/s, independent of the render rate
constexpr float kSlowClimbFraction = 0.35f;
constexpr float kSlowSpeed = 0.53f;
constexpr float kPixelLossRate = 0.70f; // erased pixels/pixel/s; not a brightness fade
constexpr float kBurstLifetime = 8.f;  // s, including the very sparse stationary remnants
constexpr float kRevealSpeed = 230.f; // initial radial reveal, px/s
constexpr float kRevealDrag = 1.4f;    // 1/s; faint outer hairs unfold for several seconds

struct Palette {
    Rgb core;
    Rgb inner;
    Rgb outer;
    Rgb tip;
    float weight;
    bool star;
};

// Warm, rather muted hairlike flowers alternate with the brighter red/pink starbursts.
constexpr std::array kPalettes{
    Palette{{255, 235, 175}, {245, 204, 145}, {178, 115, 103}, {244, 185, 151}, 4.f, false},
    Palette{{255, 255, 244}, {255, 157, 145}, {242, 81, 94}, {255, 235, 220}, 2.f, true},
    Palette{{255, 255, 242}, {255, 217, 202}, {215, 153, 153}, {255, 222, 214}, 3.f, false},
    Palette{{255, 250, 215}, {255, 200, 173}, {211, 134, 133}, {255, 203, 181}, 3.f, false},
    Palette{{255, 245, 225}, {238, 215, 180}, {174, 146, 130}, {255, 225, 208}, 1.5f, false},
    Palette{{255, 246, 255}, {225, 181, 211}, {162, 126, 161}, {255, 218, 236}, 0.15f, false},
    Palette{{255, 255, 223}, {217, 229, 167}, {137, 161, 108}, {242, 253, 198}, 0.15f, false},
    Palette{{255, 255, 244}, {255, 164, 152}, {252, 95, 100}, {255, 240, 230}, 0.75f, true},
    Palette{{255, 255, 245}, {255, 225, 130}, {230, 155, 155}, {255, 245, 235}, 1.5f, false},
    Palette{{255, 255, 245}, {255, 217, 205}, {238, 145, 150}, {255, 247, 239}, 1.f, true},
};

constexpr std::array kRocketColors{
    Rgb{122, 192, 255}, Rgb{255, 235, 54}, Rgb{141, 248, 112},
    Rgb{255, 166, 66}, Rgb{233, 237, 255}, Rgb{136, 232, 255},
};

struct LaunchCue {
    float at; // s from the start of the recording
    float x;  // head coordinates in the 1200x800 client area, px
    float startY;
    float peakY;
    int radius;
    int rocketColor;
    int palette;
};

// Measured head positions and peaks, with fitted emission times and final image radii.
constexpr std::array kLaunchCues{
    LaunchCue{0.f, 311.f, 670.f, 19.f, 125, 0, 3},
    LaunchCue{0.f, 902.f, 600.f, 113.f, 105, 0, 0},
    LaunchCue{0.f, 1073.f, 600.f, 337.f, 105, 1, 0},
    LaunchCue{0.30f, 233.f, 710.f, 75.f, 110, 0, 1},
    LaunchCue{0.52f, 1034.f, 680.f, 160.f, 105, 2, 7},
    LaunchCue{0.62f, 1044.f, 700.f, 19.f, 130, 1, 9},
    LaunchCue{0.70f, 1108.f, 700.f, 191.f, 110, 2, 3},
    LaunchCue{0.70f, 52.f, 650.f, 321.f, 135, 3, 2},
    LaunchCue{0.85f, 954.f, 720.f, 240.f, 115, 1, 8},
    LaunchCue{1.10f, 655.f, 650.f, 375.f, 110, 1, 4},
    LaunchCue{1.85f, 341.f, 760.f, 220.f, 120, 2, 3},
    LaunchCue{2.17f, 925.f, 700.f, 390.f, 125, 0, 1},
    LaunchCue{2.45f, 109.f, 730.f, 200.f, 115, 0, 0},
    LaunchCue{3.10f, 347.f, 730.f, 337.f, 120, 0, 0},
    LaunchCue{3.18f, 627.f, 730.f, 275.f, 110, 2, 1},
    LaunchCue{3.40f, 797.f, 700.f, 294.f, 115, 2, 9},
    LaunchCue{3.55f, 11.f, 700.f, 350.f, 115, 2, 0},
    LaunchCue{3.90f, 638.f, 690.f, 75.f, 110, 1, 8},
    LaunchCue{4.20f, 740.f, 720.f, 275.f, 130, 0, 2},
    LaunchCue{4.46f, 257.f, 720.f, 75.f, 130, 3, 2},
    LaunchCue{4.65f, 809.f, 680.f, 40.f, 115, 0, 9},
    LaunchCue{4.70f, 277.f, 690.f, 75.f, 130, 0, 3},
    LaunchCue{5.55f, 1194.f, 750.f, 200.f, 125, 2, 1},
    LaunchCue{6.00f, 1006.f, 740.f, 11.f, 130, 2, 9},
    LaunchCue{6.30f, 670.f, 740.f, 250.f, 115, 2, 4},
    LaunchCue{6.80f, 698.f, 700.f, 250.f, 130, 1, 0},
    LaunchCue{7.20f, 906.f, 682.f, 113.f, 120, 0, 9},
    LaunchCue{7.20f, 1094.f, 717.f, 300.f, 125, 1, 8},
    LaunchCue{7.55f, 449.f, 720.f, 200.f, 110, 0, 0},
    LaunchCue{8.05f, 509.f, 720.f, 375.f, 115, 0, 1},
    LaunchCue{8.20f, 1154.f, 717.f, 75.f, 120, 1, 9},
    LaunchCue{8.50f, 779.f, 760.f, 315.f, 125, 3, 2},
    LaunchCue{8.80f, 978.f, 740.f, 135.f, 110, 0, 1},
    LaunchCue{8.94f, 440.f, 720.f, 350.f, 110, 0, 3},
    LaunchCue{9.30f, 344.f, 730.f, 220.f, 125, 0, 0},
    LaunchCue{9.75f, 741.f, 730.f, 330.f, 115, 2, 0},
};

int pickPalette(Random& rng) {
    float total = 0.f;
    for (const Palette& palette : kPalettes)
        total += palette.weight;
    float roll = rng.uniform(0.f, total);
    for (std::size_t i = 0; i < kPalettes.size(); ++i) {
        if (roll < kPalettes[i].weight)
            return static_cast<int>(i);
        roll -= kPalettes[i].weight;
    }
    return 0;
}

std::array<FireworkShow::Sprite, 2> makeRocketFrames(Rgb color) {
    std::array<FireworkShow::Sprite, 2> frames;
    for (int f = 0; f < 2; ++f) {
        FireworkShow::Sprite& sprite = frames[f];
        sprite.width = kRocketWidth;
        sprite.height = kRocketHeight;
        sprite.pixels.resize(kRocketWidth * kRocketHeight);
        for (int y = 0; y < kRocketHeight; ++y) {
            for (int x = 0; x < kRocketWidth; ++x) {
                const float dx = static_cast<float>(x - kRocketHeadX);
                const float dy = static_cast<float>(y - kRocketHeadY);
                const float sx = dy < 0.f ? 3.6f : std::max(1.2f, 3.6f - dy * 0.085f);
                const float sy = dy < 0.f ? 6.f : 17.f;
                const float energy = dx * dx / (sx * sx) + dy * dy / (sy * sy);
                float intensity = std::exp(-energy);
                if (intensity < 0.055f)
                    continue;
                // A textured, tapering flame, not a solid line or a perfectly smooth ellipse.
                const int grain = (x * 19 + y * 13 + f * 23) % 31;
                if (dy > 5.f && grain < 6)
                    intensity *= 0.22f;
                if (f == 1)
                    intensity *= 0.83f;
                const float hot = std::clamp((intensity - 0.55f) * 1.8f, 0.f, 0.75f);
                const Rgb tinted = lerp(color, Rgb{255, 255, 239}, hot);
                sprite.pixels[y * kRocketWidth + x] = scale(tinted, std::min(1.f, intensity * 1.35f));
            }
        }
    }
    return frames;
}

const auto& rocketSprites() {
    static const auto sprites = [] {
        std::array<std::array<FireworkShow::Sprite, 2>, kRocketColors.size()> result;
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i] = makeRocketFrames(kRocketColors[i]);
        return result;
    }();
    return sprites;
}

std::vector<FireworkShow::FlowerPixel> makeFlower(Random& rng, int radius, int paletteIndex) {
    const Palette& palette = kPalettes[paletteIndex];
    const int size = 2 * radius + 1;
    const float r = static_cast<float>(radius);
    std::vector<Rgb> grid(static_cast<std::size_t>(size) * size);

    auto plot = [&](float fx, float fy, Rgb color) {
        const int dx = static_cast<int>(std::lround(fx));
        const int dy = static_cast<int>(std::lround(fy));
        if (dx * dx + dy * dy > radius * radius)
            return;
        Rgb& dst = grid[static_cast<std::size_t>(dy + radius) * size + dx + radius];
        dst = {std::max(dst.r, color.r), std::max(dst.g, color.g), std::max(dst.b, color.b)};
    };

    const int rays = static_cast<int>(r * (palette.star ? rng.uniform(0.55f, 0.75f) : rng.uniform(1.80f, 2.20f)));
    for (int i = 0; i < rays; ++i) {
        const float angle = (static_cast<float>(i) + rng.uniform(-0.47f, 0.47f)) *
                            2.f * std::numbers::pi_v<float> / static_cast<float>(rays);
        const float ca = std::cos(angle);
        const float sa = std::sin(angle);
        const float length = r * rng.uniform(0.60f, 1.f);
        const float brightness = rng.uniform(0.80f, 1.10f) * (paletteIndex == 0 ? 0.92f : 1.f);
        const float curl = r * rng.uniform(-0.20f, 0.20f);
        const float ripple = rng.uniform(0.7f, 2.f);
        const float ripplePhase = rng.angle();
        const int filaments = palette.star ? rng.range(2, 3) : (rng.chance(0.2f) ? 2 : 1);
        for (int filament = 0; filament < filaments; ++filament) {
            const float offset = (static_cast<float>(filament) - static_cast<float>(filaments - 1) * 0.5f) * 1.1f;
            for (float t = 1.f; t < length; t += 0.65f) {
                const float u = t / r;
                const float bend = curl * u * u + offset * u +
                                   ripple * std::sin(t * 0.23f + ripplePhase) * u * u;
                const float x = t * ca - bend * sa;
                const float y = t * sa + bend * ca + r * (0.18f + 0.50f * std::abs(ca)) * u * u;
                if (rng.chance(palette.star ? 0.07f : 0.15f))
                    continue;
                // Most outer hairs are darker and less dense, so their visible rim goes first
                // even though the stationary pixels are erased at the same rate everywhere.
                const float grain = rng.uniform(0.80f, 1.f);
                const Rgb inner = palette.star ? lerp(palette.core, palette.inner, std::min(1.f, u * 2.f))
                                               : palette.inner;
                Rgb color = scale(lerp(inner, palette.outer, u), brightness * grain * (1.f - 0.12f * u));
                if (t > length - (palette.star ? 7.f : 2.5f) && rng.chance(0.65f))
                    color = palette.star ? palette.tip : scale(palette.tip, brightness * grain);
                else if (rng.chance(0.025f))
                    color = scale(palette.tip, brightness);
                plot(x, y, color);
                if (palette.star && u > 0.55f) {
                    plot(x - sa, y + ca, scale(color, 0.85f));
                    plot(x + sa, y - ca, scale(color, 0.85f));
                }
            }
        }
    }

    // The bitmap-like inner fill remains visible between the thin hairs in fresh flowers.
    const float fillRadius = r * (palette.star ? 0.62f : 0.55f);
    const int fillExtent = static_cast<int>(std::ceil(fillRadius));
    const float fillGain = palette.star ? 0.95f : (paletteIndex == 0 ? 0.15f : 0.28f);
    for (int y = -fillExtent; y <= fillExtent; ++y) {
        for (int x = -fillExtent; x <= fillExtent; ++x) {
            const float d2 = static_cast<float>(x * x + y * y) / (fillRadius * fillRadius);
            if (d2 >= 1.f || rng.chance(palette.star ? 0.04f : 0.12f))
                continue;
            const float profile = palette.star ? (1.f - 0.35f * d2) * std::min(1.f, (1.f - d2) / 0.51f)
                                               : 1.f - d2;
            const float grain = palette.star ? rng.uniform(0.90f, 1.10f) : rng.uniform(0.60f, 1.f);
            plot(static_cast<float>(x), static_cast<float>(y), scale(palette.inner, fillGain * profile * grain));
        }
    }

    const float coreRadius = r * (paletteIndex == 8 ? 0.23f : (palette.star ? 0.24f : 0.13f));
    const int coreExtent = static_cast<int>(std::ceil(coreRadius));
    for (int y = -coreExtent; y <= coreExtent; ++y) {
        for (int x = -coreExtent; x <= coreExtent; ++x) {
            const float d = std::hypot(static_cast<float>(x), static_cast<float>(y)) / coreRadius;
            if (d < 1.f && !rng.chance(0.14f))
                plot(static_cast<float>(x), static_cast<float>(y),
                     scale(lerp(palette.core, palette.inner, d), (1.f - 0.35f * d) * rng.uniform(0.90f, 1.10f)));
        }
    }

    // One red flower in the recording has a small green-to-blue crescent beside its white core.
    if (paletteIndex == 7) {
        const int extent = static_cast<int>(std::ceil(r * 0.4f));
        for (int y = -extent; y <= extent; ++y) {
            for (int x = 0; x <= extent; ++x) {
                const float nx = (static_cast<float>(x) / r - 0.22f) / 0.08f;
                const float ny = (static_cast<float>(y) / r + 0.10f) / 0.28f;
                if (nx * nx + ny * ny >= 1.f || rng.chance(0.15f))
                    continue;
                const float mix = std::clamp((ny + 1.f) * 0.5f, 0.f, 1.f);
                grid[static_cast<std::size_t>(y + radius) * size + x + radius] =
                    scale(lerp(Rgb{65, 230, 150}, Rgb{45, 125, 255}, mix), rng.uniform(0.65f, 1.f));
            }
        }
    }

    std::vector<FireworkShow::FlowerPixel> pixels;
    const auto lit = std::ranges::count_if(grid, [](Rgb color) { return !color.isBlack(); });
    pixels.reserve(static_cast<std::size_t>(lit));
    for (int y = 0; y < size; ++y) {
        float life = 0.f;
        for (int x = 0; x < size; ++x) {
            // The source leaves little horizontal holes, also visible in the lettering.
            if ((x & 1) == 0)
                life = -std::log(rng.uniform(0.00001f, 1.f)) / kPixelLossRate;
            const Rgb color = grid[static_cast<std::size_t>(y) * size + x];
            if (color.isBlack())
                continue;
            const auto dx = static_cast<std::int16_t>(x - radius);
            const auto dy = static_cast<std::int16_t>(y - radius);
            const float distance = std::hypot(static_cast<float>(dx), static_cast<float>(dy));
            // Inverse of r(t) = speed*t / (1 + drag*t), fitted to the long reveal.
            const float appearsAt = distance / (kRevealSpeed - kRevealDrag * distance);
            pixels.push_back({appearsAt, appearsAt + life, dx, dy, color});
        }
    }
    std::ranges::sort(pixels, {}, &FireworkShow::FlowerPixel::appearsAt);
    return pixels;
}

} // namespace

void FireworkShow::update(float dt, Canvas& sky, Random& rng) {
    // Preserve each spark's colour until it is erased, rather than smoothly dimming every trail.
    erodeRemainder_ += static_cast<float>(sky.width() * sky.height()) * kPixelLossRate * dt * 0.5f;
    const int pairs = static_cast<int>(erodeRemainder_);
    sky.erode(pairs, rng);
    erodeRemainder_ -= static_cast<float>(pairs);
    elapsed_ += dt;

    while (cue_ < kLaunchCues.size() && elapsed_ >= kLaunchCues[cue_].at) {
        if (const auto slot = std::ranges::find(slots_, Phase::Idle, &Slot::phase); slot != slots_.end())
            launch(*slot, sky, rng, static_cast<int>(cue_));
        ++cue_;
    }

    launchTimer_ -= dt;
    if (launchTimer_ <= 0.f) {
        launchTimer_ += rng.uniform(0.22f, 0.42f);
        if (const auto slot = std::ranges::find(slots_, Phase::Idle, &Slot::phase); slot != slots_.end())
            launch(*slot, sky, rng, -1);
    }

    for (Slot& slot : slots_) {
        if (slot.phase == Phase::Rising)
            rise(slot, dt, sky, rng);
        else if (slot.phase == Phase::Bursting)
            bloom(slot, dt);
    }
}

void FireworkShow::draw(Canvas& frame) const {
    for (const Slot& slot : slots_)
        if (slot.phase == Phase::Bursting)
            drawBurst(slot, frame);

    const auto& sprites = rocketSprites();
    for (const Slot& slot : slots_) {
        if (slot.phase != Phase::Rising)
            continue;
        const int flicker = static_cast<int>(slot.elapsed * kFlickerRate) & 1;
        const Sprite& sprite = sprites[slot.rocketColor][flicker];
        const int left = static_cast<int>(std::lround(slot.rocketX)) - kRocketHeadX;
        const int top = static_cast<int>(std::lround(slot.rocketY)) - kRocketHeadY;
        for (int y = 0; y < sprite.height; ++y)
            for (int x = 0; x < sprite.width; ++x)
                if (const Rgb color = sprite.pixels[y * sprite.width + x]; !color.isBlack())
                    frame.lighten(left + x, top + y, color);
    }
}

void FireworkShow::launch(Slot& slot, const Canvas& sky, Random& rng, int cue) {
    if (cue >= 0) {
        const LaunchCue& recorded = kLaunchCues[cue];
        slot.rocketX = recorded.x;
        slot.rocketY = recorded.startY;
        slot.peakY = recorded.peakY;
        slot.radius = recorded.radius;
        slot.rocketColor = recorded.rocketColor;
        slot.palette = recorded.palette;
    } else {
        slot.rocketX = static_cast<float>(rng.range(6, sky.width() - 5));
        slot.rocketY = static_cast<float>(rng.range(sky.height() * 72 / 100, sky.height() * 92 / 100));
        slot.peakY = static_cast<float>(rng.range(12, sky.height() * 48 / 100));
        slot.radius = rng.range(95, 140);
        slot.rocketColor = rng.range(0, static_cast<int>(kRocketColors.size()) - 1);
        slot.palette = pickPalette(rng);
    }
    slot.climb = slot.rocketY - slot.peakY;
    slot.speed = cue >= 0 ? 920.f : rng.uniform(880.f, 970.f);
    slot.elapsed = 0.f;
    slot.phase = Phase::Rising;
}

void FireworkShow::rise(Slot& slot, float dt, Canvas& sky, Random& rng) {
    slot.elapsed += dt;
    const float previousY = slot.rocketY;
    const float remaining = previousY - slot.peakY;
    const float slowBoundary = slot.climb * kSlowClimbFraction;
    // Split the step at the speed change so a large dt cannot skip the slower final climb.
    const float fastDistance = std::max(0.f, remaining - slowBoundary);
    const float fastTime = std::min(dt, fastDistance / slot.speed);
    slot.rocketY = std::max(slot.peakY, previousY - slot.speed * fastTime -
                                                      slot.speed * kSlowSpeed * (dt - fastTime));

    const int centerX = static_cast<int>(std::lround(slot.rocketX));
    for (int y = static_cast<int>(slot.rocketY) + 20; y < static_cast<int>(previousY) + 20; ++y) {
        if (!rng.chance(0.27f))
            continue;
        const int x = centerX + rng.range(-3, 3);
        const Rgb color = scale(Rgb{235, 230, 211}, rng.uniform(0.35f, 0.85f));
        sky.lighten(x, y, color);
        if (rng.chance(0.45f))
            sky.lighten(x + 1, y, scale(color, 0.60f));
    }

    if (slot.rocketY <= slot.peakY) {
        slot.centerX = centerX;
        slot.centerY = static_cast<int>(std::lround(slot.peakY));
        slot.flower = makeFlower(rng, slot.radius, slot.palette);
        slot.deathOrder.resize(slot.flower.size());
        for (std::size_t i = 0; i < slot.deathOrder.size(); ++i)
            slot.deathOrder[i] = static_cast<std::uint32_t>(i);
        std::ranges::sort(slot.deathOrder, {}, [&](std::uint32_t index) { return slot.flower[index].disappearsAt; });
        const int width = slot.radius * 2 + 3;
        slot.raster.assign(static_cast<std::size_t>(width) * width, GlowPixel{});
        slot.born = 0;
        slot.dead = 0;
        slot.elapsed = 0.f;
        slot.phase = Phase::Bursting;
        bloom(slot, 0.f);
    }
}

void FireworkShow::bloom(Slot& slot, float dt) {
    slot.elapsed += dt;
    if (slot.elapsed >= kBurstLifetime) {
        slot.flower.clear();
        slot.deathOrder.clear();
        slot.raster.clear();
        slot.phase = Phase::Idle;
        return;
    }
    while (slot.born < slot.flower.size() && slot.flower[slot.born].appearsAt <= slot.elapsed)
        changePixel(slot, slot.flower[slot.born++], 1);
    while (slot.dead < slot.deathOrder.size() &&
           slot.flower[slot.deathOrder[slot.dead]].disappearsAt <= slot.elapsed)
        changePixel(slot, slot.flower[slot.deathOrder[slot.dead++]], -1);
}

void FireworkShow::changePixel(Slot& slot, const FlowerPixel& pixel, int sign) {
    // A tiny reconstruction filter matches the recording's soft, closely packed grain.
    // Update on birth/death only: no full-image convolution or allocation in draw().
    constexpr std::array kWeights{1, 2, 1};
    const int width = slot.radius * 2 + 3;
    const int x = pixel.dx + slot.radius + 1;
    const int y = pixel.dy + slot.radius + 1;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            GlowPixel& target = slot.raster[static_cast<std::size_t>(y + dy) * width + x + dx];
            const int weight = kWeights[dx + 1] * kWeights[dy + 1] * sign;
            target.r = static_cast<std::uint16_t>(target.r + pixel.color.r * weight);
            target.g = static_cast<std::uint16_t>(target.g + pixel.color.g * weight);
            target.b = static_cast<std::uint16_t>(target.b + pixel.color.b * weight);
        }
    }
}

void FireworkShow::drawBurst(const Slot& slot, Canvas& frame) {
    const int width = slot.radius * 2 + 3;
    const int left = slot.centerX - slot.radius - 1;
    const int top = slot.centerY - slot.radius - 1;
    for (int y = 0; y < width; ++y) {
        for (int x = 0; x < width; ++x) {
            const GlowPixel& pixel = slot.raster[static_cast<std::size_t>(y) * width + x];
            if ((pixel.r | pixel.g | pixel.b) == 0)
                continue;
            frame.lighten(left + x, top + y, {static_cast<std::uint8_t>(pixel.r / 16),
                                             static_cast<std::uint8_t>(pixel.g / 16),
                                             static_cast<std::uint8_t>(pixel.b / 16)});
        }
    }
}
