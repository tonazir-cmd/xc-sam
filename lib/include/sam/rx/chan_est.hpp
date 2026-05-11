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
/*
--- REMOVE THIS COMMENT AFTER REVIEW
We are using pointer array (*) instead of reference (&) as
reference arrays are not allowed, and these will be assinged
from a bigger array. Either use pure vectors or create new
signal type of greater dimension and create the outer bigger array
of those types and then use reference to them here
*/

    // Inputs for ChannelEstimator::eval
    struct Inputs
    {
        const SignalData* rx; ///< Pointer to the rx symbol containing recieved dmrs
        const SignalData* dmrs; ///< Pointer to the expected dmrs symbol
    };

    using Outputs = SignalData;  // Estimated channel response H (n_sc)

    // Strongly typed enum for DMRS Pattern configuration
    enum class DMRSPattern : uint8_t
    {
        Even = 0,   ///< Pilots located on even-indexed subcarriers {0, 2, 4...}
        Odd  = 1    ///< Pilots located on odd-indexed subcarriers {1, 3, 5...}
    };

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
    
    /**
     * @brief Executes the estimation pipeline: LS, Averaging, and Interpolation.
     *
     * @param in   Input structures containing the received grid and reference pilots.
     * @param out  Output buffer to store the interpolated channel coefficients.
     * @param ctrl Control metadata for frame and symbol timing.
     * @param cfg  Configuration parameters including subcarrier count and DMRS pattern.
     */
    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg);

private:
    // -------------------------------------------------------------------------
    // Calculates Least Squares (LS) estimate H = Y * conj(X) at pilot indices.
    //
    // -------------------------------------------------------------------------
    itpp::cvec ls_estimate_(const itpp::cvec& rx_grid, 
                            const itpp::cvec& tx_dmrs, 
                            uint16_t n_sc, 
                            DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // Averages adjacent pilot pairs to reduce independent noise variance by half 
    //
    // -------------------------------------------------------------------------
    void average_adjacent_pilots_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // Linearly interpolates the channel estimates for non-pilot subcarriers.
    //
    // -------------------------------------------------------------------------
    void interpolate_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);

    // -------------------------------------------------------------------------
    // Linearly extrapolates the channel response for edge subcarriers.
    //
    // -------------------------------------------------------------------------
    void extrapolate_edges_(itpp::cvec& hest, uint16_t n_sc, DMRSPattern dmrs_pattern);
};

} // namespace rx
} // namespace sam