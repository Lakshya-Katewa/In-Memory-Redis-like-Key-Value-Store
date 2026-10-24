#include "CommandParser.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <vector>

namespace {

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// Strict positive-integer parse.
bool parsePositive(const std::string& s, long long& out) {
    try {
        std::size_t used = 0;
        long long v = std::stoll(s, &used);
        if (used != s.size() || v <= 0) return false;
        out = v;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

Command invalid(const std::string& msg) {
    Command c;
    c.type = Command::Type::Invalid;
    c.error = msg;
    return c;
}

}  // namespace

Command CommandParser::parse(const std::string& line) const {
    auto tokens = tokenize(line);
    Command cmd;
    if (tokens.empty()) return cmd;  // Type::Empty

    const std::string name = toUpper(tokens[0]);

    if (name == "SET") {
        if (tokens.size() < 3) return invalid("usage: SET key value [EX seconds]");
        cmd.type = Command::Type::Set;
        cmd.key = tokens[1];

        std::size_t valueEnd = tokens.size();
        // Optional trailing "EX <seconds>"
        if (tokens.size() >= 5 && toUpper(tokens[tokens.size() - 2]) == "EX") {
            long long secs = 0;
            if (!parsePositive(tokens.back(), secs))
                return invalid("EX requires a positive integer number of seconds");
            cmd.seconds = secs;
            cmd.hasTTL = true;
            valueEnd = tokens.size() - 2;
        }

        std::string value;
        for (std::size_t i = 2; i < valueEnd; ++i) {
            if (i > 2) value += ' ';
            value += tokens[i];
        }
        cmd.value = value;
        return cmd;
    }

    if (name == "GET" || name == "EXISTS" || name == "TTL" ||
        name == "DELETE" || name == "DEL") {
        if (tokens.size() != 2) return invalid("usage: " + name + " key");
        cmd.key = tokens[1];
        if (name == "GET") cmd.type = Command::Type::Get;
        else if (name == "EXISTS") cmd.type = Command::Type::Exists;
        else if (name == "TTL") cmd.type = Command::Type::Ttl;
        else cmd.type = Command::Type::Delete;
        return cmd;
    }

    if (name == "EXPIRE") {
        if (tokens.size() != 3) return invalid("usage: EXPIRE key seconds");
        long long secs = 0;
        if (!parsePositive(tokens[2], secs))
            return invalid("seconds must be a positive integer");
        cmd.type = Command::Type::Expire;
        cmd.key = tokens[1];
        cmd.seconds = secs;
        cmd.hasTTL = true;
        return cmd;
    }

    if (name == "KEYS" || name == "SIZE" || name == "SAVE" || name == "HELP" ||
        name == "EXIT" || name == "QUIT") {
        if (tokens.size() != 1) return invalid("usage: " + name);
        if (name == "KEYS") cmd.type = Command::Type::Keys;
        else if (name == "SIZE") cmd.type = Command::Type::Size;
        else if (name == "SAVE") cmd.type = Command::Type::Save;
        else if (name == "HELP") cmd.type = Command::Type::Help;
        else cmd.type = Command::Type::Exit;
        return cmd;
    }

    return invalid("unknown command '" + tokens[0] + "' (type HELP)");
}

std::string CommandParser::execute(const Command& cmd, Database& db, bool& shouldExit) const {
    using Type = Command::Type;
    shouldExit = false;

    switch (cmd.type) {
        case Type::Empty:
            return "";
        case Type::Invalid:
            return "ERROR: " + cmd.error;
        case Type::Set:
            if (cmd.hasTTL) db.set(cmd.key, cmd.value, std::chrono::seconds(cmd.seconds));
            else db.set(cmd.key, cmd.value);
            return "OK";
        case Type::Get: {
            auto v = db.get(cmd.key);
            return v ? *v : "NULL";
        }
        case Type::Delete:
            return db.del(cmd.key) ? "OK" : "NULL";
        case Type::Exists:
            return db.exists(cmd.key) ? "1" : "0";
        case Type::Expire:
            return db.expire(cmd.key, std::chrono::seconds(cmd.seconds)) ? "1" : "0";
        case Type::Ttl:
            return std::to_string(db.ttl(cmd.key));
        case Type::Keys: {
            auto keys = db.keys();
            if (keys.empty()) return "(empty)";
            std::sort(keys.begin(), keys.end());
            std::string out;
            for (std::size_t i = 0; i < keys.size(); ++i) {
                if (i) out += '\n';
                out += keys[i];
            }
            return out;
        }
        case Type::Size:
            return std::to_string(db.size());
        case Type::Save:
            return db.save() ? "OK" : "ERROR: could not write " + db.filePath();
        case Type::Help:
            return helpText();
        case Type::Exit:
            shouldExit = true;
            return "";
    }
    return "";
}

std::string CommandParser::helpText() {
    return
        "Commands:\n"
        "  SET key value [EX seconds]   store a value, optionally with a TTL\n"
        "  GET key                      read a value (NULL if missing/expired)\n"
        "  DELETE key                   remove a key (alias: DEL)\n"
        "  EXISTS key                   1 if the key exists, else 0\n"
        "  EXPIRE key seconds           set a TTL on an existing key\n"
        "  TTL key                      seconds left (-1 no expiry, -2 missing)\n"
        "  KEYS                         list all live keys\n"
        "  SIZE                         number of live keys\n"
        "  SAVE                         write the database to disk now\n"
        "  HELP                         show this message\n"
        "  EXIT                         save and quit (alias: QUIT)";
}
