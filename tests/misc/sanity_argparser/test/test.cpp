#include <iostream>
#include "sam/utils/argsparser.h"

// --- Usage example -----------------------------------------------------------

int main(int argc, char* argv[]) {
    try {
        TestConfig cfg = ArgsParser::parse(argc, argv);

        std::cout << "--- Enum Access ---\n";
        std::cout << "Mode Enum ID:  " << static_cast<int>(cfg.mode) << "\n";
        
        std::cout << "\n--- String Access (direct properties) ---\n";
        std::cout << "Mode:      " << cfg.mode_str << "\n";
        std::cout << "Channel:   " << cfg.channel_str << "\n";
        std::cout << "Bandwidth: " << cfg.bandwidth_str << " MHz\n";
        std::cout << "FilePath:  " << cfg.toVectorsDir("../test_vectors") << "\n\n";

        // In your test, use cfg.mode == Mode::NR5G etc.
        if (cfg.mode == Mode::NR5G && cfg.channel == Channel::PDSCH) {
            std::cout << "Running 5G PDSCH test...\n";
        }

    } catch (const std::invalid_argument& e) {
        std::cerr << "Arg error: " << e.what() << "\n";
        std::cerr << "Usage: ./test_app --mode=5g --channel=pdsch --bandwidth=20\n";
        return 1;
    }

    return 0;
}