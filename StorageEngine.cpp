#include "StorageEngine.h"

void StorageEngine::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::optional<std::string> StorageEngine::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

bool StorageEngine::erase(const std::string& key) {
    return data_.erase(key) > 0;
}

bool StorageEngine::contains(const std::string& key) const {
    return data_.find(key) != data_.end();
}

std::vector<std::string> StorageEngine::keys() const {
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (const auto& kv : data_) result.push_back(kv.first);
    return result;
}
