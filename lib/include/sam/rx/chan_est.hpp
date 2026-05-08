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
    struct Inputs
    {
        const SignalData* rx;
        const SignalData* dmrs;
    };

    using Outputs = SignalData;  // Estimated channel response H (n_sc)

    // Strongly typed enum for DMRS Pattern configuration
    enum class DMRSPattern : uint8_t
    {
        Even = 0,
        Odd  = 1
    };

    // TODO: See if we require dmrs_symbol_indices to identify if a symbol is dmrs or not (for 5g)
    struct Config
    {
        uint16_t    n_sc;
        DMRSPattern dmrs_pattern;
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
              const Config&      cfg);

private:
    // -------------------------------------------------------------------------
    // ls_estimate_()
    // Calculates Least Squares (LS) estimate H = Y * conj(X) at pilot indices.
    //
    // -------------------------------------------------------------------------
    itpp::cvec ls_estimate_(const itpp::cvec& rx_grid, 
                            const itpp::cvec& tx_dmrs, 
                            uint16_t n_sc, 
                            DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // average_adjacent_pilots_()
    // Averages adjacent pilot pairs to reduce independent noise variance by half 
    //
    // -------------------------------------------------------------------------
    void average_adjacent_pilots_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // interpolate_()
    // Linearly interpolates the channel estimates for non-pilot subcarriers.
    //
    // -------------------------------------------------------------------------
    void interpolate_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // extrapolate_edges_()
    // Linearly extrapolates the channel response for edge subcarriers.
    //
    // -------------------------------------------------------------------------
    void extrapolate_edges_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);
};

} // namespace rx
} // namespace sam