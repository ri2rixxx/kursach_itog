#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <set>
#include "Card.h"
#include "Player.h"
#include "Database.h"
#include "GUI/Button.h"
#include "GUI/CardSprite.h"
#include "GUI/Menu.h"
#include "Audio/SoundManager.h"
#include "Audio/MusicPlayer.h"
#include "ContactForm.h"

enum class GameState {
    MAIN_MENU,
    ENTER_NAME,
    SETUP,
    PLAYING,
    PAUSED,
    GAME_OVER_WIN,
    GAME_OVER_LOSE,
    LEADERBOARD,
    SETTINGS,
    CONTACT_FORM,  // НОВОЕ СОСТОЯНИЕ
    EXIT
};

enum class Difficulty {
    EASY,    // 3x4 = 12 cards (6 pairs)
    MEDIUM,  // 4x4 = 16 cards (8 pairs)
    HARD,    // 4x6 = 24 cards (12 pairs)
    EXPERT   // 6x6 = 36 cards (18 pairs)
};

class Game {
private:
    // Window and rendering
    sf::RenderWindow window;
    sf::Font mainFont;
    sf::Clock gameClock;
    sf::Time elapsedTime;
    
    // Database
    Database db;
    
    // Game elements
    std::vector<std::unique_ptr<CardSprite>> cards;
    std::unique_ptr<Player> player;
    std::unique_ptr<Database> database;
    std::unique_ptr<SoundManager> soundManager;
    std::unique_ptr<MusicPlayer> musicPlayer;
    std::vector<Card> gameCards;
    
    // Input
    std::string playerNameInput;
    bool isEnteringName;
    sf::Text nameInputText;
    sf::RectangleShape nameInputBox;
    
    // Backgrounds
    sf::Texture menuBackgroundTexture;
    sf::Texture gameBackgroundTexture;
    sf::Sprite background;
    sf::Color menuBackgroundColor;
    sf::Color gameBackgroundColor;
    
    // UI elements
    std::vector<Button> mainMenuButtons;
    std::vector<Button> gameButtons;
    std::vector<Button> pauseButtons;
    std::vector<Button> setupButtons;
    std::vector<Button> leaderboardButtons;
    std::vector<Button> settingsButtons;
    Button surrenderButton;
    
    // Settings
    float brightness;
    sf::VideoMode currentVideoMode;
    std::vector<sf::VideoMode> availableVideoModes;
    int currentVideoModeIndex;
    
    // Menu
    std::unique_ptr<Menu> mainMenu;
    std::unique_ptr<Menu> settingsMenu;
    
    // Text elements
    sf::Text titleText;
    sf::Text statsText;
    sf::Text timerText;
    sf::Text scoreText;
    sf::Text difficultyText;
    sf::Text settingsTitle;
    
    // Game state
    GameState currentState;
    GameState previousState;  // Для возврата из формы обратной связи
    Difficulty difficulty;
    CardTheme currentTheme;
    int rows, cols, totalPairs;
    int matchedPairs;
    int moves;
    bool isGameActive;
    bool firstCardSelected;
    int selectedCard1, selectedCard2;
    
    // Animation
    float cardFlipTime;
    float cardFlipProgress;
    bool isFlipping;
    
    // Card pointers for comparison
    CardSprite* firstCard;
    CardSprite* secondCard;
    bool isChecking;
    
    bool hasWon;
    
    // Resource paths
    std::map<CardTheme, std::string> themeImagePaths;
    
    // Форма обратной связи
    ContactForm contactForm;
    
    // Private methods
    void updateBackgrounds();
    void loadResources();
    void setupMainMenu();
    void setupGameUI();
    void setupPauseMenu();
    void setupSetupMenu();
    void setupLeaderboardUI();
    void setupSettingsMenu();
    void setupContactForm();
    void initializeCards();
    void createCardSprites();
    void resetGame();
    void updateStats();
    void handleCardClick(int cardIndex);
    void processCardMatch();
    void saveGameResult();
    void renderNameInput();
    void renderContactForm();
    std::string getDifficultyString() const;
    std::string getCurrentDate() const;
    sf::Color getDifficultyColor() const;
    void renderGame();
    void renderMainMenu();
    void renderPauseMenu();
    void renderSetupMenu();
    void renderGameOverWin();
    void renderGameOverLose();
    void renderLeaderboard();
    void renderSettings();
    void getImagePathsForTheme(CardTheme theme, std::vector<std::string>& imagePaths);

public:
    Game();
    ~Game();
    
    // Game loop methods
    void run();
    void handleEvents();
    void update(float deltaTime);
    void render();
    
    // Game control methods
    void startNewGame();
    void pauseGame();
    void resumeGame();
    void showLeaderboard();
    void showSettings();
    void exitGame();
    void surrenderGame();
    void setDifficulty(Difficulty diff);
    void setTheme(CardTheme theme);
    
    // Getter methods
    GameState getState() const { return currentState; }
    int getScore() const { return player ? player->getScore() : 0; }
    sf::Font& getMainFont() { return mainFont; }
};

#endif
