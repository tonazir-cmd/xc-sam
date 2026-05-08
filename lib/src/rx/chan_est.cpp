#include "sam/rx/chan_est.hpp"

#include <itpp/base/vec.h>
#include <itpp/base/math/misc.h>

#include <cassert>
#include <complex>

namespace sam
{
namespace rx
{

void ChannelEstimator::eval(const Inputs&      in,
                            Outputs&           out,
                            const Control&     ctrl,
                            const Config&      cfg)
{
    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------
    if (!ctrl.enable) return;
    if (ctrl.bypass) return;

    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    assert(cfg.n_sc > 0);
    assert(in.rx->samples.length() == cfg.n_sc);
    assert(in.dmrs->samples.length() == (cfg.n_sc / 2));

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------
    itpp::cvec hest = ls_estimate_(in.rx->samples, in.dmrs->samples, cfg.n_sc, cfg.dmrs_pattern);
    average_adjacent_pilots_(hest, cfg.n_sc, cfg.dmrs_pattern);
    interpolate_(hest, cfg.n_sc, cfg.dmrs_pattern);
    extrapolate_edges_(hest, cfg.n_sc, cfg.dmrs_pattern);

    out.samples = hest;
}

// =============================================================================
// Private Helpers
// =============================================================================

itpp::cvec ChannelEstimator::ls_estimate_(const itpp::cvec& rx_grid, 
                                          const itpp::cvec& tx_dmrs, 
                                          uint16_t n_sc, 
                                          DMRSPattern dmrs_pattern)
{
    itpp::cvec hest_ls = itpp::zeros_c(n_sc);

    const int start_idx = (dmrs_pattern == DMRSPattern::Even) ? 0 : 1;

    for (int i = start_idx; i < n_sc; i += 2)
    {
        hest_ls[i] = rx_grid[i] * std::conj(tx_dmrs[i / 2]);
    }

    return hest_ls;
}

void ChannelEstimator::average_adjacent_pilots_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern)
{
    const int start_idx = (dmrs_pattern == DMRSPattern::Even) ? 0 : 1;

    // if input = a, 0, b, 0, c, 0, d, 0 ..
    // then output = (a+b)/2, 0, (a+b)/2, 0, (c+d)/2, 0, (c+d)/2
    for (int i = start_idx; i < n_sc - 2; i += 4)
    {
        std::complex<double> avg_val = (hest[i] + hest[i + 2]) / 2.0;

        hest[i] = avg_val;
        hest[i + 2] = avg_val;
    }
}

void ChannelEstimator::interpolate_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern)
{
    // If odd dmrs_pattern (1), missing subcarriers start at 2. If even dmrs_pattern (0), they start at 1.
    const int start_idx = (dmrs_pattern == DMRSPattern::Odd) ? 2 : 1;

    for (int i = start_idx; i < n_sc - 1; i += 2)
    {
        hest[i] = (hest[i - 1] + hest[i + 1]) / 2.0;
    }
}

void ChannelEstimator::extrapolate_edges_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern)
{
    // extrapolate first estimated pilot in pattern in odd patterns
    if (dmrs_pattern == DMRSPattern::Odd)
    {
        hest[0] = hest[1] - (hest[2] - hest[1]); 
    }

    // Extrapolate the last pilot
    hest[n_sc - 1] = hest[n_sc - 2] + (hest[n_sc - 2] - hest[n_sc - 4]);
}

} // namespace rx
} // namespace sam