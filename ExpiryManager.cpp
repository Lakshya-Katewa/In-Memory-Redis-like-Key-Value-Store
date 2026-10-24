#include "ExpiryManager.h"

void ExpiryManager::setExpiry(const std::string& key, std::chrono::seconds ttl) {
    expiries_[key] = Clock::now() + ttl;
}

void ExpiryManager::setExpiryAt(const std::string& key, TimePoint when) {
    expiries_[key] = when;
}

bool ExpiryManager::hasExpiry(const std::string& key) const {
    return expiries_.find(key) != expiries_.end();
}

bool ExpiryManager::isExpired(const std::string& key) const {
    auto it = expiries_.find(key);
    return it != expiries_.end() && Clock::now() >= it->second;
}

void ExpiryManager::clearExpiry(const std::string& key) {
    expiries_.erase(key);
}

long long ExpiryManager::remainingSeconds(const std::string& key) const {
    auto it = expiries_.find(key);
    if (it == expiries_.end()) return -1;

    auto left = std::chrono::duration_cast<std::chrono::milliseconds>(it->second - Clock::now());
    if (left.count() <= 0) return 0;
    return (left.count() + 999) / 1000;  // round up
}

std::vector<std::string> ExpiryManager::expiredKeys() const {
    std::vector<std::string> result;
    const auto now = Clock::now();
    for (const auto& [key, when] : expiries_) {
        if (now >= when) result.push_back(key);
    }
    return result;
}
