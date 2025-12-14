#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <chrono>

class Player {
private:
    std::string name;
    int score;
    int moves;
    int matchedPairs;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    bool gameFinished;

public:
    Player(const std::string& name);
    
    std::string getName() const { return name; }
    int getScore() const { return score; }
    int getMoves() const { return moves; }
    int getMatchedPairs() const { return matchedPairs; }
    
    void incrementMoves();
    void incrementMatchedPairs();
    void calculateScore(int totalPairs);
    void startGame();
    void finishGame();
    double getElapsedTime() const;
    
    void displayStats() const;
};

#endif
