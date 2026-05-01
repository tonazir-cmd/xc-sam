#include "sam/utils/params_reader.hpp"
#include <fstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string ParamsReader::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ParamsReader::tryParseFloat(const std::string& s, float& out) {
    if (s.empty()) return false;
    try {
        std::size_t pos = 0;
        out = std::stof(s, &pos);
        return pos == s.size();
    } catch (...) { return false; }
}

// Indented value: starts with whitespace, no '=', and parses as a float
// (handles negatives like -0.250000)
bool ParamsReader::isIndentedValue(const std::string& line) {
    if (line.empty() || (line[0] != ' ' && line[0] != '\t'))
        return false;
    if (line.find('=') != std::string::npos)
        return false;
    float dummy;
    return tryParseFloat(trim(line), dummy);
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

ParamsReader::ParamsReader(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("ParamsReader: cannot open file: " + filepath);

    std::string line;
    std::string current_key;
    std::vector<float>* current_arr = nullptr;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        // --- Array continuation ---
        if (isIndentedValue(line) && !current_key.empty()) {
            float elem;
            tryParseFloat(trimmed, elem);

            // Promote scalar -> array on first continuation
            if (current_arr == nullptr) {
                auto it = entries_.find(current_key);
                if (it != entries_.end() && std::holds_alternative<float>(it->second)) {
                    float seed  = std::get<float>(it->second);
                    it->second  = std::vector<float>{ seed };
                    current_arr = &std::get<std::vector<float>>(it->second);
                }
            }
            if (current_arr)
                current_arr->push_back(elem);
            continue;
        }

        // --- Key = Value ---
        auto eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            current_key.clear();
            current_arr = nullptr;
            continue;
        }

        std::string key   = trim(trimmed.substr(0, eq_pos));
        std::string value = trim(trimmed.substr(eq_pos + 1));
        if (key.empty()) continue;

        float scalar_val;
        if (tryParseFloat(value, scalar_val)) {
            entries_[key] = scalar_val;    // last-write-wins for duplicates
            current_key   = key;
            current_arr   = nullptr;       // may be promoted if indented lines follow
        } else {
            entries_[key] = std::vector<float>{};
            current_key   = key;
            current_arr   = &std::get<std::vector<float>>(entries_[key]);
        }
    }
}

// ---------------------------------------------------------------------------
// Non-template implementations (called by the template wrappers in the header)
// ---------------------------------------------------------------------------

bool ParamsReader::hasKey(const std::string& key) const {
    return entries_.count(key) > 0;
}

bool ParamsReader::isArray(const std::string& key) const {
    auto it = entries_.find(key);
    return it != entries_.end() && std::holds_alternative<std::vector<float>>(it->second);
}

bool ParamsReader::isScalar(const std::string& key) const {
    auto it = entries_.find(key);
    return it != entries_.end() && std::holds_alternative<float>(it->second);
}

float ParamsReader::getFloatImpl(const std::string& key) const {
    auto it = entries_.find(key);
    if (it == entries_.end())
        throw std::runtime_error("ParamsReader: key not found: " + key);
    if (!std::holds_alternative<float>(it->second))
        throw std::runtime_error("ParamsReader: key is an array, not scalar: " + key);
    return std::get<float>(it->second);
}

const std::vector<float>& ParamsReader::getArrayImpl(const std::string& key) const {
    auto it = entries_.find(key);
    if (it == entries_.end())
        throw std::runtime_error("ParamsReader: key not found: " + key);
    if (!std::holds_alternative<std::vector<float>>(it->second))
        throw std::runtime_error("ParamsReader: key is a scalar, not array: " + key);
    return std::get<std::vector<float>>(it->second);
}

// ---------------------------------------------------------------------------
// Debug dump
// ---------------------------------------------------------------------------

void ParamsReader::dump() const {
    for (const auto& [key, val] : entries_) {
        if (std::holds_alternative<float>(val)) {
            std::cout << key << " = " << std::get<float>(val) << "\n";
        } else {
            const auto& arr = std::get<std::vector<float>>(val);
            std::cout << key << " [" << arr.size() << "] = { ";
            for (std::size_t i = 0; i < arr.size(); ++i) {
                std::cout << arr[i];
                if (i + 1 < arr.size()) std::cout << ", ";
            }
            std::cout << " }\n";
        }
    }
}