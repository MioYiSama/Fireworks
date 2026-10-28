#pragma once

#include "random.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

struct Rgb {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    bool isBlack() const { return (r | g | b) == 0; }
};

inline std::uint8_t toChannel(float value) { return static_cast<std::uint8_t>(std::clamp(value, 0.f, 255.f)); }

inline Rgb lerp(Rgb a, Rgb b, float t) {
    return {toChannel(a.r + (b.r - a.r) * t), toChannel(a.g + (b.g - a.g) * t), toChannel(a.b + (b.b - a.b) * t)};
}

inline Rgb scale(Rgb c, float k) { return {toChannel(c.r * k), toChannel(c.g * k), toChannel(c.b * k)}; }

// CPU-side RGBA buffer that is never cleared: traces stay until overwritten, faded or eroded.
class Canvas {
public:
    Canvas(int width, int height)
        : width_(width), height_(height), pixels_(static_cast<std::size_t>(width) * height, Pixel{}) {}

    int width() const { return width_; }
    int height() const { return height_; }

    bool contains(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }

    Rgb get(int x, int y) const {
        const Pixel& p = pixels_[index(x, y)];
        return {p.r, p.g, p.b};
    }

    void set(int x, int y, Rgb c) {
        if (contains(x, y))
            pixels_[index(x, y)] = {c.r, c.g, c.b, 255};
    }

    void blend(int x, int y, Rgb c, float coverage) {
        if (contains(x, y))
            set(x, y, lerp(get(x, y), c, coverage));
    }

    // Per-channel max: light adds up instead of covering.
    void lighten(int x, int y, Rgb c) {
        if (!contains(x, y))
            return;
        Pixel& p = pixels_[index(x, y)];
        p = {std::max(p.r, c.r), std::max(p.g, c.g), std::max(p.b, c.b), 255};
    }

    // Blacks out `pairs` random horizontal pixel pairs: the slow speckled dissolve of the text layer.
    void erode(int pairs, Random& rng) {
        for (int i = 0; i < pairs; ++i) {
            const std::size_t at = index(rng.range(0, width_ - 2), rng.range(0, height_ - 1));
            pixels_[at] = pixels_[at + 1] = Pixel{};
        }
    }

    // Smoothly dims every pixel; flooring guarantees everything reaches black.
    void fade(float keep) {
        std::array<std::uint8_t, 256> table{};
        for (int v = 0; v < 256; ++v)
            table[v] = static_cast<std::uint8_t>(static_cast<float>(v) * keep);
        for (Pixel& p : pixels_)
            p = {table[p.r], table[p.g], table[p.b], 255};
    }

    // this = max(a, b) per channel; all three canvases share one size.
    void composite(const Canvas& a, const Canvas& b) {
        for (std::size_t i = 0; i < pixels_.size(); ++i) {
            const Pixel& pa = a.pixels_[i];
            const Pixel& pb = b.pixels_[i];
            pixels_[i] = {std::max(pa.r, pb.r), std::max(pa.g, pb.g), std::max(pa.b, pb.b), 255};
        }
    }

    const std::uint8_t* rgba() const { return &pixels_.front().r; }

private:
    struct Pixel {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;
    };
    static_assert(sizeof(Pixel) == 4);

    std::size_t index(int x, int y) const { return static_cast<std::size_t>(y) * width_ + x; }

    int width_;
    int height_;
    std::vector<Pixel> pixels_;
};
