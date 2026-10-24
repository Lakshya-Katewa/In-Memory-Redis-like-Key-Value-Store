#include "Database.h"

#include <filesystem>
#include <fstream>
#include <utility>

namespace {

// Escape characters that would break the line-based file format.
std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: out += s[i];  // handles "\\\\"
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

const char* kHeader = "KVSTORE 1";

}  // namespace

Database::Database(std::string filePath) : filePath_(std::move(filePath)) {}

bool Database::evictIfExpired(const std::string& key) {
    if (expiry_.isExpired(key)) {
        storage_.erase(key);
        expiry_.clearExpiry(key);
        return true;
    }
    return false;
}

void Database::set(const std::string& key, const std::string& value,
                   std::optional<std::chrono::seconds> ttl) {
    storage_.set(key, value);
    if (ttl) {
        expiry_.setExpiry(key, *ttl);
    } else {
        expiry_.clearExpiry(key);
    }
}

std::optional<std::string> Database::get(const std::string& key) {
    if (evictIfExpired(key)) return std::nullopt;
    return storage_.get(key);
}

bool Database::del(const std::string& key) {
    bool wasExpired = evictIfExpired(key);
    if (wasExpired) return false;
    expiry_.clearExpiry(key);
    return storage_.erase(key);
}

bool Database::exists(const std::string& key) {
    if (evictIfExpired(key)) return false;
    return storage_.contains(key);
}

bool Database::expire(const std::string& key, std::chrono::seconds ttl) {
    if (!exists(key)) return false;
    expiry_.setExpiry(key, ttl);
    return true;
}

long long Database::ttl(const std::string& key) {
    if (!exists(key)) return -2;
    return expiry_.remainingSeconds(key);
}

std::vector<std::string> Database::keys() {
    purgeExpired();
    return storage_.keys();
}

std::size_t Database::size() {
    purgeExpired();
    return storage_.size();
}

std::size_t Database::purgeExpired() {
    auto expired = expiry_.expiredKeys();
    for (const auto& key : expired) {
        storage_.erase(key);
        expiry_.clearExpiry(key);
    }
    return expired.size();
}

bool Database::save() const {
    namespace fs = std::filesystem;
    try {
        fs::path target(filePath_);
        if (target.has_parent_path()) fs::create_directories(target.parent_path());

        fs::path tmp = target;
        tmp += ".tmp";

        {
            std::ofstream out(tmp, std::ios::trunc);
            if (!out) return false;

            const auto now = ExpiryManager::Clock::now();
            out << kHeader << '\n';
            for (const auto& [key, value] : storage_.items()) {
                long long expiryMs = 0;  // 0 = no expiry
                auto it = expiry_.all().find(key);
                if (it != expiry_.all().end()) {
                    if (it->second <= now) continue;  // already expired, skip
                    expiryMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   it->second.time_since_epoch()).count();
                }
                out << expiryMs << '\t' << escape(key) << '\t' << escape(value) << '\n';
            }
            out.flush();
            if (!out) return false;
        }

        fs::rename(tmp, target);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool Database::load() {
    std::ifstream in(filePath_);
    if (!in) return false;  // no file yet: start empty

    std::string line;
    if (!std::getline(in, line) || line != kHeader) return false;

    storage_.clear();
    expiry_.clear();

    const auto now = ExpiryManager::Clock::now();
    while (std::getline(in, line)) {
        auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;

        long long expiryMs = 0;
        try {
            expiryMs = std::stoll(line.substr(0, t1));
        } catch (const std::exception&) {
            continue;  // skip corrupt line
        }

        std::string key = unescape(line.substr(t1 + 1, t2 - t1 - 1));
        std::string value = unescape(line.substr(t2 + 1));

        if (expiryMs != 0) {
            ExpiryManager::TimePoint when{std::chrono::milliseconds(expiryMs)};
            if (when <= now) continue;  // expired while the app was down
            expiry_.setExpiryAt(key, when);
        }
        storage_.set(key, value);
    }
    return true;
}
