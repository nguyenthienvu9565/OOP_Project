#include "tool.h"
#include "tool_definition.h"
#include <sqlite3.h>
#include <string>
#include <iostream>
#include <sstream>

// ==========================================
// TRIỂN KHAI SQLiteManager
// ==========================================

SQLiteManager::SQLiteManager(std::string_view databasePath) : dbPath(databasePath) {
    if (openDatabase()) {
        createTable();
    }
}

SQLiteManager::~SQLiteManager() {
    closeDatabase();
}

bool SQLiteManager::openDatabase() {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

void SQLiteManager::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool SQLiteManager::createTable() {
    if (!db) return false;
    
    const char* sql = "CREATE TABLE IF NOT EXISTS memory ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "content TEXT NOT NULL,"
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error during table creation: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}


// ==========================================
// TRIỂN KHAI MemorySaveTool
// ==========================================

MemorySaveTool::MemorySaveTool(std::string_view databasePath)
    : Tool("memory_save", "Saves an important piece of information or fact into the agent's long-term memory. Input: exact text content to remember."),
      SQLiteManager(databasePath) {}

std::string MemorySaveTool::execute(const std::string& arguments) {
    if (!db) {
        return "Error: Database connection is not available.";
    }
    if (arguments.empty()) {
        return "Error: Cannot save empty memory.";
    }

    const char* sql = "INSERT INTO memory (content) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "Error: Failed to prepare statement: " + std::string(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, arguments.c_str(), -1, SQLITE_STATIC);

    std::string result;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = "Success: Information successfully saved to long-term memory.";
    } else {
        result = "Error: Failed to execute statement: " + std::string(sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}


// ==========================================
// TRIỂN KHAI MemorySearchTool
// ==========================================

MemorySearchTool::MemorySearchTool(std::string_view databasePath)
    : Tool("memory_search", "Searches the agent's long-term memory for past facts or information using keyword query. Input: search keyword/phrase."),
      SQLiteManager(databasePath) {}

std::string MemorySearchTool::execute(const std::string& arguments) {
    if (!db) {
        return "Error: Database connection is not available.";
    }
    if (arguments.empty()) {
        return "Error: Search query cannot be empty.";
    }
    const char* sql = "SELECT content, timestamp FROM memory WHERE content LIKE ? ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "Error: Failed to prepare search statement: " + std::string(sqlite3_errmsg(db));
    }

    std::string searchPattern = "%" + arguments + "%";
    sqlite3_bind_text(stmt, 1, searchPattern.c_str(), -1, SQLITE_STATIC);

    std::ostringstream oss;
    oss << "Search results for memory query '" << arguments << "':\n";
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
        const unsigned char* content = sqlite3_column_text(stmt, 0);
        const unsigned char* timestamp = sqlite3_column_text(stmt, 1);
        
        oss << count << ". [" << (timestamp ? reinterpret_cast<const char*>(timestamp) : "Unknown") << "] "
            << (content ? reinterpret_cast<const char*>(content) : "") << "\n";
    }

    sqlite3_finalize(stmt);

    if (count == 0) {
        return "No relevant memories found for: '" + arguments + "'";
    }

    return oss.str();
}