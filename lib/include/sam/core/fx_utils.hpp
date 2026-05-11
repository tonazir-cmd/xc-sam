// =============================================================================
// fx_utils.h
// Xcelerium SAM Framework — Standardised Fixed-Point Utility Functions
//
// All processing blocks that model quantisation effects MUST use these
// functions (or a utility class wrapping them) to ensure cross-component
// consistency.
//
// Representation convention:
//   Fixed-point values are stored as double / std::complex<double>.
//   The integer format is described by (total_bits, fractional_bits).
//   This matches hardware two's-complement Qm.n notation where
//     m = total_bits - fractional_bits - 1  (sign + integer bits)
//     n = fractional_bits
// =============================================================================

#pragma once

#include <complex>
#include <cmath>
#include <algorithm>

namespace sam {
namespace fx {

// -----------------------------------------------------------------------------
// quantize_fx  (rounding / scaling)
//
// Simulates the rounding of a floating-point value to the resolution of a
// fixed-point format with `fractional_bits` fractional bits.
// Uses round-half-to-even (banker's rounding) to match typical hardware.
//
// -----------------------------------------------------------------------------
double quantize_fx(double value, int total_bits, int fractional_bits);

std::complex<double> quantize_fx(std::complex<double> value,
                                  int total_bits,
                                  int fractional_bits);

// -----------------------------------------------------------------------------
// saturate_fx  (clamping)
//
// Simulates saturation: clamps the value to the representable range of a
// signed fixed-point format defined by (total_bits, fractional_bits).
//   max_val =  (2^(total_bits-1) - 1) / 2^fractional_bits
//   min_val = -(2^(total_bits-1))     / 2^fractional_bits
//
// -----------------------------------------------------------------------------
double saturate_fx(double value, int total_bits, int fractional_bits);

std::complex<double> saturate_fx(std::complex<double> value,
                                  int total_bits,
                                  int fractional_bits);

// -----------------------------------------------------------------------------
// quantize_saturate_fx  (round then saturate — MANDATORY for bus/accumulator outputs)
//
// Performs quantize_fx followed by saturate_fx, reflecting the typical
// fixed-point hardware signal path:
//   raw double  →  round to grid  →  clamp to range  →  quantised double
//
// All fixed-point operations involving a limited register width (e.g.
// accumulator output, data-bus output) MUST use this combined function.
//
// -----------------------------------------------------------------------------
double quantize_saturate_fx(double value, int total_bits, int fractional_bits);

std::complex<double> quantize_saturate_fx(std::complex<double> value,
                                           int total_bits,
                                           int fractional_bits);

} // namespace fx
} // namespace sam