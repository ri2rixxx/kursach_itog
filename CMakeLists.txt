cmake_minimum_required(VERSION 3.10)
project(memory_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Явно указываем компилятору флаги
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Find SFML with explicit components
find_package(SFML 2.5 COMPONENTS system window graphics audio REQUIRED)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Add your executable
add_executable(memory_game
    src/main.cpp
    src/Game.cpp
    src/Card.cpp
    src/Player.cpp
    src/Database.cpp
    src/GUI/Button.cpp
    src/GUI/CardSprite.cpp
    src/GUI/Menu.cpp
    src/Audio/SoundManager.cpp
    src/Audio/MusicPlayer.cpp
    src/ContactForm.cpp
    src/EmailSender.cpp
)

# Include directories
target_include_directories(memory_game PRIVATE include)

# Link libraries - ВАЖНО: линкуем каждый компонент отдельно
target_link_libraries(memory_game
    sfml-system
    sfml-window
    sfml-graphics
    sfml-audio
    ${SQLite3_LIBRARIES}
    pthread
)

# Альтернативный вариант (если выше не работает):
# target_link_libraries(memory_game
#     SFML::System
#     SFML::Window
#     SFML::Graphics
#     SFML::Audio
#     ${SQLite3_LIBRARIES}
#     pthread
# )
