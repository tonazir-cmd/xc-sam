#include "sam/utils/argsparser.hpp"
#include <iostream>
#include <cstdlib>

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

void ArgsParser::printHelp() {
    std::cout << "Usage: program [OPTIONS]\n\n"
              << "Options:\n"
              << "  --help, -h          Print this help message\n"
              << "  --mode=VAL          Set mode. Valid options: lte, 5g (default: lte)\n"
              << "  --channel=VAL       Set channel. Valid options: pdsch, pbch, pdcch (default: pdsch)\n"
              << "  --bandwidth=VAL     Set bandwidth. Valid options: 3, 5, 10, 15, 20 (default: 20)\n"
              << "  --dmrs-mode=VAL     Set DMRS mode. Valid options: 1, 2, 3, 4, 5 (default: 1)\n";
}

void ArgsParser::parse(TestArgs& args, int argc, char* argv[]) {
    try {
        for (int i = 1; i < argc; ++i) {
            std::string argStr = argv[i];
            
            // Intercept help flag
            if (argStr == "--help" || argStr == "-h") {
                printHelp();
                std::exit(0); 
            }

            auto argPair = splitArg(argStr);
            const std::string& key = argPair.first;
            const std::string& val = argPair.second;

            if      (key == "mode")      args.mode_str      = val;
            else if (key == "channel")   args.channel_str   = val;
            else if (key == "bandwidth") args.bandwidth_str = val;
            else if (key == "dmrs-mode") args.dmrs_mode_str = val;
            else
                throw std::invalid_argument("Unknown argument: --" + key);
        }

        // Validate and sync Enums
        args.mode      = lookupEnum("mode",      args.mode_str,      kModeMap);
        args.channel   = lookupEnum("channel",   args.channel_str,   kChannelMap);
        args.bandwidth = lookupEnum("bandwidth", args.bandwidth_str, kBandwidthMap);
        args.dmrs_mode = lookupEnum("dmrs-mode", args.dmrs_mode_str, kDMRSModeMap);

    } catch (const std::exception& e) {
        std::cerr << "\n[Error] " << e.what() << "\n\n";
        printHelp();
        std::exit(1); 
    }
}

TestArgs ArgsParser::parse(int argc, char* argv[]) {
    TestArgs args;
    parse(args, argc, argv);
    return args;
}