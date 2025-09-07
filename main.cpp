#include "ParticleText.h"

int main() {
    auto window = sf::RenderWindow(sf::VideoMode({1280, 720}), "Fireworks", sf::Style::Default & ~sf::Style::Resize);
    window.setFramerateLimit(60);

    // ParticleText particleText{L"H", 64};
    ParticleText texts[] = {
        {L"这", 64},
        {L"是", 64},
        {L"一", 64},
        {L"段", 64},
        {L"测", 64},
        {L"试", 64},
        {L"文", 64},
        {L"本", 64},
        {L"生", 64},
        {L"日", 64},
        {L"快", 64},
        {L"乐", 64},
        {L"！", 64},
    };

    texts[0].setPosition({0, 400 + 20});
    texts[1].setPosition({50, 400 + 10});
    texts[2].setPosition({100, 400 + 80});
    texts[3].setPosition({150, 400 + 20});
    texts[4].setPosition({200, 400 + 20});
    texts[5].setPosition({250, 400 + 100});
    texts[6].setPosition({300, 400 + 0});
    texts[7].setPosition({350, 400 + 0});
    texts[8].setPosition({400, 400 + 50});
    texts[9].setPosition({450, 400 + 100});
    texts[10].setPosition({500, 400 + 70});
    texts[11].setPosition({550, 400 + 50});
    texts[12].setPosition({630, 400 + 20});

    sf::Clock clock{};

    while (window.isOpen()) {
        const auto delta = clock.restart();
        std::cout << delta.asSeconds() << std::endl;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::Escape:
                        window.close();
                        break;
                    default:
                        break;
                }
            }
        }

        window.clear();
        for (const auto &text: texts) {
            window.draw(text);
        }
        window.display();
    }

    return 0;
}
