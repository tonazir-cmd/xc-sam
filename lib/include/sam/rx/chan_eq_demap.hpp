#pragma once

#include "sam/core/base.hpp"
#include "sam/core/signal_types.hpp"

#include <itpp/itbase.h>
#include <itpp/comm/modulator.h>

#include <cstdint>
#include <cassert>
#include <vector>

namespace sam
{
namespace rx
{

/**
 * @class ChanEqDemap
 * @brief Performs Joint Channel Equalization and Soft Demapping for MIMO-OFDM systems.
 *
 * This processing block evaluates received OFDM grids across multiple receive antennas 
 * to estimate transmitted bits. It applies the combined N_RX x N_LAYERS channel 
 * matrix (H) to the received signals to perform spatial equalization, and subsequently 
 * computes Log-Likelihood Ratios (LLRs) based on the specified QAM constellation.
 *
 * @tparam N_RX     Number of receive antennas.
 * @tparam N_LAYERS Number of spatial transmission layers.
 *
 * @note Implementation Detail: This class utilizes raw pointer arrays within the 
 * Inputs and Outputs structures to interface with external signal buffers. 
 * Callers must ensure that the source buffers (e.g., from ChanEst or OfdmDemod) 
 * remain in scope and outlive the execution of the eval() method.
 */
template<size_t N_RX, size_t N_LAYERS>
class ChanEqDemap : public IProcessingBlock
{
public:
    struct Inputs
    {
        // 2D array of pointers to the channel estimates (H) provided by ChanEst.
        // Dimensions: [N_RX][N_LAYERS]. Requires one estimate per Rx/Layer pair.
        sam::SignalData* hp[N_RX][N_LAYERS];

        //1D array of pointers to the received resource grids from OfdmDemod.
        // Dimensions: [N_RX]. Requires one resource grid per rx antenna.
        sam::SignalData* rx_grid[N_RX];
    };

    struct Outputs
    {
        // 1D array of pointers to the output LLR (Log-Likelihood Ratio) buffers.
        // Dimensions: [N_LAYERS]. Generates one LLR buffer per tx layer.
        sam::RealData* llrs[N_LAYERS];
    };

    struct Config
    {
        uint16_t n_sc;    ///< Number of active subcarriers.
        uint8_t  qm_mode; ///< Modulation order indicator (QAM constellation size = 2^qm_mode).
        double   n_var;   ///< Estimated noise variance.
    };

    ChanEqDemap() = default;
    ~ChanEqDemap() override = default;

    ChanEqDemap(const ChanEqDemap&)            = delete;
    ChanEqDemap& operator=(const ChanEqDemap&) = delete;

    ChanEqDemap(ChanEqDemap&&)            = default;
    ChanEqDemap& operator=(ChanEqDemap&&) = default;

    void reset() override {};
    
    /**
     * @brief Executes the primary equalization and demapping processing loop.
     *
     * @param in   Input structure containing H-matrix estimates and received OFDM grids.
     * @param out  Output structure to be populated with computed LLRs.
     * @param ctrl Control signals containing timing and frame metadata.
     * @param cfg  Operational configuration, including subcarrier count and noise variance.
     */
    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg);
};

} // namespace rx
} // namespace sam