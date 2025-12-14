#ifndef BUTTON_H
#define BUTTON_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Button {
private:
    sf::RectangleShape shape;
    sf::Text text;
    const sf::Font* fontPtr;
    
    sf::Color idleColor;
    sf::Color hoverColor;
    sf::Color activeColor;
    
    std::function<void()> onClick;
    
public:
    Button() : fontPtr(nullptr) {}
    
    Button(float x, float y, float width, float height, 
           const std::string& textStr, 
           const sf::Font& font,
           std::function<void()> onClickFunc);
    
    void update(const sf::Vector2f& mousePos);
    void render(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    
    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;
    void setColors(const sf::Color& idle, const sf::Color& hover, const sf::Color& active);
    void setText(const std::string& textStr);
    void setFont(const sf::Font& font);
    bool hasValidFont() const { return fontPtr != nullptr; }
};

#endif
