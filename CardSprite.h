#ifndef CARDSPRITE_H
#define CARDSPRITE_H

#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

enum class CardState {
    HIDDEN,
    REVEALED,
    MATCHED
};

class CardSprite {
private:
    sf::RectangleShape shape;
    sf::Text symbolText;
    sf::Sprite imageSprite;
    sf::Texture imageTexture;
    
    int id;
    std::string symbol;
    CardState state;
    bool isClickable;
    bool hasImage;
    
    void centerText();
    void centerImage();
    
public:
    CardSprite(int id, const std::string& symbol, float x, float y, float size);
    
    void setPosition(float x, float y);
    void setSymbol(const std::string& symbol, const sf::Font& font);
    void setSize(float size);
    void setState(CardState newState);
    void setClickable(bool clickable);

    bool loadImage(const std::string& imagePath);
    
    void flip();
    void reveal();
    void hide();
    void markMatched();
    
    bool contains(const sf::Vector2f& point) const;
    void update(float deltaTime);
    void render(sf::RenderWindow& window);
    
    int getId() const { return id; }
    std::string getSymbol() const { return symbol; }
    CardState getState() const { return state; }
    bool getIsClickable() const { return isClickable; }
    bool getHasImage() const { return hasImage; }
};

#endif
