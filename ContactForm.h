#ifndef CONTACT_FORM_H
#define CONTACT_FORM_H

#include <SFML/Graphics.hpp>
#include <string>
#include "EmailSender.h"

class ContactForm {
private:
    sf::Font font;
    
    sf::Text titleText;
    sf::Text nameLabel;
    sf::Text emailLabel;
    sf::Text messageLabel;
    sf::Text statusText;
    
    sf::RectangleShape nameBox;
    sf::RectangleShape emailBox;
    sf::RectangleShape messageBox;
    
    sf::RectangleShape sendButton;
    sf::RectangleShape backButton;
    sf::Text sendButtonText;
    sf::Text backButtonText;
    
    std::string nameInput;
    std::string emailInput;
    std::string messageInput;
    
    enum class ActiveField { NAME, EMAIL, MESSAGE, NONE };
    ActiveField activeField;
    
    EmailSender emailSender;
    
public:
    ContactForm();
    
    bool loadFont(const std::string& fontPath);
    void setup(float windowWidth, float windowHeight);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void update(const sf::Vector2f& mousePos);
    void render(sf::RenderWindow& window);
    
    void reset();
    
    bool isMouseOverButton(const sf::Vector2f& mousePos) const;
    bool isMouseOverBackButton(const sf::Vector2f& mousePos) const;
    
private:
    void sendFeedback();
    void updateActiveField(const sf::Vector2f& mousePos);
    void handleTextInput(sf::Uint32 unicode);
};

#endif
