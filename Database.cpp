#include "Database.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

Database::Database(const std::string& dbPath) : db(nullptr), dbPath(dbPath) {
    std::cout << "📁 Конструктор Database: " << dbPath << std::endl;
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::initialize() {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ Не удалось открыть БД: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    std::cout << "✅ БД открыта: " << dbPath << std::endl;
    
    // Создаем таблицу
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS games ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "player_name TEXT NOT NULL,"
        "score INTEGER NOT NULL,"
        "moves INTEGER NOT NULL,"
        "pairs INTEGER NOT NULL,"
        "time REAL NOT NULL,"
        "date TEXT NOT NULL,"
        "difficulty TEXT NOT NULL"
        ");";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ Ошибка SQL: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    std::cout << "✅ Таблица создана/проверена" << std::endl;
    return true;
}

bool Database::executeQuery(const std::string& query) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ SQL ошибка: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::saveGame(const GameRecord& record) {
    std::string query = "INSERT INTO games (player_name, score, moves, pairs, time, date, difficulty) VALUES (?, ?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // Привязываем параметры
    sqlite3_bind_text(stmt, 1, record.playerName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, record.score);
    sqlite3_bind_int(stmt, 3, record.moves);
    sqlite3_bind_int(stmt, 4, record.pairs);
    sqlite3_bind_double(stmt, 5, record.time);
    sqlite3_bind_text(stmt, 6, record.date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, record.difficulty.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    
    bool success = (rc == SQLITE_DONE);
    
    if (success) {
        std::cout << "💾 Результат сохранен в БД: " << record.playerName 
                  << " - " << record.score << " очков" << std::endl;
    } else {
        std::cerr << "❌ Ошибка сохранения: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_finalize(stmt);
    
    return success;
}

std::vector<GameRecord> Database::getTopScores(int limit) {
    std::vector<GameRecord> records;
    
    std::string query = "SELECT * FROM games ORDER BY score DESC LIMIT " + std::to_string(limit) + ";";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return records;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GameRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.playerName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.score = sqlite3_column_int(stmt, 2);
        record.moves = sqlite3_column_int(stmt, 3);
        record.pairs = sqlite3_column_int(stmt, 4);
        record.time = sqlite3_column_double(stmt, 5);
        record.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}

void Database::displayLeaderboard() {
    auto records = getTopScores(10);
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    🏆 ТАБЛИЦА ЛИДЕРОВ 🏆                      ║\n";
    std::cout << "╠════╦═══════════════╦═══════╦═══════╦═══════╦═════════════════╣\n";
    std::cout << "║ №  ║ Игрок         ║ Очки  ║ Ходы  ║ Время ║ Сложность       ║\n";
    std::cout << "╠════╬═══════════════╬═══════╬═══════╬═══════╬═════════════════╣\n";
    
    for (size_t i = 0; i < records.size(); i++) {
        std::cout << "║ " << std::setw(2) << (i + 1) << " ║ "
                  << std::setw(13) << std::left << records[i].playerName.substr(0, 13) << " ║ "
                  << std::setw(5) << std::right << records[i].score << " ║ "
                  << std::setw(5) << records[i].moves << " ║ "
                  << std::setw(5) << (int)records[i].time << " ║ "
                  << std::setw(15) << std::left << records[i].difficulty << " ║\n";
    }
    
    std::cout << "╚════╩═══════════════╩═══════╩═══════╩═══════╩═════════════════╝\n";
}

std::vector<GameRecord> Database::getPlayerHistory(const std::string& playerName) {
    std::vector<GameRecord> records;
    
    std::string query = "SELECT * FROM games WHERE player_name = ? ORDER BY score DESC LIMIT 10;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "❌ Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return records;
    }
    
    sqlite3_bind_text(stmt, 1, playerName.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GameRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.playerName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.score = sqlite3_column_int(stmt, 2);
        record.moves = sqlite3_column_int(stmt, 3);
        record.pairs = sqlite3_column_int(stmt, 4);
        record.time = sqlite3_column_double(stmt, 5);
        record.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.difficulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}
