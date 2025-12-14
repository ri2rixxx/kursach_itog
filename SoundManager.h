#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <SFML/Audio.hpp>
#include <map>
#include <string>

class SoundManager {
private:
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::map<std::string, sf::Sound> sounds;
    float volume;
    bool soundEnabled;
    
    void createEmptySound(const std::string& name);
    void createProgrammaticSound(const std::string& name, float frequency, float duration);
    void createFallbackSound(const std::string& name); // ← ДОБАВЬТЕ ЭТУ СТРОКУ
    
public:
    SoundManager();
    ~SoundManager();
    
    bool loadSound(const std::string& name, const std::string& filepath);
    void playSound(const std::string& name);
    void stopSound(const std::string& name);
    void setVolume(float volume);
    float getVolume() const;
    
    void enableSound(bool enable);
    bool isSoundEnabled() const;
    
    void playCardFlip();
    void playCardMatch();
    void playCardMismatch();
    void playButtonClick();
    void playGameWin();
    void playGameLose();
    
    bool isSoundLoaded(const std::string& name) const;
};

#endif
