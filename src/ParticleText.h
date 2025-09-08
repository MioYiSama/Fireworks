#pragma once

class ParticleText final : public sf::Drawable {
public:
    ParticleText(const sf::Font& font, const sf::String &text, unsigned size);

    void setPosition(const sf::Vector2f& pos);

    void update(sf::Time dt);

protected:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

private:
    sf::Vector2f position{0, 0};
    std::vector<sf::CircleShape> particles{};

    float sum{0};
};
