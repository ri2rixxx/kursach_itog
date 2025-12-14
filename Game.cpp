#include "Game.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
#include <clocale>
#include <fstream>
#include <filesystem>
#include <set>
#include <map>

namespace fs = std::filesystem;

// Вспомогательная функция для проверки Docker
bool isRunningInDockerInternal() {
    std::ifstream dockerEnv("/.dockerenv");
    if (dockerEnv.good()) {
        return true;
    }
    
    std::ifstream cgroup("/proc/self/cgroup");
    if (cgroup.is_open()) {
        std::string line;
        while (std::getline(cgroup, line)) {
            if (line.find("docker") != std::string::npos ||
                line.find("kubepods") != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

Game::Game() 
    : window(sf::VideoMode(1200, 800), "Memory Game", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize),
      brightness(1.0f),
      currentVideoMode(1200, 800),
      currentVideoModeIndex(2),
      currentState(GameState::MAIN_MENU),
      previousState(GameState::MAIN_MENU),
      difficulty(Difficulty::MEDIUM),
      currentTheme(CardTheme::ANIMALS),
      rows(4),
      cols(4),
      totalPairs(8),
      matchedPairs(0),
      moves(0),
      isGameActive(false),
      firstCardSelected(false),
      selectedCard1(-1),
      selectedCard2(-1),
      cardFlipTime(0.3f),
      cardFlipProgress(0.0f),
      isFlipping(false),
      firstCard(nullptr),
      secondCard(nullptr),
      isChecking(false),
      hasWon(false)
{
    std::cout << "=== ИНИЦИАЛИЗАЦИЯ ИГРЫ ===" << std::endl;
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    
    std::cout << "Настройки по умолчанию:" << std::endl;
    std::cout << "  Сложность: Medium (4x4)" << std::endl;
    std::cout << "  Всего пар: " << totalPairs << std::endl;
    
    // Доступные разрешения
    availableVideoModes = {
        sf::VideoMode(800, 600),
        sf::VideoMode(1024, 768),
        sf::VideoMode(1200, 800),
        sf::VideoMode(1280, 720),
        sf::VideoMode(1366, 768),
        sf::VideoMode(1920, 1080)
    };
    
    // Загрузка ресурсов
    std::cout << "Загрузка ресурсов..." << std::endl;
    loadResources();
    std::cout << "Ресурсы загружены" << std::endl;
    
    // Кнопка сдачи
    surrenderButton = Button(950, 700, 200, 50, "Surrender", mainFont, 
                            [this]() { surrenderGame(); });
    surrenderButton.setColors(sf::Color(220, 20, 60), sf::Color(255, 0, 0), sf::Color(178, 34, 34));
    
    // Настройка UI
    setupMainMenu();
    setupGameUI();
    setupPauseMenu();
    setupSetupMenu();
    setupLeaderboardUI();
    setupSettingsMenu();
    setupContactForm();
    
    // Инициализация умных указателей
    soundManager = std::make_unique<SoundManager>();
    musicPlayer = std::make_unique<MusicPlayer>();
    
    // === ПРОСТАЯ ИНИЦИАЛИЗАЦИЯ БАЗЫ ДАННЫХ ===
    std::string dbPath = "memory_game.db";
    
    // Проверяем Docker простым способом
    std::ifstream dockerFile("/.dockerenv");
    if (dockerFile.good()) {
        std::cout << "🐳 Запущено в Docker" << std::endl;
        dbPath = "/app/database/memory_game.db";
        std::cout << "📁 Путь к БД в Docker: " << dbPath << std::endl;
        
        // Просто пытаемся создать директорию
        system("mkdir -p /app/database 2>/dev/null");
    } else {
        std::cout << "💻 Запущено локально" << std::endl;
        std::cout << "📁 Путь к БД локально: " << dbPath << std::endl;
    }
    
    // Пытаемся создать базу данных
    try {
        std::cout << "Создаем базу данных..." << std::endl;
        database = std::make_unique<Database>(dbPath);
        
        if (database->initialize()) {
            std::cout << "✅ База данных инициализирована" << std::endl;
            
            // Проверяем что-нибудь в базе
            auto testRecords = database->getTopScores(1);
            if (!testRecords.empty()) {
                std::cout << "📊 В базе найдено записей: " << testRecords.size() << std::endl;
            } else {
                std::cout << "📊 База данных пуста (первый запуск)" << std::endl;
            }
        } else {
            std::cout << "⚠ Не удалось инициализировать БД" << std::endl;
            database = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка при создании БД: " << e.what() << std::endl;
        std::cout << "⚠ Продолжаем без базы данных" << std::endl;
        database = nullptr;
    }
    
    std::cout << "=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===" << std::endl;
}

Game::~Game() {
    std::cout << "Игра завершена." << std::endl;
}

void Game::loadResources() {
    // Загрузка шрифта
    std::vector<std::string> fontPaths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/local/share/memory_game/assets/fonts/gamefont.ttf",
        "assets/fonts/arial.ttf",
        "./arial.ttf"
    };
    
    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (mainFont.loadFromFile(path)) {
            fontLoaded = true;
            std::cout << "Шрифт загружен: " << path << std::endl;
            break;
        }
    }
    
    if (!fontLoaded) {
        throw std::runtime_error("Cannot load any font!");
    }
    
    // Инициализируем цвета фона
    menuBackgroundColor = sf::Color(30, 30, 60);
    gameBackgroundColor = sf::Color(20, 20, 40);
    
    updateBackgrounds();
    
    // Настройка текстовых элементов
    titleText.setFont(mainFont);
    titleText.setString("Memory Game");
    titleText.setCharacterSize(72);
    titleText.setFillColor(sf::Color::White);
    titleText.setStyle(sf::Text::Bold);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(2);
    
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                        titleBounds.top + titleBounds.height / 2.0f);
    titleText.setPosition(600, 100);
    
    statsText.setFont(mainFont);
    statsText.setCharacterSize(24);
    statsText.setFillColor(sf::Color::White);
    statsText.setPosition(50, 50);
    
    timerText.setFont(mainFont);
    timerText.setCharacterSize(32);
    timerText.setFillColor(sf::Color::White);
    timerText.setPosition(50, 100);
    
    scoreText.setFont(mainFont);
    scoreText.setCharacterSize(32);
    scoreText.setFillColor(sf::Color::Yellow);
    scoreText.setPosition(50, 150);
    
    difficultyText.setFont(mainFont);
    difficultyText.setCharacterSize(28);
    difficultyText.setFillColor(sf::Color::White);
    difficultyText.setPosition(50, 200);
    
    settingsTitle.setFont(mainFont);
    settingsTitle.setString("Settings");
    settingsTitle.setCharacterSize(48);
    settingsTitle.setFillColor(sf::Color::White);
    settingsTitle.setStyle(sf::Text::Bold);
    settingsTitle.setPosition(400, 100);
    
    nameInputText.setFont(mainFont);
    nameInputText.setCharacterSize(32);
    nameInputText.setFillColor(sf::Color::White);
    
    nameInputBox.setSize(sf::Vector2f(400, 60));
    nameInputBox.setFillColor(sf::Color(50, 50, 50));
    nameInputBox.setOutlineThickness(2);
    nameInputBox.setOutlineColor(sf::Color::White);
}

void Game::setupContactForm() {
    std::cout << "Настройка формы обратной связи..." << std::endl;
    
    // Пытаемся загрузить шрифт для формы
    std::vector<std::string> fontPaths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
    };
    
    for (const auto& path : fontPaths) {
        if (contactForm.loadFont(path)) {
            std::cout << "Шрифт для формы загружен: " << path << std::endl;
            break;
        }
    }
    
    // Настраиваем форму
    contactForm.setup(window.getSize().x, window.getSize().y);
}

void Game::updateBackgrounds() {
    // Применяем яркость
    sf::Color adjustedMenuColor(
        std::min(255, int(menuBackgroundColor.r * brightness)),
        std::min(255, int(menuBackgroundColor.g * brightness)),
        std::min(255, int(menuBackgroundColor.b * brightness))
    );
    
    sf::Color adjustedGameColor(
        std::min(255, int(gameBackgroundColor.r * brightness)),
        std::min(255, int(gameBackgroundColor.g * brightness)),
        std::min(255, int(gameBackgroundColor.b * brightness))
    );
    
    // Создаем текстуры
    sf::Image menuImage;
    menuImage.create(window.getSize().x, window.getSize().y, adjustedMenuColor);
    menuBackgroundTexture.loadFromImage(menuImage);
    
    sf::Image gameImage;
    gameImage.create(window.getSize().x, window.getSize().y, adjustedGameColor);
    gameBackgroundTexture.loadFromImage(gameImage);
    
    background.setTexture(menuBackgroundTexture);
}

void Game::setupMainMenu() {
    mainMenuButtons.clear();
    
    float buttonWidth = 300.0f;
    float buttonHeight = 60.0f;
    float startY = 300.0f;
    float spacing = 80.0f;
    
    mainMenuButtons.emplace_back(
        450.0f, startY, buttonWidth, buttonHeight, 
        "New Game", mainFont, 
        [this]() { startNewGame(); }
    );
    
    mainMenuButtons.emplace_back(
        450.0f, startY + spacing, buttonWidth, buttonHeight, 
        "Leaderboard", mainFont, 
        [this]() { showLeaderboard(); }
    );
    
    mainMenuButtons.emplace_back(
        450.0f, startY + spacing * 2, buttonWidth, buttonHeight, 
        "Settings", mainFont, 
        [this]() { showSettings(); }
    );
    
    mainMenuButtons.emplace_back(
        450.0f, startY + spacing * 3, buttonWidth, buttonHeight, 
        "Exit", mainFont, 
        [this]() { exitGame(); }
    );
    
    for (auto& button : mainMenuButtons) {
        button.setColors(sf::Color(70, 130, 180), sf::Color(100, 149, 237), sf::Color(30, 144, 255));
    }
}

void Game::setupSettingsMenu() {
    settingsButtons.clear();
    
    float buttonWidth = 300.0f;
    float buttonHeight = 60.0f;
    float centerX = 450.0f;
    float startY = 200.0f;
    float spacing = 80.0f;
    
    // Яркость
    settingsButtons.emplace_back(
        centerX, startY, buttonWidth, buttonHeight,
        "Brightness: 100%", mainFont,
        [this]() { 
            brightness += 0.1f;
            if (brightness > 1.5f) brightness = 0.5f;
            
            std::stringstream ss;
            ss << "Brightness: " << int(brightness * 100) << "%";
            settingsButtons[0].setText(ss.str());
            
            updateBackgrounds();
        }
    );
    
    // Разрешение
    settingsButtons.emplace_back(
        centerX, startY + spacing, buttonWidth, buttonHeight,
        "Resolution: 1200x800", mainFont,
        [this]() { 
            currentVideoModeIndex = (currentVideoModeIndex + 1) % availableVideoModes.size();
            currentVideoMode = availableVideoModes[currentVideoModeIndex];
            
            std::stringstream ss;
            ss << "Resolution: " << currentVideoMode.width << "x" << currentVideoMode.height;
            settingsButtons[1].setText(ss.str());
            
            window.create(currentVideoMode, "Memory Game", 
                         sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
            window.setFramerateLimit(60);
            
            updateBackgrounds();
        }
    );
    
    // Обратная связь
    settingsButtons.emplace_back(
        centerX, startY + spacing * 2, buttonWidth, buttonHeight,
        "Contact Developer", mainFont,
        [this]() { 
            previousState = currentState;
            currentState = GameState::CONTACT_FORM;
            contactForm.reset();
        }
    );
    
    // Назад
    settingsButtons.emplace_back(
        centerX, startY + spacing * 3, buttonWidth, buttonHeight,
        "Back to Menu", mainFont,
        [this]() { 
            currentState = GameState::MAIN_MENU;
            background.setTexture(menuBackgroundTexture);
        }
    );
    
    // Цвета кнопок
    settingsButtons[0].setColors(sf::Color(138, 43, 226), sf::Color(148, 0, 211), sf::Color(128, 0, 128));
    settingsButtons[1].setColors(sf::Color(138, 43, 226), sf::Color(148, 0, 211), sf::Color(128, 0, 128));
    settingsButtons[2].setColors(sf::Color(70, 130, 180), sf::Color(100, 149, 237), sf::Color(30, 144, 255));
    settingsButtons[3].setColors(sf::Color(220, 20, 60), sf::Color(255, 0, 0), sf::Color(178, 34, 34));
}

void Game::setupGameUI() {
    gameButtons.clear();
    
    float buttonWidth = 150.0f;
    float buttonHeight = 40.0f;
    
    gameButtons.emplace_back(
        window.getSize().x - 200, 50.0f, buttonWidth, buttonHeight,
        "Pause", mainFont,
        [this]() { pauseGame(); }
    );
    
    gameButtons.emplace_back(
        window.getSize().x - 200, 100.0f, buttonWidth, buttonHeight,
        "Menu", mainFont,
        [this]() { 
            currentState = GameState::MAIN_MENU;
            background.setTexture(menuBackgroundTexture);
        }
    );
    
    gameButtons.emplace_back(
        window.getSize().x - 200, 150.0f, buttonWidth, buttonHeight,
        "Restart", mainFont,
        [this]() { startNewGame(); }
    );
    
    for (auto& button : gameButtons) {
        button.setColors(
            sf::Color(50, 205, 50),
            sf::Color(60, 215, 60),
            sf::Color(40, 195, 40)
        );
    }
    
    // Обновляем позицию кнопки сдачи
    surrenderButton.setPosition(window.getSize().x - 250, window.getSize().y - 100);
}

void Game::setupPauseMenu() {
    pauseButtons.clear();
    
    float buttonWidth = 250.0f;
    float buttonHeight = 60.0f;
    float centerX = window.getSize().x / 2 - buttonWidth / 2;
    float startY = 350.0f;
    float spacing = 80.0f;
    
    pauseButtons.emplace_back(centerX, startY, buttonWidth, buttonHeight, "Resume", mainFont, 
                             [this]() { resumeGame(); });
    pauseButtons.emplace_back(centerX, startY + spacing, buttonWidth, buttonHeight, "Restart", mainFont, 
                             [this]() { startNewGame(); });
    pauseButtons.emplace_back(centerX, startY + spacing * 2, buttonWidth, buttonHeight, "Main Menu", mainFont, 
                             [this]() { 
                                 currentState = GameState::MAIN_MENU;
                                 background.setTexture(menuBackgroundTexture); 
                             });
    
    for (auto& button : pauseButtons) {
        button.setColors(sf::Color(255, 165, 0), sf::Color(255, 185, 0), sf::Color(255, 140, 0));
    }
}

void Game::setupSetupMenu() {
    setupButtons.clear();
    
    float buttonWidth = 300.0f;
    float buttonHeight = 60.0f;
    float centerX = 450.0f;
    float startY = 200.0f;
    float spacing = 100.0f;
    
    // Сложность
    setupButtons.emplace_back(centerX, startY, buttonWidth, buttonHeight, "Difficulty: Medium", mainFont, 
                             [this]() { 
        switch (difficulty) {
            case Difficulty::EASY: 
                setDifficulty(Difficulty::MEDIUM); 
                setupButtons[0].setText("Difficulty: Medium"); 
                break;
            case Difficulty::MEDIUM: 
                setDifficulty(Difficulty::HARD); 
                setupButtons[0].setText("Difficulty: Hard"); 
                break;
            case Difficulty::HARD: 
                setDifficulty(Difficulty::EXPERT); 
                setupButtons[0].setText("Difficulty: Expert"); 
                break;
            case Difficulty::EXPERT: 
                setDifficulty(Difficulty::EASY); 
                setupButtons[0].setText("Difficulty: Easy"); 
                break;
        }
    });
    
    // Тема
    setupButtons.emplace_back(centerX, startY + spacing, buttonWidth, buttonHeight, "Theme: Animals", mainFont, 
                             [this]() { 
        switch (currentTheme) {
            case CardTheme::ANIMALS: 
                setTheme(CardTheme::FRUITS); 
                setupButtons[1].setText("Theme: Fruits"); 
                break;
            case CardTheme::FRUITS: 
                setTheme(CardTheme::EMOJI); 
                setupButtons[1].setText("Theme: Emoji"); 
                break;
            case CardTheme::EMOJI: 
                setTheme(CardTheme::MEMES); 
                setupButtons[1].setText("Theme: Memes"); 
                break;
            case CardTheme::MEMES: 
                setTheme(CardTheme::SYMBOLS); 
                setupButtons[1].setText("Theme: Symbols"); 
                break;
            case CardTheme::SYMBOLS: 
                setTheme(CardTheme::ANIMALS); 
                setupButtons[1].setText("Theme: Animals"); 
                break;
        }
    });
    
    // Начать игру
    setupButtons.emplace_back(centerX, startY + spacing * 2, buttonWidth, buttonHeight, "Start Game!", mainFont, 
                             [this]() { 
        if (player) {
            resetGame();
            currentState = GameState::PLAYING;
            background.setTexture(gameBackgroundTexture);
            isGameActive = true;
            gameClock.restart();
            std::cout << "Игра начата! Всего пар: " << totalPairs << std::endl;
        }
    });
    
    // Назад
    setupButtons.emplace_back(centerX, startY + spacing * 3, buttonWidth, buttonHeight, "Back to Menu", mainFont, 
                             [this]() { 
        currentState = GameState::MAIN_MENU;
        titleText.setString("Memory Game");
        titleText.setCharacterSize(72);
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                           titleBounds.top + titleBounds.height / 2.0f);
        titleText.setPosition(window.getSize().x / 2, 100);
    });
    
    // Цвета
    for (int i = 0; i < 2; i++) {
        setupButtons[i].setColors(sf::Color(138, 43, 226), sf::Color(148, 0, 211), sf::Color(128, 0, 128));
    }
    setupButtons[2].setColors(sf::Color(0, 200, 0), sf::Color(0, 230, 0), sf::Color(0, 170, 0));
    setupButtons[3].setColors(sf::Color(220, 20, 60), sf::Color(255, 0, 0), sf::Color(178, 34, 34));
}

void Game::setupLeaderboardUI() {
    leaderboardButtons.clear();
    
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float centerX = window.getSize().x / 2 - buttonWidth / 2;
    float buttonY = window.getSize().y - 100;
    
    leaderboardButtons.emplace_back(centerX, buttonY, buttonWidth, buttonHeight, "Back to Menu", mainFont, 
                                   [this]() { 
        currentState = GameState::MAIN_MENU;
    });
    
    leaderboardButtons[0].setColors(sf::Color(70, 130, 180), sf::Color(100, 149, 237), sf::Color(30, 144, 255));
}

void Game::getImagePathsForTheme(CardTheme theme, std::vector<std::string>& imagePaths) {
    std::string themeFolder;
    
    switch (theme) {
        case CardTheme::ANIMALS: themeFolder = "animals"; break;
        case CardTheme::FRUITS: themeFolder = "fruits"; break;
        case CardTheme::EMOJI: themeFolder = "emoji"; break;
        case CardTheme::MEMES: themeFolder = "memes"; break;
        case CardTheme::SYMBOLS: themeFolder = "symbols"; break;
        default: themeFolder = "animals"; break;
    }
    
    std::string imageDir = "assets/images/" + themeFolder + "/";
    std::cout << "📁 Поиск изображений в: " << imageDir << std::endl;
    
    imagePaths.clear();
    
    try {
        int foundCount = 0;
        for (const auto& entry : fs::directory_iterator(imageDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    imagePaths.push_back(entry.path().string());
                    foundCount++;
                    
                    if (foundCount <= 5) {
                        std::cout << "   ✅ " << entry.path().filename() << std::endl;
                    }
                }
            }
        }
        
        if (foundCount > 5) {
            std::cout << "   ... и еще " << (foundCount - 5) << " файлов" << std::endl;
        }
        
        std::cout << "📊 Найдено файлов: " << foundCount << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Ошибка доступа к папке: " << e.what() << std::endl;
        // Создаем тестовые пути
        for (int i = 1; i <= 18; i++) {
            imagePaths.push_back(imageDir + "image" + std::to_string(i) + ".png");
        }
    }
}

void Game::initializeCards() {
    gameCards.clear();
    
    // Устанавливаем размеры
    switch (difficulty) {
        case Difficulty::EASY: rows = 3; cols = 4; totalPairs = 6; break;
        case Difficulty::MEDIUM: rows = 4; cols = 4; totalPairs = 8; break;
        case Difficulty::HARD: rows = 4; cols = 6; totalPairs = 12; break;
        case Difficulty::EXPERT: rows = 6; cols = 6; totalPairs = 18; break;
    }
    
    int totalCards = rows * cols;
    std::cout << "\n=== ИНИЦИАЛИЗАЦИЯ КАРТ ===" << std::endl;
    std::cout << "Поле: " << rows << "x" << cols << " = " << totalCards << " карт" << std::endl;
    std::cout << "Нужно пар: " << totalPairs << std::endl;
    
    // 1. Получаем файлы из папки текущей темы
    std::string themeFolder;
    switch (currentTheme) {
        case CardTheme::ANIMALS: themeFolder = "animals"; break;
        case CardTheme::FRUITS: themeFolder = "fruits"; break;
        case CardTheme::EMOJI: themeFolder = "emoji"; break;
        case CardTheme::MEMES: themeFolder = "memes"; break;
        case CardTheme::SYMBOLS: themeFolder = "symbols"; break;
        default: themeFolder = "animals"; break;
    }
    
    std::string imageDir = "assets/images/" + themeFolder + "/";
    std::cout << "Ищем изображения в: " << imageDir << std::endl;
    
    // 2. Собираем список доступных файлов
    std::vector<std::string> availableImages;
    try {
        for (const auto& entry : fs::directory_iterator(imageDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    availableImages.push_back(entry.path().string());
                    std::cout << "  Найдено: " << entry.path().filename() << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка доступа к папке: " << e.what() << std::endl;
    }
    
    // 3. Если нет файлов, создаем тестовые имена
    if (availableImages.empty()) {
        std::cout << "Файлы не найдены, создаем тестовые..." << std::endl;
        for (int i = 1; i <= totalPairs; i++) {
            availableImages.push_back(imageDir + "image" + std::to_string(i) + ".png");
        }
    }
    
    // 4. ГАРАНТИРУЕМ ПАРНОСТЬ
    std::vector<std::string> pairedImages;
    
    // Берем первые totalPairs уникальных изображений
    int imagesToUse = std::min(totalPairs, (int)availableImages.size());
    for (int i = 0; i < imagesToUse; i++) {
        pairedImages.push_back(availableImages[i % availableImages.size()]);
    }
    
    // Если нужно больше пар, чем есть уникальных изображений - дублируем
    while ((int)pairedImages.size() < totalPairs) {
        pairedImages.push_back(availableImages[pairedImages.size() % availableImages.size()]);
    }
    
    std::cout << "Используем " << pairedImages.size() << " изображений для пар" << std::endl;
    
    // 5. СОЗДАЕМ КАРТЫ ПАРАМИ
    int cardId = 0;
    for (int i = 0; i < totalPairs; i++) {
        std::string imagePath = pairedImages[i];
        
        // Первая карта пары
        gameCards.emplace_back(cardId++, imagePath, currentTheme);
        // Вторая карта пары (ТА ЖЕ САМАЯ!)
        gameCards.emplace_back(cardId++, imagePath, currentTheme);
        
        std::string filename = fs::path(imagePath).filename().string();
        std::cout << "  Пара #" << (i+1) << ": " << filename 
                  << " (ID: " << (cardId-2) << " и " << (cardId-1) << ")" << std::endl;
    }
    
    // 6. Перемешиваем
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(gameCards.begin(), gameCards.end(), g);
    
    // 7. ПРОВЕРКА
    std::cout << "\n📊 ПРОВЕРКА:" << std::endl;
    std::cout << "Всего карт: " << gameCards.size() << std::endl;
    std::cout << "Должно быть: " << totalCards << std::endl;
    
    if (gameCards.size() == static_cast<size_t>(totalCards)) {
        std::cout << "✅ Размер правильный!" << std::endl;
    } else {
        std::cout << "❌ ОШИБКА: неверное количество карт!" << std::endl;
        // Корректируем
        if (gameCards.size() > static_cast<size_t>(totalCards)) {
            gameCards.resize(totalCards);
        } else {
            while (gameCards.size() < static_cast<size_t>(totalCards)) {
                std::string fallback = pairedImages[gameCards.size() % pairedImages.size()];
                gameCards.emplace_back(cardId++, fallback, currentTheme);
            }
        }
    }
    
    std::cout << "=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА ===\n" << std::endl;
}

void Game::createCardSprites() {
    cards.clear();
    
    // Размеры карточек
    float cardSize = 80.0f;
    float spacing = 10.0f;
    
    // Центрируем игровое поле
    float totalWidth = cols * cardSize + (cols - 1) * spacing;
    float totalHeight = rows * cardSize + (rows - 1) * spacing;
    float startX = (window.getSize().x - totalWidth) / 2;
    float startY = (window.getSize().y - totalHeight) / 2 + 50;
    
    std::cout << "\n=== СОЗДАНИЕ СПРАЙТОВ КАРТ ===" << std::endl;
    std::cout << "Создание " << (rows * cols) << " спрайтов..." << std::endl;
    
    // Создаем спрайты карточек
    for (int i = 0; i < rows * cols && i < static_cast<int>(gameCards.size()); i++) {
        int row = i / cols;
        int col = i % cols;
        
        float x = startX + col * (cardSize + spacing);
        float y = startY + row * (cardSize + spacing);
        
        const Card& cardData = gameCards[i];
        std::string imagePath = cardData.getSymbol();
        
        auto cardSprite = std::make_unique<CardSprite>(
            cardData.getId(), imagePath, x, y, cardSize
        );
        
        // Пытаемся загрузить изображение
        if (!cardSprite->loadImage(imagePath)) {
            std::cout << "⚠ Не удалось загрузить изображение: " << imagePath << std::endl;
            // Если не удалось загрузить, используем текстовый символ
            std::string fallback = "IMG" + std::to_string((i % totalPairs) + 1);
            cardSprite->setSymbol(fallback, mainFont);
        }
        
        cardSprite->setClickable(true);
        cardSprite->hide();
        
        cards.push_back(std::move(cardSprite));
    }
    
    std::cout << "✅ Создано " << cards.size() << " спрайтов карт" << std::endl;
}

void Game::resetGame() {
    std::cout << "\n=== СБРОС ИГРЫ ===" << std::endl;
    
    // Сбрасываем состояние игры
    matchedPairs = 0;
    moves = 0;
    isGameActive = false;
    firstCardSelected = false;
    selectedCard1 = -1;
    selectedCard2 = -1;
    firstCard = nullptr;
    secondCard = nullptr;
    isChecking = false;
    isFlipping = false;
    cardFlipProgress = 0.0f;
    hasWon = false;
    
    std::cout << "matchedPairs сброшен на 0" << std::endl;
    std::cout << "hasWon сброшен на false" << std::endl;
    
    // Очищаем существующие карты
    cards.clear();
    gameCards.clear();
    
    std::cout << "Сбрасываем игрока..." << std::endl;
    if (player) {
        player->startGame();
    }
    
    std::cout << "Инициализируем новые карты..." << std::endl;
    initializeCards();
    
    std::cout << "Создаем спрайты карт..." << std::endl;
    createCardSprites();
    
    // Проверяем результат
    std::cout << "Результат инициализации:" << std::endl;
    std::cout << "  Размер поля: " << rows << "x" << cols << " = " << (rows * cols) << " карт" << std::endl;
    std::cout << "  Создано спрайтов: " << cards.size() << std::endl;
    std::cout << "  Всего пар: " << totalPairs << std::endl;
    
    if (cards.size() == static_cast<size_t>(rows * cols)) {
        std::cout << "✅ Инициализация успешна!" << std::endl;
    } else {
        std::cout << "❌ ОШИБКА: Не все спрайты созданы!" << std::endl;
    }
    
    // Сбрасываем таймер
    gameClock.restart();
    elapsedTime = sf::Time::Zero;
    
    std::cout << "=== СБРОС ЗАВЕРШЕН ===\n" << std::endl;
}

void Game::run() {
    std::cout << "=== НАЧАЛО ИГРОВОГО ЦИКЛА ===" << std::endl;
    sf::Clock clock;
    
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        
        handleEvents();
        update(deltaTime.asSeconds());
        render();
    }
}

void Game::handleEvents() {
    sf::Event event;
    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
    
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        
        if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
            window.setView(sf::View(visibleArea));
            updateBackgrounds();
        }
        
        switch (currentState) {
            case GameState::MAIN_MENU:
                for (auto& button : mainMenuButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::ENTER_NAME:
                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode == '\b') {
                        if (!playerNameInput.empty()) {
                            playerNameInput.pop_back();
                        }
                    } else if (event.text.unicode == '\r') {
                        if (!playerNameInput.empty()) {
                            player = std::make_unique<Player>(playerNameInput);
                            currentState = GameState::SETUP;
                            isEnteringName = false;
                            std::cout << "Игрок создан: " << playerNameInput << std::endl;
                        }
                    } else if (event.text.unicode >= 32 && event.text.unicode < 128) {
                        if (playerNameInput.length() < 20) {
                            playerNameInput += static_cast<char>(event.text.unicode);
                        }
                    }
                }
                break;
                
            case GameState::SETUP:
                for (auto& button : setupButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::PLAYING:
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        for (size_t i = 0; i < cards.size(); i++) {
                            if (cards[i]->contains(mousePos) && 
                                cards[i]->getState() == CardState::HIDDEN &&
                                cards[i]->getIsClickable() &&
                                !isFlipping && !isChecking) {
                                handleCardClick(i);
                                break;
                            }
                        }
                        
                        surrenderButton.handleEvent(event, mousePos);
                    }
                }
                for (auto& button : gameButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::PAUSED:
                for (auto& button : pauseButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::GAME_OVER_WIN:
            case GameState::GAME_OVER_LOSE:
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (mousePos.x >= window.getSize().x / 2 - 150 && 
                            mousePos.x <= window.getSize().x / 2 + 150 &&
                            mousePos.y >= window.getSize().y - 150 && 
                            mousePos.y <= window.getSize().y - 90) {
                            currentState = GameState::MAIN_MENU;
                            background.setTexture(menuBackgroundTexture);
                        }
                    }
                }
                break;
                
            case GameState::LEADERBOARD:
                for (auto& button : leaderboardButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::SETTINGS:
                for (auto& button : settingsButtons) {
                    button.handleEvent(event, mousePos);
                }
                break;
                
            case GameState::CONTACT_FORM:
                contactForm.handleEvent(event, mousePos);
                
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (contactForm.isMouseOverBackButton(mousePos)) {
                            currentState = previousState;
                            background.setTexture(menuBackgroundTexture);
                        }
                    }
                }
                break;
                
            case GameState::EXIT:
                window.close();
                break;
        }
    }
}

void Game::update(float deltaTime) {
    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
    
    switch (currentState) {
        case GameState::MAIN_MENU:
            for (auto& button : mainMenuButtons) button.update(mousePos);
            break;
            
        case GameState::ENTER_NAME:
            break;
            
        case GameState::SETUP:
            for (auto& button : setupButtons) button.update(mousePos);
            break;
            
        case GameState::PLAYING:
            if (isGameActive) {
                elapsedTime = gameClock.getElapsedTime();
                
                int seconds = static_cast<int>(elapsedTime.asSeconds());
                int minutes = seconds / 60;
                seconds %= 60;
                std::stringstream timeSS;
                timeSS << std::setfill('0') << std::setw(2) << minutes << ":"
                       << std::setfill('0') << std::setw(2) << seconds;
                timerText.setString("Time: " + timeSS.str());
                
                updateStats();
            }
            
            for (auto& button : gameButtons) button.update(mousePos);
            surrenderButton.update(mousePos);
            
            if (isFlipping) {
                cardFlipProgress += deltaTime;
                if (cardFlipProgress >= cardFlipTime) {
                    cardFlipProgress = 0.0f;
                    isFlipping = false;
                    
                    if (firstCard && secondCard) {
                        isChecking = true;
                        processCardMatch();
                    }
                }
            }
            break;
            
        case GameState::PAUSED:
            for (auto& button : pauseButtons) button.update(mousePos);
            break;
            
        case GameState::GAME_OVER_WIN:
            break;
            
        case GameState::GAME_OVER_LOSE:
            break;
            
        case GameState::LEADERBOARD:
            for (auto& button : leaderboardButtons) button.update(mousePos);
            break;
            
        case GameState::SETTINGS:
            for (auto& button : settingsButtons) button.update(mousePos);
            break;
            
        case GameState::CONTACT_FORM:
            contactForm.update(mousePos);
            break;
            
        case GameState::EXIT:
            break;
    }
    
    for (auto& card : cards) {
        card->update(deltaTime);
    }
}

void Game::render() {
    window.clear();
    
    if (currentState == GameState::MAIN_MENU || 
        currentState == GameState::SETUP ||
        currentState == GameState::LEADERBOARD ||
        currentState == GameState::ENTER_NAME ||
        currentState == GameState::SETTINGS) {
        background.setTexture(menuBackgroundTexture);
    } else {
        background.setTexture(gameBackgroundTexture);
    }
    
    window.draw(background);
    
    switch (currentState) {
        case GameState::MAIN_MENU:
            renderMainMenu();
            break;
            
        case GameState::ENTER_NAME:
            renderNameInput();
            break;
            
        case GameState::SETUP:
            renderSetupMenu();
            break;
            
        case GameState::PLAYING:
            renderGame();
            break;
            
        case GameState::PAUSED:
            renderPauseMenu();
            break;
            
        case GameState::GAME_OVER_WIN:
            renderGameOverWin();
            break;
            
        case GameState::GAME_OVER_LOSE:
            renderGameOverLose();
            break;
            
        case GameState::LEADERBOARD:
            renderLeaderboard();
            break;
            
        case GameState::SETTINGS:
            renderSettings();
            break;
            
        case GameState::CONTACT_FORM:
            renderContactForm();
            break;
            
        default:
            break;
    }
    
    window.display();
}

void Game::renderContactForm() {
    // Полупрозрачный фон
    sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    contactForm.render(window);
}

void Game::renderSettings() {
    window.draw(settingsTitle);
    
    for (auto& button : settingsButtons) {
        button.render(window);
    }
    
    sf::Text hintText("Changes apply immediately!", mainFont, 20);
    hintText.setFillColor(sf::Color(200, 200, 200));
    hintText.setPosition(window.getSize().x / 2 - 100, 500);
    window.draw(hintText);
}

void Game::renderGameOverWin() {
    std::cout << "=== ОТРИСОВКА ЭКРАНА ПОБЕДЫ ===" << std::endl;
    std::cout << "Статистика: " << matchedPairs << "/" << totalPairs << " пар" << std::endl;
    
    // Поздравление с победой
    sf::Text victoryText("VICTORY!", mainFont, 72);
    victoryText.setFillColor(sf::Color(255, 215, 0));
    victoryText.setStyle(sf::Text::Bold);
    
    sf::FloatRect bounds = victoryText.getLocalBounds();
    victoryText.setOrigin(bounds.left + bounds.width / 2.0f,
                         bounds.top + bounds.height / 2.0f);
    victoryText.setPosition(window.getSize().x / 2, 150);
    window.draw(victoryText);
    
    // Поздравительное сообщение
    sf::Text congratsText("Congratulations!", mainFont, 48);
    congratsText.setFillColor(sf::Color::Green);
    congratsText.setStyle(sf::Text::Bold);
    
    sf::FloatRect congratsBounds = congratsText.getLocalBounds();
    congratsText.setOrigin(congratsBounds.left + congratsBounds.width / 2.0f,
                          congratsBounds.top + congratsBounds.height / 2.0f);
    congratsText.setPosition(window.getSize().x / 2, 250);
    window.draw(congratsText);
    
    // Статистика
    if (player) {
        std::stringstream stats;
        stats << "Player: " << player->getName() << "\n\n";
        stats << "Final Score: " << player->getScore() << "\n";
        stats << "Moves: " << moves << "\n";
        stats << "Perfect Match: " << (moves == totalPairs ? "YES!" : "No") << "\n";
        stats << "Time: " << (int)elapsedTime.asSeconds() << " seconds\n";
        stats << "Difficulty: " << getDifficultyString();
        
        sf::Text statsText(stats.str(), mainFont, 32);
        statsText.setFillColor(sf::Color::White);
        statsText.setPosition(window.getSize().x / 2 - 200, 300);
        window.draw(statsText);
    }
    
    // Кнопка продолжения
    sf::RectangleShape continueButton(sf::Vector2f(300, 60));
    continueButton.setPosition(window.getSize().x / 2 - 150, window.getSize().y - 150);
    
    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
    bool isMouseOverButton = continueButton.getGlobalBounds().contains(mousePos);
    
    if (isMouseOverButton) {
        continueButton.setFillColor(sf::Color(50, 205, 50));
        continueButton.setOutlineColor(sf::Color::Yellow);
    } else {
        continueButton.setFillColor(sf::Color(0, 200, 0));
        continueButton.setOutlineColor(sf::Color::White);
    }
    
    continueButton.setOutlineThickness(2);
    window.draw(continueButton);
    
    sf::Text continueText("Continue to Menu", mainFont, 28);
    continueText.setFillColor(sf::Color::White);
    sf::FloatRect continueBounds = continueText.getLocalBounds();
    continueText.setOrigin(continueBounds.left + continueBounds.width / 2.0f,
                          continueBounds.top + continueBounds.height / 2.0f);
    continueText.setPosition(window.getSize().x / 2, window.getSize().y - 120);
    
    // Тень
    sf::Text shadowText = continueText;
    shadowText.setFillColor(sf::Color(0, 0, 0, 150));
    shadowText.move(2, 2);
    window.draw(shadowText);
    
    window.draw(continueText);
    
    std::cout << "✅ Экран победы отрисован" << std::endl;
}

void Game::renderGameOverLose() {
    // Game Over текст
    sf::Text gameOverText("GAME OVER", mainFont, 72);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);
    
    sf::FloatRect bounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(bounds.left + bounds.width / 2.0f,
                          bounds.top + bounds.height / 2.0f);
    gameOverText.setPosition(window.getSize().x / 2, 200);
    window.draw(gameOverText);
    
    // Сообщение
    sf::Text messageText("Better luck next time!", mainFont, 36);
    messageText.setFillColor(sf::Color(200, 200, 200));
    
    sf::FloatRect messageBounds = messageText.getLocalBounds();
    messageText.setOrigin(messageBounds.left + messageBounds.width / 2.0f,
                         messageBounds.top + messageBounds.height / 2.0f);
    messageText.setPosition(window.getSize().x / 2, 300);
    window.draw(messageText);
    
    // Статистика
    if (player) {
        std::stringstream stats;
        stats << "Player: " << player->getName() << "\n\n";
        stats << "Final Score: " << player->getScore() << "\n";
        stats << "Progress: " << matchedPairs << "/" << totalPairs << " pairs\n";
        stats << "Time: " << (int)elapsedTime.asSeconds() << " seconds\n";
        stats << "Difficulty: " << getDifficultyString();
        
        sf::Text statsText(stats.str(), mainFont, 32);
        statsText.setFillColor(sf::Color::White);
        statsText.setPosition(window.getSize().x / 2 - 200, 350);
        window.draw(statsText);
    }
    
    // Кнопка продолжения
    sf::RectangleShape continueButton(sf::Vector2f(300, 60));
    continueButton.setPosition(window.getSize().x / 2 - 150, window.getSize().y - 150);
    
    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
    bool isMouseOverButton = continueButton.getGlobalBounds().contains(mousePos);
    
    if (isMouseOverButton) {
        continueButton.setFillColor(sf::Color(70, 130, 180));
        continueButton.setOutlineColor(sf::Color::Yellow);
    } else {
        continueButton.setFillColor(sf::Color(50, 100, 150));
        continueButton.setOutlineColor(sf::Color::White);
    }
    
    continueButton.setOutlineThickness(2);
    window.draw(continueButton);
    
    sf::Text continueText("Return to Menu", mainFont, 28);
    continueText.setFillColor(sf::Color::White);
    sf::FloatRect continueBounds = continueText.getLocalBounds();
    continueText.setOrigin(continueBounds.left + continueBounds.width / 2.0f,
                          continueBounds.top + continueBounds.height / 2.0f);
    continueText.setPosition(window.getSize().x / 2, window.getSize().y - 120);
    
    // Тень
    sf::Text shadowText = continueText;
    shadowText.setFillColor(sf::Color(0, 0, 0, 150));
    shadowText.move(2, 2);
    window.draw(shadowText);
    
    window.draw(continueText);
}

void Game::renderGame() {
    // Заголовок и статистика
    window.draw(titleText);
    window.draw(statsText);
    window.draw(timerText);
    window.draw(scoreText);
    window.draw(difficultyText);
    
    // Карточки
    for (auto& card : cards) {
        card->render(window);
    }
    
    // Кнопки
    for (auto& button : gameButtons) {
        button.render(window);
    }
    
    surrenderButton.render(window);
}

void Game::renderMainMenu() {
    window.draw(titleText);
    
    for (auto& button : mainMenuButtons) {
        button.render(window);
    }
}

void Game::renderPauseMenu() {
    // Полупрозрачный фон
    sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);
    
    sf::Text pauseText("PAUSED", mainFont, 72);
    pauseText.setFillColor(sf::Color::Yellow);
    pauseText.setStyle(sf::Text::Bold);
    sf::FloatRect pauseBounds = pauseText.getLocalBounds();
    pauseText.setOrigin(pauseBounds.left + pauseBounds.width / 2.0f,
                        pauseBounds.top + pauseBounds.height / 2.0f);
    pauseText.setPosition(window.getSize().x / 2, 200);
    window.draw(pauseText);
    
    for (auto& button : pauseButtons) {
        button.render(window);
    }
}

void Game::renderSetupMenu() {
    // Заголовок
    sf::Text setupTitle("Game Setup", mainFont, 48);
    setupTitle.setFillColor(sf::Color::White);
    setupTitle.setStyle(sf::Text::Bold);
    setupTitle.setPosition(window.getSize().x / 2 - 100, 100);
    window.draw(setupTitle);
    
    // Информация о текущих настройках
    std::stringstream settingsInfo;
    settingsInfo << "Current settings:\n";
    settingsInfo << "• Player: " << (player ? player->getName() : "Not set") << "\n";
    settingsInfo << "• Difficulty: " << getDifficultyString() << "\n";
    settingsInfo << "• Theme: ";
    switch (currentTheme) {
        case CardTheme::ANIMALS: settingsInfo << "Animals"; break;
        case CardTheme::FRUITS: settingsInfo << "Fruits"; break;
        case CardTheme::EMOJI: settingsInfo << "Emoji"; break;
        case CardTheme::MEMES: settingsInfo << "Memes"; break;
        case CardTheme::SYMBOLS: settingsInfo << "Symbols"; break;
    }
    
    sf::Text infoText(settingsInfo.str(), mainFont, 24);
    infoText.setFillColor(sf::Color(200, 200, 200));
    infoText.setPosition(window.getSize().x / 2 - 200, 150);
    window.draw(infoText);
    
    // Кнопки
    for (auto& button : setupButtons) button.render(window);
}

void Game::renderLeaderboard() {
    // Заголовок
    sf::Text title("Leaderboard", mainFont, 64);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(window.getSize().x / 2 - 150, 80);
    window.draw(title);
    
    // Получаем лучшие результаты
    auto topPlayers = database ? database->getTopScores(10) : std::vector<GameRecord>();
    
    if (topPlayers.empty()) {
        sf::Text noData("No records in leaderboard yet", mainFont, 32);
        noData.setFillColor(sf::Color(200, 200, 200));
        noData.setPosition(window.getSize().x / 2 - 150, 200);
        window.draw(noData);
    } else {
        // Заголовок таблицы
        sf::Text header("#  Player              Score   Time   Difficulty", mainFont, 28);
        header.setFillColor(sf::Color::Yellow);
        header.setPosition(150, 180);
        window.draw(header);
        
        // Список
        float yPos = 230;
        int rank = 1;
        
        for (const auto& record : topPlayers) {
            std::stringstream line;
            line << std::setw(2) << std::right << rank << ". ";
            line << std::setw(15) << std::left << record.playerName.substr(0, 15) << " ";
            line << std::setw(6) << std::right << record.score << " ";
            line << std::setw(4) << std::right << (int)record.time << "s ";
            line << record.difficulty;
            
            sf::Text playerText(line.str(), mainFont, 24);
            
            if (rank == 1) playerText.setFillColor(sf::Color(255, 215, 0));
            else if (rank == 2) playerText.setFillColor(sf::Color(192, 192, 192));
            else if (rank == 3) playerText.setFillColor(sf::Color(205, 127, 50));
            else playerText.setFillColor(sf::Color::White);
            
            playerText.setPosition(150, yPos);
            window.draw(playerText);
            
            yPos += 40;
            rank++;
            if (rank > 10) break;
        }
    }
    
    // Кнопки
    for (auto& button : leaderboardButtons) button.render(window);
}

void Game::renderNameInput() {
    // Заголовок
    sf::Text title;
    title.setFont(mainFont);
    title.setString("Enter your name:");
    title.setCharacterSize(48);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                    titleBounds.top + titleBounds.height / 2.0f);
    title.setPosition(window.getSize().x / 2, 200);
    window.draw(title);
    
    // Поле ввода
    nameInputBox.setPosition(window.getSize().x / 2 - 200, 300);
    window.draw(nameInputBox);
    
    // Текст ввода
    nameInputText.setString(playerNameInput + "_");
    nameInputText.setPosition(window.getSize().x / 2 - 180, 315);
    window.draw(nameInputText);
    
    // Подсказка
    sf::Text hint;
    hint.setFont(mainFont);
    hint.setString("Press Enter to continue");
    hint.setCharacterSize(24);
    hint.setFillColor(sf::Color(200, 200, 200));
    
    sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setOrigin(hintBounds.left + hintBounds.width / 2.0f,
                   hintBounds.top + hintBounds.height / 2.0f);
    hint.setPosition(window.getSize().x / 2, 400);
    window.draw(hint);
}

void Game::updateStats() {
    std::stringstream statsSS;
    statsSS << "Player: " << (player ? player->getName() : "Guest") << "\n"
            << "Difficulty: " << getDifficultyString() << "\n"
            << "Field: " << rows << "x" << cols << " (" << (rows * cols) << " cards)\n"
            << "Moves: " << moves << "\n"
            << "Pairs found: " << matchedPairs << "/" << totalPairs << "\n"
            << "Progress: " << std::fixed << std::setprecision(1) 
            << (totalPairs > 0 ? (matchedPairs * 100.0 / totalPairs) : 0) << "%";
    
    statsText.setString(statsSS.str());
    
    if (player) {
        player->calculateScore(totalPairs);
        scoreText.setString("Score: " + std::to_string(player->getScore()));
    }
    
    difficultyText.setString("Difficulty: " + getDifficultyString());
    difficultyText.setFillColor(getDifficultyColor());
}

sf::Color Game::getDifficultyColor() const {
    switch (difficulty) {
        case Difficulty::EASY:
            return sf::Color::Green;
        case Difficulty::MEDIUM:
            return sf::Color::Yellow;
        case Difficulty::HARD:
            return sf::Color(255, 165, 0);
        case Difficulty::EXPERT:
            return sf::Color::Red;
        default:
            return sf::Color::White;
    }
}

void Game::handleCardClick(int cardIndex) {
    if (!isGameActive || isFlipping || isChecking) {
        return;
    }
    
    if (cardIndex < 0 || cardIndex >= static_cast<int>(cards.size())) {
        return;
    }
    
    if (!cards[cardIndex]) {
        return;
    }
    
    if (cards[cardIndex]->getState() == CardState::HIDDEN && !cards[cardIndex]->getIsClickable()) {
        return;
    }
    
    // Звук переворота
    if (soundManager) {
        soundManager->playCardFlip();
    }
    
    // Переворачиваем карту
    cards[cardIndex]->reveal();
    cards[cardIndex]->setClickable(false);
    
    if (!firstCardSelected) {
        // Первая карта
        firstCardSelected = true;
        selectedCard1 = cardIndex;
        firstCard = cards[cardIndex].get();
        isFlipping = true;
        cardFlipProgress = 0.0f;
    } else {
        // Вторая карта
        selectedCard2 = cardIndex;
        secondCard = cards[cardIndex].get();
        isFlipping = true;
        cardFlipProgress = 0.0f;
        
        // Увеличиваем счетчик ходов
        moves++;
        if (player) {
            player->incrementMoves();
        }
        
        firstCardSelected = false;
    }
}

void Game::processCardMatch() {
    std::cout << "=== ПРОВЕРКА СОВПАДЕНИЯ КАРТ ===" << std::endl;
    std::cout << "Найдено пар: " << matchedPairs << "/" << totalPairs << std::endl;
    
    if (!firstCard || !secondCard || !isChecking) {
        std::cout << "Ошибка: карты не инициализированы" << std::endl;
        return;
    }
    
    // Проверяем совпадение
    bool match = (firstCard->getSymbol() == secondCard->getSymbol());
    std::cout << "Символ 1: '" << firstCard->getSymbol() << "'" << std::endl;
    std::cout << "Символ 2: '" << secondCard->getSymbol() << "'" << std::endl;
    std::cout << "Совпадение: " << (match ? "ДА" : "НЕТ") << std::endl;
    
    if (match) {
        // Совпадение
        if (soundManager) {
            soundManager->playCardMatch();
        }
        
        // Помечаем как совпавшие
        firstCard->markMatched();
        secondCard->markMatched();
        
        // Увеличиваем счетчик совпавших пар
        matchedPairs++;
        std::cout << "✅ НОВАЯ ПАРА НАЙДЕНА! Всего: " << matchedPairs << "/" << totalPairs << std::endl;
        
        if (player) {
            player->incrementMatchedPairs();
        }
        
        // Обновляем счет
        if (player) {
            player->calculateScore(totalPairs);
        }
        
        // ===== ПРОВЕРКА ПОБЕДЫ =====
        if (matchedPairs >= totalPairs && !hasWon) {
            std::cout << "🎉🎉🎉 ПОБЕДА! ВСЕ ПАРЫ НАЙДЕНЫ! 🎉🎉🎉" << std::endl;
            std::cout << "Условие: " << matchedPairs << " >= " << totalPairs << std::endl;
            
            hasWon = true;
            isGameActive = false;
            
            if (player) {
                player->finishGame();
                player->calculateScore(totalPairs);
                saveGameResult();
            }
            
            if (soundManager) {
                soundManager->playGameWin();
            }
            
            currentState = GameState::GAME_OVER_WIN;
            std::cout << "Состояние изменено на GAME_OVER_WIN" << std::endl;
            return;
        }
    } else {
        // Несовпадение
        if (soundManager) {
            soundManager->playCardMismatch();
        }
        
        isChecking = false;
        
        // Небольшая задержка
        sf::Clock delayClock;
        while (delayClock.getElapsedTime().asSeconds() < 0.8f) {
            // Ждем
        }
        
        firstCard->hide();
        secondCard->hide();
        firstCard->setClickable(true);
        secondCard->setClickable(true);
        std::cout << "❌ Карты не совпали, переворачиваем обратно" << std::endl;
    }
    
    // Сбрасываем выбор только если не победили
    if (currentState != GameState::GAME_OVER_WIN) {
        firstCard = nullptr;
        secondCard = nullptr;
        selectedCard1 = -1;
        selectedCard2 = -1;
        isChecking = false;
    }
    
    std::cout << "=== ПРОВЕРКА ЗАВЕРШЕНА ===\n" << std::endl;
}

void Game::saveGameResult() {
    if (!player || !database) {
        return;
    }
    
    GameRecord record;
    record.playerName = player->getName();
    record.score = player->getScore();
    record.moves = moves;
    record.pairs = matchedPairs;
    record.time = elapsedTime.asSeconds();
    record.date = getCurrentDate();
    record.difficulty = getDifficultyString();
    
    database->saveGame(record);
    std::cout << "💾 Результат сохранен в БД" << std::endl;
}

std::string Game::getCurrentDate() const {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buffer);
}

std::string Game::getDifficultyString() const {
    switch (difficulty) {
        case Difficulty::EASY: return "Easy";
        case Difficulty::MEDIUM: return "Medium";
        case Difficulty::HARD: return "Hard";
        case Difficulty::EXPERT: return "Expert";
        default: return "Unknown";
    }
}

void Game::startNewGame() {
    std::cout << "\n=== НАЧАЛО НОВОЙ ИГРЫ ===" << std::endl;
    currentState = GameState::ENTER_NAME;
    playerNameInput = "";
    isEnteringName = true;
    hasWon = false;
}

void Game::pauseGame() {
    if (currentState == GameState::PLAYING) {
        currentState = GameState::PAUSED;
        isGameActive = false;
    }
}

void Game::resumeGame() {
    if (currentState == GameState::PAUSED) {
        currentState = GameState::PLAYING;
        isGameActive = true;
        gameClock.restart();
    }
}

void Game::showLeaderboard() {
    currentState = GameState::LEADERBOARD;
}

void Game::showSettings() {
    currentState = GameState::SETTINGS;
}

void Game::exitGame() {
    window.close();
}

void Game::surrenderGame() {
    if (!isGameActive) return;
    
    std::cout << "Игрок сдался!" << std::endl;
    
    if (soundManager) {
        soundManager->playGameLose();
    }
    
    isGameActive = false;
    
    if (player) {
        player->finishGame();
        player->calculateScore(totalPairs);
        
        GameRecord record;
        record.playerName = player->getName();
        record.score = player->getScore() / 2;
        record.moves = moves;
        record.pairs = matchedPairs;
        record.time = elapsedTime.asSeconds();
        record.date = getCurrentDate();
        record.difficulty = getDifficultyString();
        
        if (database) {
            database->saveGame(record);
        }
    }
    
    currentState = GameState::GAME_OVER_LOSE;
}

void Game::setDifficulty(Difficulty diff) {
    difficulty = diff;
}

void Game::setTheme(CardTheme theme) {
    currentTheme = theme;
}
