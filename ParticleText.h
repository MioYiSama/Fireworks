#pragma once

class ParticleText final : public sf::Drawable {
public:
    ParticleText(const sf::String &text, unsigned size);

    void setPosition(sf::Vector2f pos);

protected:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

private:
    sf::Vector2f position{0, 0};
    std::vector<sf::CircleShape> particles{};
};
