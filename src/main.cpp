#include "canvas.hpp"
#include "firework.hpp"
#include "greeting.hpp"
#include "random.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <filesystem>
#include <random>

namespace {

constexpr unsigned kWidth = 1200;
constexpr unsigned kHeight = 800;

// Random text-layer pixels blacked out per second: greetings decay at ~0.65/s per pixel, as measured.
constexpr float kErosionPerSecond = 620'000.f;

} // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode({kWidth, kHeight}), "Fireworks", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    const sf::Font serif(std::filesystem::path(FIREWORKS_ASSET_DIR) / "NotoSerifSC-VariableFont_wght.ttf");

    Random rng(std::random_device{}());
    Canvas sky(kWidth, kHeight);   // rocket spark trails; fades smoothly
    Canvas text(kWidth, kHeight);  // greetings; erodes into speckles
    Canvas frame(kWidth, kHeight); // composed output
    FireworkShow show;
    Banner banner(serif);
    BirthdayLetters letters(serif);

    sf::Texture texture(sf::Vector2u{kWidth, kHeight});
    const sf::Sprite sprite(texture);

    sf::Clock clock;
    float erosionDebt = 0.f;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>();
                     key && key->code == sf::Keyboard::Key::Escape)
                window.close();
        }

        const float dt = std::min(clock.restart().asSeconds(), 0.05f);

        erosionDebt += kErosionPerSecond * dt;
        const int pixels = static_cast<int>(erosionDebt);
        erosionDebt -= static_cast<float>(pixels);
        text.erode(pixels / 2, rng);

        letters.update(dt, text, rng);
        banner.update(dt, text, rng);
        show.update(dt, sky, rng);

        frame.composite(sky, text);
        banner.draw(frame, sky, text);
        show.draw(frame);
        texture.update(frame.rgba());
        window.clear();
        window.draw(sprite);
        window.display();
    }
}
