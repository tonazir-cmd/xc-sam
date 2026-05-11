#include "sam/rx/ofdm_irx.hpp"

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
template<size_t N_RX, size_t N_LAYERS>
void OfdmIRX<N_RX, N_LAYERS>::eval(const Inputs&            in,
                                         Outputs&           out,
                                         const Control&     ctrl,
                                         const Config&      cfg,
                                         const ExecContext& ctx)
{
    // Output flag, only set to valid if channel equalization demap result writes to it.
    out.valid = false;

    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------
    if (!ctrl.irx.enable || ctrl.irx.bypass) return;
    
    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    assert(ctx.symbol_idx < N_SYM);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------
    

    // ----- 1. OFDM Demodulation
    for (size_t rx = 0; rx < N_RX; rx++)
        demod_.eval(*in.rx[rx], rx_grid_[rx][ctx.symbol_idx], ctrl.demod, cfg.demod, ctx);

    // ----- 2. Channel Estimation
    // Only do in-case of a pilot symbol
    if (cfg.is_pilot_sym[ctx.symbol_idx])
        for (size_t rx = 0; rx < N_RX; rx++)
            for (size_t layer = 0; layer < N_LAYERS; layer++)
            {
                ChannelEstimator::Inputs chan_eq_in;
                chan_eq_in.rx = &rx_grid_[rx][ctx.symbol_idx];
                chan_eq_in.dmrs = in.dmrs[layer];

                chan_est_.eval(chan_eq_in, h_[rx][layer], ctrl.chan_est, cfg.chan_est[layer]);
            }


    // ----- 3. Channel Equalization & Demapping
    // delay output by process_delay symbols and return without equalization (out.valid = false)
    if (symbols_delayed++ < cfg.process_delay) return;

    // Apply process delay to the current symbol index, wrapping around the N_SYM boundary.
    size_t d_sym_idx = (((int)ctx.symbol_idx - (int)cfg.process_delay) % N_SYM + 14) % N_SYM;

    // here if (ctx.symbol_idx - cfg.process_delay) < 0, 
    // we should also reduce slot_id by 1, if it is being used below
    // currently it is not being used, hence no need

    // incase of pilot symbol, no need to process, no data in pilot symbol (for 5G)
    if (cfg.is_pilot_sym[d_sym_idx]) return;

    // Populate Input
    typename ChanEqDemap<N_RX, N_LAYERS>::Inputs chan_eq_demap_in;
    for (size_t rx = 0; rx < N_RX; rx++) chan_eq_demap_in.rx_grid[rx] = &rx_grid_[rx][d_sym_idx];
    for (size_t rx = 0; rx < N_RX; rx++) for (size_t layer = 0; layer < N_LAYERS; layer++) chan_eq_demap_in.hp[rx][layer] = &h_[rx][layer];
    
    // Populate Output
    typename ChanEqDemap<N_RX, N_LAYERS>::Outputs chan_eq_demap_out;
    for (size_t layer = 0; layer < N_LAYERS; layer++) chan_eq_demap_out.llrs[layer] = out.llrs[layer];

    // Evaluate
    chan_eq_demap_.eval(chan_eq_demap_in, chan_eq_demap_out, ctrl.chan_eq_demap, cfg.chan_eq_demap);

    // Set output to valid, as channel eq will have updated output
    out.valid = true;
}


template<size_t N_RX, size_t N_LAYERS>
void OfdmIRX<N_RX, N_LAYERS>::reset()
{
    for (size_t rx = 0; rx < N_RX; rx++) for (size_t sym = 0; sym < N_SYM; sym++) rx_grid_[rx][sym].samples.set_size(0);
    for (size_t rx = 0; rx < N_RX; rx++) for (size_t layer = 0; layer < N_LAYERS; layer++) h_[rx][layer].samples.set_size(0);

    demod_.reset();
    chan_est_.reset();
    chan_eq_demap_.reset();
    symbols_delayed = 0;
}

template class OfdmIRX<2,2>;
}
}