#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <iostream>

// --- Enums -------------------------------------------------------------------

enum class Mode      { LTE, NR5G };
enum class Channel   { PDSCH, PBCH, PDCCH };
enum class Bandwidth { BW_3, BW_5, BW_10, BW_15, BW_20 };
enum class DMRSMode  { Mode1, Mode2, Mode3, Mode4, Mode5 };

// --- toString helpers --------------------------------------------------------

std::string toString(Mode m);
std::string toString(Channel c);
std::string toString(Bandwidth b);
std::string toString(DMRSMode b);

// --- Config struct -----------------------------------------------------------

struct TestArgs {
    // Enum values with defaults
    Mode      mode      = Mode::LTE;
    Channel   channel   = Channel::PDSCH;
    Bandwidth bandwidth = Bandwidth::BW_20;
    DMRSMode  dmrs_mode = DMRSMode::Mode1;

    // String variants with defaults
    std::string mode_str      = "lte";
    std::string channel_str   = "pdsch";
    std::string bandwidth_str = "20";
    std::string dmrs_mode_str = "1";

    // Constructor synchronizes Enums with their string representations
    TestArgs(Mode m = Mode::LTE, 
             Channel c = Channel::PDSCH, 
             Bandwidth b = Bandwidth::BW_20, 
             DMRSMode d = DMRSMode::Mode1)
        : mode(m), 
          channel(c), 
          bandwidth(b), 
          dmrs_mode(d),
          mode_str(toString(m)),
          channel_str(toString(c)),
          bandwidth_str(toString(b)),
          dmrs_mode_str(toString(d)) {}

    // Build filepath like: "testdata/lte/pdsch/10MHz/" or "testdata/5g/pdsch/dmrs_mode_2/"
    std::string toVectorsDir(const std::string& base = "..") const {
        std::ostringstream oss;
        if (mode == Mode::NR5G) {
            oss << base << "/"
                << mode_str << "/"
                << channel_str << "/"
                << "dmrs_mode_" << dmrs_mode_str << "/";
        } 
        else {
            oss << base << "/"
                << mode_str << "/"
                << channel_str << "/"
                << bandwidth_str << "MHz/";
        }
        return oss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const TestArgs& args) {
        os << "Mode: " << args.mode_str << "\n"
           << "Channel: " << args.channel_str << "\n"
           << "Bandwidth: " << args.bandwidth_str << "\n"
           << "DMRS Mode: " << args.dmrs_mode_str << "\n";
        return os;
    }
};

// --- ArgsParser Class --------------------------------------------------------

class ArgsParser {
private:
    static const std::unordered_map<std::string, Mode> kModeMap;
    static const std::unordered_map<std::string, Channel> kChannelMap;
    static const std::unordered_map<std::string, Bandwidth> kBandwidthMap;
    static const std::unordered_map<std::string, DMRSMode> kDMRSModeMap;

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
    static TestArgs parse(int argc, char* argv[]);
    static void parse(TestArgs& args, int argc, char* argv[]);
    static void printHelp();
};