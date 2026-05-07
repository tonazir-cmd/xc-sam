#pragma once

#include <string>
#include <itpp/itbase.h>

itpp::cvec read_sc16_as_cvec(const std::string& filename, size_t fix_point, size_t num_samples = 0);
itpp::ivec read_int8_as_ivec(const std::string& filename, size_t num_samples = 0);
void save_cvec_as_complex128(const itpp::cvec& vec, const std::string& filename);