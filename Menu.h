#ifndef MENU_H
#define MENU_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include "GUI/Button.h"

class Menu {
private:
    std::vector<Button> buttons;
    sf::RectangleShape background;
    sf::Text title;
    bool isVisible;
    
public:
    Menu();
    
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setTitle(const std::string& titleText, const sf::Font& font);
    void addButton(const Button& button);
    void clearButtons();
    
    void show();
    void hide();
    bool getIsVisible() const;
    
    void update(const sf::Vector2f& mousePos);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void render(sf::RenderWindow& window);
    
    void setBackgroundColor(const sf::Color& color);
    void setTitleColor(const sf::Color& color);
};

#endif

