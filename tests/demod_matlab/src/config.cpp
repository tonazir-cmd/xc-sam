#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <algorithm>

#include "config.h"

namespace fs = std::filesystem;
bool get_all_configs(const std::string& directoryPath, std::vector<std::string>& filenames) {
    try {
        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            std::cerr << "Error: Directory does not exist or is not a directory: " << directoryPath << std::endl;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (fs::is_regular_file(entry) && entry.path().extension() == ".txt") {
                filenames.push_back(entry.path().stem().string());
            }
        }

        // sort according to frequency extracted from filename (e.g., config_15MHz.txt -> 15)
        std::sort(filenames.begin(), filenames.end(), [](const std::string& a, const std::string& b) {
            auto get_num = [](const std::string& s) {
                size_t last_underscore = s.find_last_of('_', s.find("_MHz") - 1);
                return std::stoi(s.substr(last_underscore + 1));
            };
            return get_num(a) < get_num(b);
        });
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool read_config(const std::string& filename, Config& cfg) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        std::istringstream iss(line);
        std::string key;
        iss >> key;

             if (key == "lte")       { int val; iss >> val; cfg.lte = (val != 0); }
        else if (key == "scs")       { iss >> cfg.scs; }
        else if (key == "n_fft")     { iss >> cfg.n_fft; }
        else if (key == "n_sc")      { iss >> cfg.n_sc; }
        else if (key == "cp")        { iss >> cfg.cp; }
        else if (key == "gain")      { iss >> cfg.gain; }
        else if (key == "fo")        { iss >> cfg.fo; }
        else if (key == "to")        { iss >> cfg.to; }
        else if (key == "threshold") { iss >> cfg.threshold; }
    }
    file.close();
    return true;
}

bool read_complex_binary(const std::string& filename, itpp::cvec& out_vec) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open binary file " << filename << "\n";
        return false;
    }
    
    // Calculate file size and number of complex elements
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg); // Rewind to the beginning
    
    // 1 complex number = 2 doubles (real and imaginary)
    size_t num_complex_elements = file_size / (2 * sizeof(double));

    // Initialize IT++ cvec with the correct size
    out_vec.set_size(num_complex_elements, false);

    // Read the raw interleaved bytes directly into the cvec's memory buffer.
    if (!file.read(reinterpret_cast<char*>(out_vec._data()), file_size)) {
        std::cerr << "Error reading complex binary data.\n";
        return false;
    }

    return true;
}