#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <itpp/itbase.h>

typedef struct {
    bool lte;
    uint16_t scs;
    uint16_t n_fft;
    uint16_t n_sc;
    uint16_t cp;
    double gain;
    double fo;
    size_t to;
    double threshold;
} Config;

bool get_all_configs(const std::string& directoryPath, std::vector<std::string>& filenames);
bool read_config(const std::string& filename, Config& cfg);
bool read_complex_binary(const std::string& filename, itpp::cvec& out_vec);