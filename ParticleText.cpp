#include "ParticleText.h"

#include "util.h"

ParticleText::ParticleText(const sf::String &text, const unsigned size) {
    static sf::Font font{"font.ttf"};

    float x = this->position.x, y = this->position.y;

    for (const auto c: text.toUtf32()) {
        const auto glyph = font.getGlyph(c, size, false);
        const auto texture = font.getTexture(size).copyToImage();

        const auto rect = glyph.textureRect;

        for (unsigned i = 0; i < rect.size.x; ++i) {
            for (unsigned j = 0; j < rect.size.y; ++j) {
                if (texture.getPixel({rect.position.x + i, rect.position.y + j}).a == 0) {
                    continue;
                }

                if (random() > 0.2f) {
                    continue;
                }

                sf::CircleShape circle(1);
                circle.setPosition({x + i, y + j + glyph.bounds.position.y + size});

                this->particles.push_back(std::move(circle));
            }
        }

        x += glyph.advance;
    }
}

void ParticleText::setPosition(const sf::Vector2f pos) {
    const auto delta = pos - this->position;

    for (auto &particle: particles) {
        particle.setPosition(particle.getPosition() + delta);
    }

    this->position = pos;
}

void ParticleText::draw(sf::RenderTarget &target, const sf::RenderStates states) const {
    for (const auto &particle: particles) {
        target.draw(particle, states);
    }
}
