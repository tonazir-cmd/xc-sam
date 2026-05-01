#include "sam/utils/argsparser.h"

// --- toString helpers implementation -----------------------------------------

std::string toString(Mode m) {
    switch (m) {
        case Mode::LTE:  return "lte";
        case Mode::NR5G: return "5g";
    }
    return "";
}

std::string toString(Channel c) {
    switch (c) {
        case Channel::PDSCH: return "pdsch";
        case Channel::PBCH:  return "pbch";
        case Channel::PDCCH: return "pdcch";
    }
    return "";
}

std::string toString(Bandwidth b) {
    switch (b) {
        case Bandwidth::BW_3:  return "3";
        case Bandwidth::BW_5:  return "5";
        case Bandwidth::BW_10: return "10";
        case Bandwidth::BW_15: return "15";
        case Bandwidth::BW_20: return "20";
    }
    return "";
}

// --- Static Map Initializations ----------------------------------------------

const std::unordered_map<std::string, Mode> ArgsParser::kModeMap = {
    {"lte",  Mode::LTE},
    {"5g",   Mode::NR5G},
};

const std::unordered_map<std::string, Channel> ArgsParser::kChannelMap = {
    {"pdsch", Channel::PDSCH},
    {"pbch",  Channel::PBCH},
    {"pdcch", Channel::PDCCH},
};

const std::unordered_map<std::string, Bandwidth> ArgsParser::kBandwidthMap = {
    {"3",  Bandwidth::BW_3},
    {"5",  Bandwidth::BW_5},
    {"10", Bandwidth::BW_10},
    {"15", Bandwidth::BW_15},
    {"20", Bandwidth::BW_20},
};

// --- ArgsParser implementation -----------------------------------------------

std::pair<std::string, std::string> ArgsParser::splitArg(const std::string& arg) {
    if (arg.size() < 3 || arg[0] != '-' || arg[1] != '-')
        throw std::invalid_argument("Unexpected argument format: " + arg);

    auto eq = arg.find('=');
    if (eq == std::string::npos)
        throw std::invalid_argument("Missing '=' in argument: " + arg);

    return { arg.substr(2, eq - 2), arg.substr(eq + 1) };
}

TestArgs ArgsParser::parse(int argc, char* argv[]) {
    // Defaults
    std::string modeStr      = "lte";
    std::string channelStr   = "pdsch";
    std::string bandwidthStr = "20";

    for (int i = 1; i < argc; ++i) {
        auto argPair = splitArg(argv[i]);
        const std::string& key = argPair.first;
        const std::string& val = argPair.second;

        if      (key == "mode")      modeStr      = val;
        else if (key == "channel")   channelStr   = val;
        else if (key == "bandwidth") bandwidthStr = val;
        else
            throw std::invalid_argument("Unknown argument: --" + key);
    }

    return TestArgs{
        lookupEnum("mode",      modeStr,      kModeMap),
        lookupEnum("channel",   channelStr,   kChannelMap),
        lookupEnum("bandwidth", bandwidthStr, kBandwidthMap),
        modeStr,
        channelStr,
        bandwidthStr
    };
}