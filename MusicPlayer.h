#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <SFML/Audio.hpp>
#include <map>
#include <string>

enum class MusicTheme {
    MENU,
    GAMEPLAY_EASY,
    GAMEPLAY_MEDIUM,
    GAMEPLAY_HARD,
    GAME_OVER
};

class MusicPlayer {
private:
    sf::Music currentMusic;
    std::map<MusicTheme, std::string> musicFiles;
    float volume;
    bool isPlaying;
    
public:
    MusicPlayer();
    ~MusicPlayer();
    
    bool loadMusic(MusicTheme theme, const std::string& filepath);
    void play(MusicTheme theme);
    void pause();
    void resume();
    void stop();
    void setVolume(float volume);
    float getVolume() const;
    bool getIsPlaying() const;
    
    void setLoop(bool loop);
};

#endif
