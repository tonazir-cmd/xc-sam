#include "sam/utils/argsparser.hpp"

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

std::string toString(DMRSMode m) {
    switch (m) {
        case DMRSMode::Mode1: return "1";
        case DMRSMode::Mode2: return "2";
        case DMRSMode::Mode3: return "3";
        case DMRSMode::Mode4: return "4";
        case DMRSMode::Mode5: return "5";
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

const std::unordered_map<std::string, DMRSMode> ArgsParser::kDMRSModeMap = {
    {"1", DMRSMode::Mode1},
    {"2", DMRSMode::Mode2},
    {"3", DMRSMode::Mode3},
    {"4", DMRSMode::Mode4},
    {"5", DMRSMode::Mode5},
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
    std::string dmrs_modeStr = "1";

    for (int i = 1; i < argc; ++i) {
        auto argPair = splitArg(argv[i]);
        const std::string& key = argPair.first;
        const std::string& val = argPair.second;

        if      (key == "mode")      modeStr      = val;
        else if (key == "channel")   channelStr   = val;
        else if (key == "bandwidth") bandwidthStr = val;
        else if (key == "dmrs-mode") dmrs_modeStr = val;
        else
            throw std::invalid_argument("Unknown argument: --" + key);
    }

    return TestArgs{
        lookupEnum("mode",      modeStr,      kModeMap),
        lookupEnum("channel",   channelStr,   kChannelMap),
        lookupEnum("bandwidth", bandwidthStr, kBandwidthMap),
        lookupEnum("dmrs-mode", dmrs_modeStr, kDMRSModeMap),
        modeStr,
        channelStr,
        bandwidthStr,
        dmrs_modeStr
    };
}