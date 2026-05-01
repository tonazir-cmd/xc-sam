#pragma once

#include "sam/core/base.hpp"
#include "sam/core/signal_types.hpp"

#include <itpp/base/vec.h>

#include <cstdint>
#include <cassert>

namespace sam
{
namespace rx
{

class ChannelEstimator : public IProcessingBlock
{
public:
    using Inputs  = SignalData;  // Received frequency-domain subcarriers (n_sc)
    using Outputs = SignalData;  // Estimated channel response H (n_sc)

    // Strongly typed enum for DMRS Comb configuration
    enum class CombPattern : uint8_t
    {
        Even = 0,
        Odd  = 1
    };

    struct Config
    {
        uint16_t    n_sc;
        CombPattern dmrs_comb;   // Explicitly defines the comb offset
        itpp::cvec  dmrs_seq;    // Known transmitted DMRS sequence
    };

    ChannelEstimator() = default;
    ~ChannelEstimator() override = default;

    ChannelEstimator(const ChannelEstimator&)            = delete;
    ChannelEstimator& operator=(const ChannelEstimator&) = delete;

    ChannelEstimator(ChannelEstimator&&)            = default;
    ChannelEstimator& operator=(ChannelEstimator&&) = default;

    void reset() override {};

    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg,
              const ExecContext& ctx);

private:
    // -------------------------------------------------------------------------
    // ls_estimate_()
    // Calculates Least Squares (LS) estimate H = Y * conj(X) at pilot indices.
    //
    // -------------------------------------------------------------------------
    itpp::cvec ls_estimate_(const itpp::cvec& rx_grid, 
                            const itpp::cvec& tx_dmrs, 
                            uint16_t n_sc, 
                            CombPattern comb);

    // -------------------------------------------------------------------------
    // interpolate_()
    // Linearly interpolates the channel estimates for non-pilot subcarriers.
    //
    // -------------------------------------------------------------------------
    void interpolate_(itpp::cvec& hest, uint16_t n_sc, CombPattern comb);

    // -------------------------------------------------------------------------
    // extrapolate_edges_()
    // Linearly extrapolates the channel response for edge subcarriers.
    //
    // -------------------------------------------------------------------------
    void extrapolate_edges_(itpp::cvec& hest, uint16_t n_sc, CombPattern comb);
};

} // namespace rx
} // namespace sam