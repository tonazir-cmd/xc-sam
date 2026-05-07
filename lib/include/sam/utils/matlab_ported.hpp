#pragma once

#include <itpp/itbase.h>
namespace matlab_ported 
{
/**
 * 5G LLR demapper with bias, fixed-point scaling, and saturation.
 *
 * @param rx_symbols  Concatenated complex received symbols (M elements).
 * First M/2 correspond to stream/slot 0, last M/2 to stream/slot 1.
 * @param noise_est   Estimated noise variance (sigma^2).
 * @param mcs         Modulation order (4 / 16 / 64 / 256 / 1024).
 * @return            LLR output matrix (no_of_rows = no_of_symbols, no_of_cols = bits per symbol).
 */
itpp::mat demapper_5g(const itpp::cvec &rx_symbols, double noise_est, int mcs);
}