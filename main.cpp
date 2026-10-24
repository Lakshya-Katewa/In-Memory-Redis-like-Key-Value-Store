#include <iostream>
#include <string>

#include "CommandParser.h"
#include "Database.h"

int main(int argc, char* argv[]) {
    const std::string dbFile = (argc > 1) ? argv[1] : "data/dump.db";

    Database db(dbFile);
    CommandParser parser;

    if (db.load()) {
        std::cout << "Loaded " << db.size() << " key(s) from " << dbFile << "\n";
    }
    std::cout << "In-Memory Key-Value Store. Type HELP for commands.\n";

    std::string line;
    bool shouldExit = false;
    while (!shouldExit) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;  // EOF (Ctrl+D / Ctrl+Z)

        Command cmd = parser.parse(line);
        std::string result = parser.execute(cmd, db, shouldExit);
        if (!result.empty()) std::cout << result << "\n";
    }

    if (db.save()) {
        std::cout << "Saved to " << dbFile << "\n";
    } else {
        std::cerr << "Warning: could not save to " << dbFile << "\n";
        return 1;
    }
    return 0;
}
