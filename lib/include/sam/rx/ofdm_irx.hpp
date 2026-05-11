#pragma once

#include "sam/core/base.hpp"
#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"

#include "sam/rx/ofdm_demod.hpp"
#include "sam/rx/chan_est.hpp"
#include "sam/rx/chan_eq_demap.hpp"

#include <itpp/itbase.h>

namespace sam 
{
namespace rx
{ 
/*
--- REMOVE THIS COMMENT AFTER REVIEW
- Given Template in doc
  template <size_t N_RX, size_t N_TX, size_t CH_EST_MAX_DELAY, size_t CH_EST_BUFF_SIZE>
  void OfdmIRX<N_RX, N_TX, T_FD, T_TD, T_Dec>::eval

- Reasons Different Template is being used for now:
  1- We need N_LAYERS for the Channel Equalization and Demapper class instead of N_TX
  2- T_FD, T_TD, T_Dec are written in class instead of CH_EST_MAX_DELAY, CH_EST_BUFF_SIZE 
     which is in template, making it confusing which one to use
*/

/**
 * @class OfdmIRX
 * @brief Integrated OFDM Receiver Top-Level Block.
 * * @details Orchestrates the complete physical layer receive chain, encapsulating 
 * OFDM demodulation, multi-layer channel estimation, and joint MIMO equalization/demapping.
 * The block manages internal state for signal grids and handles processing delays 
 * required for stable channel estimation.
 * * @tparam N_RX Number of receive antennas.
 * @tparam N_LAYERS Number of spatial transmission layers.
 */
template <size_t N_RX, size_t N_LAYERS>
class OfdmIRX : public IProcessingBlock 
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
    struct Inputs
    {
        const SignalData* rx[N_RX]; ///< Array of pointers to time-domain receive buffers for each antenna.
        const SignalData* dmrs[N_LAYERS]; ///< Array of pointers to reference DMRS sequences for each Tx layer.
    };

    struct Outputs
    {
        RealData* llrs [N_LAYERS]; ///< Array of pointers to output LLR buffers for each Tx layer.
        bool valid = false; ///< Validity flag indicating if the current execution produced valid LLRs.
    };

    struct Control
    {
        sam::Control irx;           ///< Controls for this class .
        sam::Control demod;         ///< Controls for OFDM Demodulator.
        sam::Control chan_est;      ///< Controls for Channel Estimation.
        sam::Control chan_eq_demap; ///< Controls for Channel Equalization and Demapper.
    };
    
    struct Config
    {
        OfdmDemod::Config demod;                                    ///< Parameters for OFDM demodulation.
        ChannelEstimator::Config chan_est[N_LAYERS];                ///< Per-layer estimation settings.
        typename ChanEqDemap<N_RX, N_LAYERS>::Config chan_eq_demap; ///< MIMO equalizer and demapper settings.
        size_t process_delay = 0;                                   ///< Symbol-level processing offset.
        bool is_pilot_sym[N_SYM] = {false};                         ///< Pilot symbol mask for the current slot.
    };

    OfdmIRX() = default;
    ~OfdmIRX() override = default;

    OfdmIRX(const OfdmIRX&)            = delete;
    OfdmIRX& operator=(const OfdmIRX&) = delete;

    OfdmIRX(OfdmIRX&&)            = default;
    OfdmIRX& operator=(OfdmIRX&&) = default;
    
    /**
     * @brief Resets the internal state of the receiver and all sub-components.
     */
    void reset();
    
    /**
     * @brief Executes the integrated receiver processing chain.
     * * @param in   Input structures for receive signals and pilots.
     * @param out  Output structures for LLRs and validity status.
     * @param ctrl Hierarchical control signals.
     * @param cfg  Operational configuration for all sub-blocks.
     * @param ctx  Execution context for environment-specific parameters.
     */
    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg,
              const ExecContext& ctx);

private:
    OfdmDemod demod_;
    ChannelEstimator chan_est_;
    ChanEqDemap<N_RX, N_LAYERS> chan_eq_demap_;
    
    // Ofdm Demod output stored as internal grid state
    // This is necessary for process delay logic
    SignalData rx_grid_[N_RX][N_SYM];
    
    // Channel Estimation output
    SignalData h_[N_RX][N_LAYERS];

    // Variable for skipping chan eq till process delay
    size_t symbols_delayed = 0;
};

}
}