#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <utility>

// --- Enums -------------------------------------------------------------------

enum class Mode      { LTE, NR5G };
enum class Channel   { PDSCH, PBCH, PDCCH };
enum class Bandwidth { BW_3, BW_5, BW_10, BW_15, BW_20 };

// --- toString helpers --------------------------------------------------------

std::string toString(Mode m);
std::string toString(Channel c);
std::string toString(Bandwidth b);

// --- Config struct -----------------------------------------------------------

struct TestConfig {
    // Enum values
    Mode      mode;
    Channel   channel;
    Bandwidth bandwidth;

    // String variants
    std::string mode_str;
    std::string channel_str;
    std::string bandwidth_str;

    // Build filepath like: "testdata/5g/pdsch/bw10/"
    std::string toVectorsDir(const std::string& base = "testdata") const {
        std::ostringstream oss;
        oss << base << "/"
            << mode_str << "/"
            << channel_str << "/"
            << bandwidth_str << "MHz/";
        return oss.str();
    }
};

// --- ArgsParser Class --------------------------------------------------------

class ArgsParser {
private:
    static const std::unordered_map<std::string, Mode> kModeMap;
    static const std::unordered_map<std::string, Channel> kChannelMap;
    static const std::unordered_map<std::string, Bandwidth> kBandwidthMap;

    // Parse "--key=value" → {"key", "value"}
    static std::pair<std::string, std::string> splitArg(const std::string& arg);

    // Template functions must be defined in the header
    template <typename E>
    static E lookupEnum(const std::string& key,
                        const std::string& val,
                        const std::unordered_map<std::string, E>& map)
    {
        auto it = map.find(val);
        if (it == map.end()) {
            std::ostringstream oss;
            oss << "Invalid value '" << val << "' for --" << key << ". Valid options: ";
            for (const auto& pair : map) oss << pair.first << " ";
            throw std::invalid_argument(oss.str());
        }
        return it->second;
    }

public:
    static TestConfig parse(int argc, char* argv[]);
};