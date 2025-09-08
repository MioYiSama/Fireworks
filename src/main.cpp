#include "ParticleText.h"

int main() {
  sf::Font font{"assets/font.ttf"};
  auto window = sf::RenderWindow(sf::VideoMode({1920, 1080}), "Fireworks",
                                 sf::Style::Default & ~sf::Style::Resize);
  window.setFramerateLimit(60);

  // ParticleText particleText{L"H", 64};
  ParticleText texts[] = {
      {font, L"H", 64}, {font, L"a", 64}, {font, L"p", 64}, {font, L"p", 64},
      {font, L"y", 64}, {font, L"B", 64}, {font, L"i", 64}, {font, L"r", 64},
      {font, L"t", 64}, {font, L"h", 64}, {font, L"d", 64}, {font, L"a", 64},
      {font, L"y", 64},
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
  float sum{0};

  while (window.isOpen()) {
    const auto delta = clock.restart();
    sum += delta.asSeconds();

    for (int i = 0; i <= std::min(sum * 5, 12.f); ++i) {
      texts[i].update(delta);
    }

    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto *keyPressed =
                     event->getIf<sf::Event::KeyPressed>()) {
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
    for (const auto &text : texts) {
      window.draw(text);
    }
    window.display();
  }

  return 0;
}
