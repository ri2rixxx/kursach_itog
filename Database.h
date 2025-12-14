#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>

struct GameRecord {
    int id;
    std::string playerName;
    int score;
    int moves;
    int pairs;
    double time;
    std::string date;
    std::string difficulty;
};

class Database {
private:
    sqlite3* db;
    std::string dbPath;
    
    bool executeQuery(const std::string& query);

public:
    Database(const std::string& dbPath = "memory_game.db");
    ~Database();
    
    bool initialize();
    bool saveGame(const GameRecord& record);
    std::vector<GameRecord> getTopScores(int limit = 10);
    std::vector<GameRecord> getPlayerHistory(const std::string& playerName);
    void displayLeaderboard();
    
    // Метод для совместимости с Game.cpp
    std::vector<GameRecord> getTopPlayers(int limit = 10) {
        return getTopScores(limit);
    }
};

#endif
