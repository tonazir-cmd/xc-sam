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
                            const Config&      cfg,
                            const ExecContext& ctx)
{
    if (!ctrl.enable) return;
    if (ctrl.bypass) { out.samples = in.samples; return; }

    assert(cfg.n_sc > 0);
    assert(in.samples.length() == cfg.n_sc);
    assert(cfg.dmrs_seq.length() == cfg.n_sc);

    // 1. Least Squares Estimation at DMRS comb locations
    itpp::cvec hest = ls_estimate_(in.samples, cfg.dmrs_seq, cfg.n_sc, cfg.dmrs_comb);

    // 2. Linear Interpolation for subcarriers between pilots
    interpolate_(hest, cfg.n_sc, cfg.dmrs_comb);

    // 3. Extrapolate edges based on linear slope
    extrapolate_edges_(hest, cfg.n_sc, cfg.dmrs_comb);

    out.samples = hest;
}

// =============================================================================
// Private Helpers
// =============================================================================

itpp::cvec ChannelEstimator::ls_estimate_(const itpp::cvec& rx_grid, 
                                          const itpp::cvec& tx_dmrs, 
                                          uint16_t n_sc, 
                                          CombPattern comb)
{
    itpp::cvec hest_ls = itpp::zeros_c(n_sc);

    // Cast the enum to int to get the correct starting index (0 or 1)
    const int start_idx = static_cast<int>(comb);

    for (int i = start_idx; i < n_sc; i += 2)
    {
        hest_ls[i] = rx_grid[i] * std::conj(tx_dmrs[i]);
    }

    return hest_ls;
}

void ChannelEstimator::interpolate_(itpp::cvec& hest, uint16_t n_sc, CombPattern comb)
{
    // If odd comb (1), missing subcarriers start at 2. If even comb (0), they start at 1.
    const int start_idx = (comb == CombPattern::Odd) ? 2 : 1;

    for (int i = start_idx; i < n_sc - 1; i += 2)
    {
        hest[i] = (hest[i - 1] + hest[i + 1]) / 2.0;
    }
}

void ChannelEstimator::extrapolate_edges_(itpp::cvec& hest, uint16_t n_sc, CombPattern comb)
{
    if (comb == CombPattern::Odd && n_sc >= 4)
    {
        // hest[0] = hest[1] - slope
        hest[0] = hest[1] - (hest[3] - hest[1]); 
    }

    const int last_idx = n_sc - 1;
    const int comb_offset = static_cast<int>(comb);

    // Extrapolate the last index if it wasn't populated by the comb
    if ((last_idx % 2) != comb_offset && n_sc >= 4)
    {
        // hest[last] = hest[last-1] + slope
        hest[last_idx] = hest[last_idx - 1] + (hest[last_idx - 1] - hest[last_idx - 3]);
    }
}

} // namespace rx
} // namespace sam