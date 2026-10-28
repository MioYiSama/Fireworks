#include "greeting.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <span>

namespace {

struct GradientStop {
    float at;
    Rgb color;
};

Rgb sample(std::span<const GradientStop> stops, float t) {
    if (t <= stops.front().at)
        return stops.front().color;
    for (std::size_t i = 1; i < stops.size(); ++i)
        if (t <= stops[i].at)
            return lerp(stops[i - 1].color, stops[i].color, (t - stops[i - 1].at) / (stops[i].at - stops[i - 1].at));
    return stops.back().color;
}

// Lavender strokes turn cyan/green only in the lowest part of the line.
constexpr std::array<GradientStop, 4> kBannerGradient{{
    {0.f, {166, 160, 232}},
    {0.55f, {158, 166, 232}},
    {0.85f, {116, 165, 209}},
    {1.f, {60, 160, 152}},
}};
constexpr unsigned kBannerSize = 68;       // font rasterization size, px
constexpr int kBannerWidth = 470;           // single-copy tight ink bounds, px
constexpr int kBannerHeight = 63;
constexpr int kBannerBaseline = 175;        // bottom ink row, px
constexpr float kBannerStartX = 14.f;
constexpr float kBannerSpeed = 70.f;        // px/s
constexpr int kBannerGhostWidth = 8;        // px of bright, closely overlapping copies
constexpr float kBannerWakeBrightness = 0.75f; // surviving vacated strokes dim before random erosion
constexpr float kBannerWakeDensity = 0.65f;
constexpr float kBannerStrokeDensity = 0.9f;
constexpr float kBannerPrefixDensity = 0.7f; // the moving prefix is more perforated than the final CJK glyphs
constexpr int kBannerBackingTop = 100;       // px
constexpr int kBannerBackingHeight = 100;
constexpr int kBannerBackingWake = 100;      // px behind the moving line
constexpr int kBannerBackingRight = 12;
constexpr float kBannerBackingTransmission = 0.12f;

// Fresh-stamp hues measured across the line: warm ends, a yellow crest over "yBir",
// and paler gold on the right. The colour does not change as the stamped ink decays.
constexpr std::array<GradientStop, 8> kLetterGradient{{
    {0.f, {255, 170, 45}},
    {0.24f, {255, 215, 60}},
    {0.32f, {255, 234, 75}},
    {0.4f, {255, 230, 65}},
    {0.47f, {255, 235, 45}},
    {0.55f, {255, 241, 45}},
    {0.7f, {255, 218, 75}},
    {1.f, {255, 171, 80}},
}};
constexpr unsigned kLetterSize = 108; // px before fitting the narrow EasyX glyph cells
constexpr float kLetterFramesPerSecond = 30.f;

// Recording-frame onsets. Several stamps share one captured frame; there is no uniform cadence.
constexpr std::array<std::array<int, 13>, 3> kLetterStampFrames{{
    {{11, 21, 28, 35, 36, 38, 42, 44, 46, 48, 77, 78, 93}},
    {{0, 4, 8, 11, 33, 35, 50, 54, 56, 77, 100, 100, 117}},
    {{0, 3, 9, 15, 21, 39, 40, 51, 55, 77, 100, 100, 117}},
}};
constexpr int kLetterFirstCycleFrames = 109;
constexpr int kLetterRepeatCycleFrames = 124; // the third cycle's unseen tail repeats cycle two

struct LetterPlacement {
    char32_t glyph;
    int centerX;
    int baselineY;
    int inkWidth;
    int inkHeight;
    int descent;
};

// Ink bounds measured after mapping video (x, y) to (x / .9, (y - 374) / .9).
// Heights and x positions stay unchanged when the same letter is stamped again.
constexpr std::array<LetterPlacement, 13> kLetters{{
    {U'H', 19, 620, 40, 72, 0},   {U'a', 107, 593, 40, 49, 0},  {U'p', 198, 675, 39, 64, 16},
    {U'p', 289, 595, 39, 64, 16}, {U'y', 381, 596, 39, 62, 15}, {U'B', 472, 673, 43, 72, 0},
    {U'i', 562, 584, 27, 72, 0},  {U'r', 655, 587, 40, 49, 0},  {U't', 744, 635, 32, 62, 0},
    {U'h', 834, 680, 39, 73, 0},  {U'd', 926, 649, 38, 72, 0},  {U'a', 1017, 615, 40, 48, 0},
    {U'y', 1106, 602, 39, 64, 16},
}};

TextMask rasterizeLetter(const sf::Font& serif, const LetterPlacement& placement) {
    const TextMask source = rasterizeText(serif, sf::String(placement.glyph), kLetterSize, 0, 0.2f);
    int left = source.width;
    int right = 0;
    int top = source.height;
    int bottom = 0;
    for (int y = 0; y < source.height; ++y)
        for (int x = 0; x < source.width; ++x)
            if (source.at(x, y) != 0) {
                left = std::min(left, x);
                right = std::max(right, x + 1);
                top = std::min(top, y);
                bottom = std::max(bottom, y + 1);
            }

    TextMask mask;
    mask.width = placement.inkWidth;
    mask.height = placement.inkHeight;
    mask.baselineY = mask.height - placement.descent;
    mask.coverage.resize(static_cast<std::size_t>(mask.width) * mask.height);
    const float sx = static_cast<float>(right - left) / mask.width;
    // Fit the body and descender separately: the source font's p/y baseline differs from
    // the recording even when their complete ink rectangles have identical dimensions.
    const auto sourceY = [&](int y) {
        if (placement.descent == 0)
            return top + static_cast<float>(y) * (bottom - top) / mask.height;
        if (y <= mask.baselineY)
            return top + static_cast<float>(y) * (source.baselineY - top) / mask.baselineY;
        return source.baselineY + static_cast<float>(y - mask.baselineY) * (bottom - source.baselineY) / placement.descent;
    };
    for (int y = 0; y < mask.height; ++y) {
        const float y0 = sourceY(y);
        const float y1 = sourceY(y + 1);
        for (int x = 0; x < mask.width; ++x) {
            const float x0 = left + x * sx;
            const float x1 = left + (x + 1) * sx;
            float coverage = 0.f;
            for (int iy = static_cast<int>(std::floor(y0)); iy < std::min(bottom, static_cast<int>(std::ceil(y1))); ++iy) {
                const float wy = std::min(y1, static_cast<float>(iy + 1)) - std::max(y0, static_cast<float>(iy));
                for (int ix = static_cast<int>(std::floor(x0)); ix < std::min(right, static_cast<int>(std::ceil(x1))); ++ix) {
                    const float wx = std::min(x1, static_cast<float>(ix + 1)) - std::max(x0, static_cast<float>(ix));
                    coverage += source.at(ix, iy) * wx * wy;
                }
            }
            mask.coverage[static_cast<std::size_t>(y) * mask.width + x] = toChannel(coverage / (sx * (y1 - y0)));
        }
    }
    // The captured lettering has a one-pixel soft fringe, rather than razor-sharp
    // raster edges. Bake this once into the mask; erosion still acts on individual pixels.
    TextMask softened;
    softened.width = mask.width + 4;
    softened.height = mask.height + 4;
    softened.baselineY = mask.baselineY + 2;
    softened.coverage.resize(static_cast<std::size_t>(softened.width) * softened.height);
    constexpr std::array<int, 5> kernel{1, 4, 10, 4, 1};
    for (int y = 0; y < softened.height; ++y)
        for (int x = 0; x < softened.width; ++x) {
            int sum = 0;
            for (int ky = 0; ky < 5; ++ky)
                for (int kx = 0; kx < 5; ++kx) {
                    const int mx = x + kx - 4;
                    const int my = y + ky - 4;
                    if (mx >= 0 && mx < mask.width && my >= 0 && my < mask.height)
                        sum += mask.at(mx, my) * kernel[kx] * kernel[ky];
                }
            softened.coverage[static_cast<std::size_t>(y) * softened.width + x] = static_cast<std::uint8_t>(sum / 400);
        }
    return softened;
}

} // namespace

TextMask rasterizeText(const sf::Font& font, const sf::String& string, unsigned int size, std::uint32_t style,
                       float embolden, float letterSpacing) {
    constexpr int pad = 3;
    sf::Text text(font, string, size);
    text.setStyle(style);
    text.setLetterSpacing(letterSpacing);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::White);
    text.setOutlineThickness(embolden);

    const sf::FloatRect bounds = text.getLocalBounds();
    const auto originX = static_cast<int>(std::lround(pad - bounds.position.x));
    const auto originY = static_cast<int>(std::lround(pad - bounds.position.y));
    text.setPosition({static_cast<float>(originX), static_cast<float>(originY)});

    TextMask mask;
    mask.width = static_cast<int>(std::ceil(bounds.size.x)) + 2 * pad;
    mask.height = static_cast<int>(std::ceil(bounds.size.y)) + 2 * pad;
    mask.baselineY = originY + static_cast<int>(size); // sf::Text puts the baseline at characterSize

    sf::RenderTexture target({static_cast<unsigned>(mask.width), static_cast<unsigned>(mask.height)});
    target.clear(sf::Color::Transparent);
    target.draw(text);
    target.display();

    const sf::Image image = target.getTexture().copyToImage();
    const std::uint8_t* rgba = image.getPixelsPtr();
    mask.coverage.resize(static_cast<std::size_t>(mask.width) * mask.height);
    for (std::size_t i = 0; i < mask.coverage.size(); ++i)
        mask.coverage[i] = rgba[i * 4 + 3];
    return mask;
}

Banner::Banner(const sf::Font& serifFont) : left_(kBannerStartX) {
    const TextMask source = rasterizeText(serifFont, U"For.2024.宝宝:", kBannerSize, sf::Text::Bold);
    int minX = source.width;
    int minY = source.height;
    int maxX = 0;
    int maxY = 0;
    for (int y = 0; y < source.height; ++y)
        for (int x = 0; x < source.width; ++x)
            if (source.at(x, y) != 0) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }

    // EasyX's font width is independent of its height; fit the serif ink to the measured rectangle.
    mask_.width = kBannerWidth;
    mask_.height = kBannerHeight;
    mask_.baselineY = kBannerHeight - 1;
    mask_.coverage.resize(static_cast<std::size_t>(mask_.width) * mask_.height);
    for (int y = 0; y < mask_.height; ++y) {
        const float sy = minY + static_cast<float>(y) * (maxY - minY) / (mask_.height - 1);
        const int y0 = static_cast<int>(sy);
        const int y1 = std::min(y0 + 1, maxY);
        const float fy = sy - y0;
        for (int x = 0; x < mask_.width; ++x) {
            const float sx = minX + static_cast<float>(x) * (maxX - minX) / (mask_.width - 1);
            const int x0 = static_cast<int>(sx);
            const int x1 = std::min(x0 + 1, maxX);
            const float fx = sx - x0;
            const float upper = source.at(x0, y0) * (1.f - fx) + source.at(x1, y0) * fx;
            const float lower = source.at(x0, y1) * (1.f - fx) + source.at(x1, y1) * fx;
            mask_.coverage[static_cast<std::size_t>(y) * mask_.width + x] =
                static_cast<std::uint8_t>(std::lround(upper * (1.f - fy) + lower * fy));
        }
    }
}

void Banner::update(float dt, Canvas& canvas, Random& rng) {
    const int top = kBannerBaseline - mask_.baselineY;
    const float rowScale = 1.f / static_cast<float>(mask_.height - 1);
    const int previousLeft = static_cast<int>(std::floor(left_));
    if (drawn_) {
        left_ += kBannerSpeed * dt;
        if (left_ > static_cast<float>(canvas.width()))
            left_ = static_cast<float>(-mask_.width);
        const int shift = static_cast<int>(std::floor(left_)) - previousLeft;
        for (int lag = 0; lag <= (openingWake_ ? kBannerGhostWidth : 0); lag += kBannerGhostWidth / 2)
            for (int y = 0; y < mask_.height; ++y)
                for (int x = 0; x < mask_.width; ++x) {
                    const std::uint8_t was = mask_.at(x, y);
                    const int nextX = x - lag - shift;
                    const std::uint8_t now = nextX >= 0 && nextX < mask_.width ? mask_.at(nextX, y) : 0;
                    const int px = previousLeft + x - lag;
                    if (was != 0 && canvas.contains(px, top + y)) {
                        const bool keep = now != 0 || rng.chance(kBannerWakeDensity);
                        canvas.set(px, top + y,
                                   keep ? scale(canvas.get(px, top + y), kBannerWakeBrightness) : Rgb{});
                    }
                }
        openingWake_ = false;
    }

    const int left = static_cast<int>(std::floor(left_));
    for (int lag = 0; lag <= (drawn_ ? 0 : kBannerGhostWidth); lag += kBannerGhostWidth / 2)
        for (int y = 0; y < mask_.height; ++y) {
            const Rgb color = sample(kBannerGradient, static_cast<float>(y) * rowScale);
            for (int x = 0; x < mask_.width; ++x) {
                const float density = drawn_ ? kBannerPrefixDensity +
                                                   (kBannerStrokeDensity - kBannerPrefixDensity) * x / (mask_.width - 1)
                                             : kBannerStrokeDensity;
                if (const std::uint8_t c = mask_.at(x, y); c != 0 && rng.chance(density))
                    canvas.lighten(left + x - lag, top + y, scale(color, c / 255.f * (1.f - 0.025f * lag)));
            }
        }
    drawn_ = true;
}

void Banner::draw(Canvas& frame, const Canvas& sky, const Canvas& text) const {
    const int left = static_cast<int>(std::floor(left_));
    const int begin = std::max(0, left - kBannerBackingWake);
    const int end = std::min(frame.width(), left + mask_.width + kBannerBackingRight);
    for (int y = kBannerBackingTop; y < std::min(frame.height(), kBannerBackingTop + kBannerBackingHeight); ++y)
        for (int x = 0; x < end; ++x) {
            const Rgb background = sky.get(x, y);
            frame.set(x, y, x >= begin ? scale(background, kBannerBackingTransmission) : background);
            int red = 0;
            int green = 0;
            int blue = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int weight = (dx == 0 ? 2 : 1) * (dy == 0 ? 2 : 1);
                    const Rgb c = text.get(std::clamp(x + dx, 0, text.width() - 1),
                                           std::clamp(y + dy, 0, text.height() - 1));
                    red += weight * c.r;
                    green += weight * c.g;
                    blue += weight * c.b;
                }
            frame.lighten(x, y, {static_cast<std::uint8_t>(red / 16), static_cast<std::uint8_t>(green / 16),
                                 static_cast<std::uint8_t>(blue / 16)});
        }
}

BirthdayLetters::BirthdayLetters(const sf::Font& serif) {
    letters_.reserve(kLetters.size());
    for (const LetterPlacement& placement : kLetters)
        letters_.push_back({rasterizeLetter(serif, placement), placement.centerX, placement.baselineY});
}

void BirthdayLetters::update(float dt, Canvas& canvas, Random& rng) {
    elapsed_ += dt;
    while (elapsed_ >= cycleStart_ + kLetterStampFrames[cycle_][next_] / kLetterFramesPerSecond) {
        const Letter& letter = letters_[next_];
        const int left = letter.centerX - letter.mask.width / 2;
        const int top = letter.baselineY - letter.mask.baselineY;
        const Rgb color = sample(kLetterGradient, static_cast<float>(letter.centerX) / canvas.width());
        // The recording retains a soft pewter edge just below and right of the gold face.
        for (int y = 0; y < letter.mask.height; ++y)
            for (int x = 0; x < letter.mask.width; ++x)
                if (const std::uint8_t c = letter.mask.at(x, y); c != 0)
                    canvas.blend(left + x + 2, top + y + 2, {151, 148, 140}, c / 255.f);
        for (int y = 0; y < letter.mask.height; ++y)
            for (int x = 0; x < letter.mask.width; ++x)
                if (const std::uint8_t c = letter.mask.at(x, y); c != 0) {
                    // Fresh ink is already granular: bronze pits and pale gold sparkles survive
                    // unchanged until the shared pixel-pair erosion removes them.
                    const float grain = rng.uniform(0.f, 1.f);
                    Rgb ink = grain > 0.94f ? lerp(color, {255, 248, 181}, 0.45f)
                                           : scale(color, grain < 0.04f ? 0.58f : rng.uniform(0.86f, 1.f));
                    ink.g = toChannel(ink.g * rng.uniform(0.91f, 1.04f));
                    canvas.blend(left + x, top + y, ink, c / 255.f);
                }

        // A stamp never moves or flickers. Only the canvas's random pixel-pair erosion removes it.
        if (++next_ == letters_.size()) {
            next_ = 0;
            cycleStart_ += (cycle_ == 0 ? kLetterFirstCycleFrames : kLetterRepeatCycleFrames) / kLetterFramesPerSecond;
            cycle_ = std::min(cycle_ + 1, kLetterStampFrames.size() - 1);
        }
    }
}
