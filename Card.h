#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>

enum class CardTheme {
    ANIMALS,
    FRUITS,
    EMOJI,
    MEMES,
    SYMBOLS
};

class Card {
private:
    int id;
    std::string symbol;
    bool isFlipped;
    bool isMatched;
    CardTheme theme;
    bool isImage;

public:
    Card();
    Card(int id, const std::string& symbol, CardTheme theme);
    
    int getId() const { return id; }
    std::string getSymbol() const { return symbol; }
    bool getIsFlipped() const { return isFlipped; }
    bool getIsMatched() const { return isMatched; }
    CardTheme getTheme() const { return theme; }
    bool getIsImage() const { return isImage; }
    
    void flip();
    void setMatched(bool matched);
    void reset();
    
    std::string getDisplay() const;
    
    static std::vector<std::string> getThemeSymbols(CardTheme theme);
};

#endif
