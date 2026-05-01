#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <stdexcept>
#include <optional>
#include <functional>
#include <type_traits>

// Internal storage is always float (file format is float).
// Public API converts to whatever T the caller requests.
using ParamsValue = std::variant<float, std::vector<float>>;

class ParamsReader {
public:
    explicit ParamsReader(const std::string& filepath);

    // -------------------------------------------------------------------------
    // get<T>(key)             — scalar, throws if missing or is array
    // get<T>(key, default)    — scalar with fallback
    // tryGet<T>(key)          — scalar, returns nullopt if missing
    //
    // getArray<T>(key)        — vector<T>, throws if missing or is scalar
    // tryGetArray<T>(key)     — vector<T>, returns nullopt if missing
    //
    // T can be: int, float, double, uint16_t, bool, or any arithmetic type.
    // -------------------------------------------------------------------------

    template<typename T>
    T get(const std::string& key) const {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        return static_cast<T>(getFloatImpl(key));
    }

    template<typename T>
    T get(const std::string& key, T default_val) const {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        if (!hasKey(key)) return default_val;
        return static_cast<T>(getFloatImpl(key));
    }

    template<typename T>
    std::optional<T> tryGet(const std::string& key) const {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        if (!hasKey(key)) return std::nullopt;
        return static_cast<T>(getFloatImpl(key));
    }

    template<typename T>
    std::vector<T> getArray(const std::string& key) const {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        const auto& src = getArrayImpl(key);
        std::vector<T> out;
        out.reserve(src.size());
        for (float f : src) out.push_back(static_cast<T>(f));
        return out;
    }

    template<typename T>
    std::optional<std::vector<T>> tryGetArray(const std::string& key) const {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
        if (!hasKey(key)) return std::nullopt;
        return getArray<T>(key);
    }

    // --- Introspection ---
    bool hasKey(const std::string& key) const;
    bool isArray(const std::string& key) const;
    bool isScalar(const std::string& key) const;

    void dump() const;

private:
    std::unordered_map<std::string, ParamsValue> entries_;

    // Non-template implementations (defined in .cpp)
    float                      getFloatImpl(const std::string& key) const;
    const std::vector<float>&  getArrayImpl(const std::string& key) const;

    static std::string trim(const std::string& s);
    static bool isIndentedValue(const std::string& line);
    static bool tryParseFloat(const std::string& s, float& out);
};