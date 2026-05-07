/**
 * matlab_ported.cpp
 *
 * Contains Matlab ported functions
 * THIS IS A TEMPORARY FILE FOR QAM DEMAPPING
 * 5G-compliant QAM soft demapper using piecewise-linear LLR approximation.
 * Supports: QPSK (4), 16-QAM, 64-QAM, 256-QAM, 1024-QAM
 *
 * Dependencies: IT++ (itpp/itbase.h, itpp/itcomm.h)
 */

#include "sam/utils/matlab_ported.hpp"

#include <itpp/itbase.h>
#include <itpp/comm/modulator.h>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <string>

namespace matlab_ported {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static constexpr int NUM_BOUNDS = 8;

/**
 * Select LUT parameters for the given MCS and bit index.
 *
 * @param mcs        Modulation order: 4, 16, 64, 256, or 1024
 * @param bit_idx    1-based bit index within one quadrature component
 * (1 .. log2(mcs)/2)
 * @param[out] y_boundaries  8-element row of y-intercepts (pre-scaled)
 * @param[out] x_boundaries  8-element left-edge x positions (pre-scaled)
 * @param[out] slopes        8-element slopes (pre-scaled)
 * @param[out] boundary_width  uniform width of each piecewise segment (pre-scaled)
 */
static void select_lut(int mcs, int bit_idx,
                       itpp::vec &y_boundaries,
                       itpp::vec &x_boundaries,
                       itpp::vec &slopes,
                       double    &boundary_width)
{
    // Normalisation scalars (unit average power per QAM order)
    const double s4    = 1.0 / std::sqrt(2.0);
    const double s16   = 1.0 / std::sqrt(10.0);
    const double s64   = 1.0 / std::sqrt(42.0);
    const double s256  = 1.0 / std::sqrt(170.0);
    const double s1024 = 1.0 / std::sqrt(682.0);

    // ------------------------------------------------------------------
    // Raw (unscaled) LUT tables, indexed [bit_idx-1][boundary 0..7]
    // ------------------------------------------------------------------

    // ---- QPSK (4-QAM) ----
    static const double yb4_raw[1][8] = {
        {0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75}
    };
    static const double sl4_raw[1][8] = {
        {1, 1, 1, 1, 1, 1, 1, 1}
    };

    // ---- 16-QAM ----
    static const double yb16_raw[2][8] = {
        {0,   0.5, 1,   1.5, 2,    3,    4,    5  },
        {2,   1.5, 1,   0.5, 0,  -0.5, -1,  -1.5}
    };
    static const double sl16_raw[2][8] = {
        {1,  1,  1,  1,  2,  2,  2,  2 },
        {-1, -1, -1, -1, -1, -1, -1, -1}
    };

    // ---- 64-QAM ----
    static const double yb64_raw[3][8] = {
        {0,  1,  2,  4,  6,  9, 12, 16},
        {6,  4,  2,  1,  0, -1, -2, -4},
        {-2, -1,  0,  1,  2,  1,  0, -1}
    };
    static const double sl64_raw[3][8] = {
        { 1,  1,  2,  2,  3,  3,  4,  4},
        {-2, -2, -1, -1, -1, -1, -2, -2},
        { 1,  1,  1,  1, -1, -1, -1, -1}
    };

    // ---- 256-QAM ----
    static const double yb256_raw[4][8] = {
        { 0,  2,  6, 12, 20, 30, 42, 56},
        {20, 12,  6,  2,  0, -2, -6,-12},
        {-6, -2,  0,  2,  6,  2,  0, -2},
        {-2,  0,  2,  0, -2,  0,  2,  0}
    };
    static const double sl256_raw[4][8] = {
        { 1,  2,  3,  4,  5,  6,  7,  8},
        {-4, -3, -2, -1, -1, -2, -3, -4},
        { 2,  1,  1,  2, -2, -1, -1, -2},
        { 1,  1, -1, -1,  1,  1, -1, -1}
    };

    // ---- 1024-QAM ----
    static const double yb1024_raw[5][8] = {
        {  0,   6,  20,  42,  72, 110, 156, 210},
        { 72,  42,  20,   6,   0,  -6, -20, -42},
        {-20,  -6,   0,   6,  20,   6,   0,  -6},
        { -6,   0,   6,   0,  -6,   0,   6,   0},
        { -2,   2,  -2,   2,  -2,   2,  -2,   2}
    };
    static const double sl1024_raw[5][8] = {
        { 1.5,  3.5,  5.5,  7.5,  9.5, 11.5, 13.5, 15.5},
        {-7.5, -5.5, -3.5, -1.5, -1.5, -3.5, -5.5, -7.5},
        { 3.5,  1.5,  1.5,  3.5, -3.5, -1.5, -1.5, -3.5},
        { 1.5,  1.5, -1.5, -1.5,  1.5,  1.5, -1.5, -1.5},
        { 1.0, -1.0,  1.0, -1.0,  1.0, -1.0,  1.0, -1.0}
    };

    // ------------------------------------------------------------------
    // Select scalar, raw arrays, boundary width; then scale and return
    // ------------------------------------------------------------------

    double sc;          
    int    max_bits;    
    double bw_raw;      

    const double (*yb_ptr)[8] = nullptr;
    const double (*sl_ptr)[8] = nullptr;

    switch (mcs) {
        case 4:
            sc = s4; max_bits = 1; bw_raw = 0.25;
            yb_ptr = yb4_raw; sl_ptr = sl4_raw;
            break;
        case 16:
            sc = s16; max_bits = 2; bw_raw = 0.5;
            yb_ptr = yb16_raw; sl_ptr = sl16_raw;
            break;
        case 64:
            sc = s64; max_bits = 3; bw_raw = 1.0;
            yb_ptr = yb64_raw; sl_ptr = sl64_raw;
            break;
        case 256:
            sc = s256; max_bits = 4; bw_raw = 2.0;
            yb_ptr = yb256_raw; sl_ptr = sl256_raw;
            break;
        case 1024:
            sc = s1024; max_bits = 5; bw_raw = 4.0;
            yb_ptr = yb1024_raw; sl_ptr = sl1024_raw;
            break;
        default:
            throw std::invalid_argument(
                "Unsupported MCS: " + std::to_string(mcs) +
                ". Must be 4, 16, 64, 256, or 1024.");
    }

    if (bit_idx < 1 || bit_idx > max_bits)
        throw std::invalid_argument("bit_idx out of range for MCS " +
                                    std::to_string(mcs));

    const int row = bit_idx - 1;

    y_boundaries.set_size(NUM_BOUNDS);
    x_boundaries.set_size(NUM_BOUNDS);
    slopes.set_size(NUM_BOUNDS);

    for (int b = 0; b < NUM_BOUNDS; ++b) {
        y_boundaries[b] = (sc * sc) * yb_ptr[row][b];
        slopes[b]       = sc        * sl_ptr[row][b];
        x_boundaries[b] = sc        * b * bw_raw;
    }

    boundary_width = sc * bw_raw;
}

// ---------------------------------------------------------------------------
// vqamdemap_internal
// ---------------------------------------------------------------------------

/**
 * Compute soft D-metric for one quadrature component and one bit index.
 *
 * @param R        Real-valued received samples (one quadrature component),
 * unit average power.
 * @param mcs      Modulation order (4 / 16 / 64 / 256 / 1024).
 * @param bit_idx  1-based bit index (1 .. log2(mcs)/2).
 * @return         D-metric vector, same length as R.
 */
static itpp::vec vqamdemap_internal(const itpp::vec &R, int mcs, int bit_idx)
{
    itpp::vec y_boundaries, x_boundaries, slopes;
    double boundary_width;
    select_lut(mcs, bit_idx, y_boundaries, x_boundaries, slopes, boundary_width);

    const int N = R.length();
    itpp::vec A = itpp::abs(R); 
    itpp::vec Y(N);

    for (int i = 0; i < N; ++i) {
        double a = A[i];

        int left = NUM_BOUNDS - 1;
        for (int b = 1; b < NUM_BOUNDS; ++b) {
            if (a >= x_boundaries[b - 1] && a < x_boundaries[b]) {
                left = b - 1;
                break;
            }
        }

        double deltaL = a - x_boundaries[left];
        Y[i] = slopes[left] * deltaL + y_boundaries[left];
    }

    if (bit_idx == 1) {
        for (int i = 0; i < N; ++i) {
            if (R[i] < 0.0)
                Y[i] = -Y[i];
        }
    }

    return Y;
}

// ---------------------------------------------------------------------------
// vqamdemap
// ---------------------------------------------------------------------------

/**
 * Full QAM soft demapper for a vector of complex received symbols.
 *
 * @param R    Complex received symbols, unit average power (itpp::cvec).
 * @param mcs  Modulation order.
 * @return     D-metric matrix: rows = symbols, cols = bits per symbol
 */
static itpp::mat vqamdemap(const itpp::cvec &R, int mcs)
{
    const int N     = R.length();
    const int K     = static_cast<int>(std::log2(mcs));
    const int Khalf = K / 2;

    itpp::vec R_I = itpp::real(R);
    itpp::vec R_Q = itpp::imag(R);

    itpp::mat D(N, K);

    for (int k = 1; k <= Khalf; ++k) {
        itpp::vec di = vqamdemap_internal(R_I, mcs, k);
        itpp::vec dq = vqamdemap_internal(R_Q, mcs, k);

        // IT++ vectorized column assignment
        D.set_col(2 * (k - 1), di);
        D.set_col(2 * (k - 1) + 1, dq);
    }

    return D;
}

// ---------------------------------------------------------------------------
// Q-format saturation helper
// ---------------------------------------------------------------------------

/**
 * Saturate and round to signed fixed-point S(int_bits).(frac_bits) format.
 */
static itpp::mat qformat_mat(const itpp::mat &X, int int_bits, int frac_bits)
{
    const double scale   = std::pow(2.0, frac_bits);
    const double max_val = std::pow(2.0, int_bits) - 1.0 / scale;
    const double min_val = -std::pow(2.0, int_bits);
    
    // IT++ matrix scalar math and rounding vectorization
    itpp::mat out = itpp::round(X * scale) / scale;
    
    for (int r = 0; r < out.rows(); ++r) {
        for (int c = 0; c < out.cols(); ++c) {
            if (out(r, c) > max_val) out(r, c) = max_val;
            if (out(r, c) < min_val) out(r, c) = min_val;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// demapper_5g
// ---------------------------------------------------------------------------

/**
 * 5G LLR demapper with bias, fixed-point scaling, and saturation.
 *
 * @param rx_symbols  Concatenated complex received symbols (M elements).
 * First M/2 correspond to stream/slot 0, last M/2 to stream/slot 1.
 * @param noise_est   Estimated noise variance (sigma^2).
 * @param mcs         Modulation order (4 / 16 / 64 / 256 / 1024).
 * @return            LLR output matrix (no_of_rows = no_of_symbols, no_of_cols = bits per symbol).
 */
itpp::mat demapper_5g(const itpp::cvec &rx_symbols,
                      double            noise_est,
                      int               mcs)
{
    double bias;
    switch (mcs) {
        case 4:    bias = 1.0; break;
        case 16:   bias = 2.0; break;
        case 64:   bias = 2.0; break;
        default:   bias = 4.0; break;  
    }

    itpp::mat D0 = bias * vqamdemap(rx_symbols, mcs);

    double raw       = (4.0 / bias) / noise_est;
    double truncated = (raw >= 0) ? std::floor(raw) : std::ceil(raw);
    double mod1      = std::fmod(truncated, 256.0);
    if (mod1 < 0) mod1 += 256.0;
    
    double shifted      = mod1 * 2.0;
    double scale_factor = std::fmod(shifted, 256.0);
    if (scale_factor < 0) scale_factor += 256.0;

    itpp::mat llr = qformat_mat(D0 * scale_factor, 7, 0);
    return llr;
}

}