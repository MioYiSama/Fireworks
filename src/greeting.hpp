#pragma once

#include "canvas.hpp"
#include "random.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/String.hpp>

#include <cstdint>
#include <vector>

// Anti-aliased coverage of a rendered string, tightly cropped.
struct TextMask {
    int width = 0;
    int height = 0;
    int baselineY = 0;  // row of the text baseline inside the mask
    std::vector<std::uint8_t> coverage;

    std::uint8_t at(int x, int y) const { return coverage[static_cast<std::size_t>(y) * width + x]; }
};

// Needs an active OpenGL context (create the window first). `embolden` thickens strokes by drawing
// a same-colour outline of that many pixels.
TextMask rasterizeText(const sf::Font& font, const sf::String& string, unsigned int size, std::uint32_t style = 0,
                       float embolden = 0.f, float letterSpacing = 1.f);

// Ming/serif "For.2024.宝宝:" drifting right, leaving randomly dissolving copies of each glyph.
class Banner {
public:
    explicit Banner(const sf::Font& serifFont);
    void update(float dt, Canvas& canvas, Random& rng);
    void draw(Canvas& frame, const Canvas& sky, const Canvas& text) const;

private:
    TextMask mask_;
    float left_;
    bool drawn_ = false;
    bool openingWake_ = true;
};

// Fixed-height serif letters, stamped in reading order and left to dissolve into speckles.
class BirthdayLetters {
public:
    explicit BirthdayLetters(const sf::Font& serif);
    void update(float dt, Canvas& canvas, Random& rng);

private:
    struct Letter {
        TextMask mask;
        int centerX;
        int baselineY;
    };

    std::vector<Letter> letters_;
    std::size_t next_ = 0;
    std::size_t cycle_ = 0;
    float elapsed_ = 0.f;
    float cycleStart_ = 0.f;
};
