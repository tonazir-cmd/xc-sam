#include <itpp/itbase.h>

#include <vector>
#include <complex>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace qam_llr {
// Returns normalized PAM levels for M-QAM
// Levels: alpha * {-(L-1), -(L-3), ..., +(L-3), +(L-1)}
// where L = sqrt(M), alpha = 1/sqrt((M-1)*2/3) for unit avg power
itpp::vec build_pam(int M)
{
    int L = static_cast<int>(std::round(std::sqrt((double)M)));
    if (L * L != M)
        throw std::invalid_argument("M must be a perfect square: " + std::to_string(M));

    // Average power of {-(L-1), ..., +(L-1)} step 2:
    // E = (1/L) * 2 * sum_{k=0}^{L/2-1} (2k+1)^2 = (L^2 - 1) / 3
    // For 2D QAM: E_avg = 2 * (L^2-1)/3, so alpha = 1/sqrt(2*(L^2-1)/3)
    double E_avg = 2.0 * (L * L - 1) / 3.0;
    double alpha = 1.0 / std::sqrt(E_avg);

    itpp::vec levels(L);
    for (int i = 0; i < L; ++i)
        levels[i] = alpha * (2 * i - (L - 1));  // -(L-1), -(L-3), ..., +(L-1)

    return levels;
}
// k-th bit (0-based, MSB first) of Gray code for index i
// with K total bits
inline int gray_bit(int i, int k, int K)
{
    int g = i ^ (i >> 1);           // Gray code of i
    return (g >> (K - 1 - k)) & 1; // extract bit k
}

double saturate(double x, bool enable,
                          double lo = -128.0, double hi = 127.0)
{
    if (!enable)
        return x;

    if (x > hi) return hi;
    if (x < lo) return lo;
    return x;
}

// Compute LLR for one bit index k (0-based) for one real received sample r.
// Returns D_k(r) / sigma2 with optional saturation.
//
// levels  : normalized PAM alphabet (from build_pam)
// k       : bit index, 0-based, 0 = MSB
// sigma2  : noise variance
// sat     : enable saturation flag
// sat_lo/hi : saturation bounds (default S8 integer range)
double llr_bit(double r, int k,
               const itpp::vec& levels,
               double sigma2,
               bool   sat     = false,
               double sat_lo  = -128.0,
               double sat_hi  =  127.0)
{
    int L = static_cast<int>(levels.size());
    int K = static_cast<int>(std::round(std::log2(L)));  // bits per PAM component

    double min_d0 = std::numeric_limits<double>::max();
    double min_d1 = std::numeric_limits<double>::max();

    for (int i = 0; i < L; ++i) {
        double d = (r - levels[i]) * (r - levels[i]);
        if (gray_bit(i, k, K) == 0)
            min_d0 = std::min(min_d0, d);
        else
            min_d1 = std::min(min_d1, d);
    }

    // Raw LLR: positive -> bit 0 more likely, negative -> bit 1 more likely
    double raw_llr = (min_d1 - min_d0) / sigma2;

    return saturate(raw_llr, sat, sat_lo, sat_hi);
}

// Full QAM soft demapper
//
// rx      : received complex symbols, unit avg power assumed
// M       : QAM order (4, 16, 64, 256, 1024)
// sigma2  : noise variance estimate
// sat     : enable saturation
// sat_lo  : lower saturation bound
// sat_hi  : upper saturation bound
//
// Returns: llr[symbol_idx][bit_idx]
//
itpp::mat qam_demap(const itpp::cvec& rx,
                    int    M,
                    double sigma2,
                    bool   sat    = false,
                    double sat_lo = -128.0,
                    double sat_hi =  127.0)
{
    if (sigma2 <= 0.0)
        throw std::invalid_argument("sigma2 must be positive");

    auto levels = build_pam(M);
    int  K      = static_cast<int>(std::round(std::log2(M)));
    int  Khalf  = K / 2;
    int  N      = static_cast<int>(rx.size());

    itpp::mat llr(N, K);

    for (int n = 0; n < N; ++n) {
        double rI = rx[n].real();
        double rQ = rx[n].imag();

        for (int k = 0; k < Khalf; ++k) {
            // I component -> even bit columns
            llr(n, 2 * k) = llr_bit(rI, k, levels, sigma2, sat, sat_lo, sat_hi);

            // Q component -> odd bit columns
            llr(n, 2 * k + 1) = llr_bit(rQ, k, levels, sigma2, sat, sat_lo, sat_hi);
        }
    }

    return llr;
}

} // namespace qam_llr