#include "sam/rx/ofdm_irx.hpp"

namespace sam
{
namespace rx
{
template<size_t N_RX, size_t N_LAYERS>
void OfdmIRX<N_RX, N_LAYERS>::eval(const Inputs&            in,
                                         Outputs&           out,
                                         const Control&     ctrl,
                                         const Config&      cfg,
                                         const ExecContext& ctx)
{
    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------
    if (!ctrl.irx.enable) {
        out.valid = false;
        return;
    }
    if (ctrl.irx.bypass) return;
    
    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------
    out.valid = false;

    if (ctx.symbol_idx < N_SYM)
    {
    // 1. OFDM Demodulation
    for (size_t rx = 0; rx < N_RX; rx++)
        demod_[rx].eval(*in.rx[rx], rx_grid_[rx][ctx.symbol_idx], ctrl.demod[rx], cfg.demod, ctx);

    // 2. Channel Estimation
    if (cfg.is_pilot_sym[ctx.symbol_idx])
        for (size_t rx = 0; rx < N_RX; rx++)
            for (size_t tx = 0; tx < N_LAYERS; tx++)
            {
                ChannelEstimator::Inputs chan_eq_in;
                chan_eq_in.rx = &rx_grid_[rx][ctx.symbol_idx];
                chan_eq_in.dmrs = in.dmrs[tx];
                chan_est_.eval(chan_eq_in, h_[rx][tx], ctrl.chan_est[rx][tx], cfg.chan_est[tx]);
            }
    }

    // 3. Channel Equalization & Demapping
    if (ctx.symbol_idx >= cfg.process_delay)
    {
        size_t d_sym_idx = ctx.symbol_idx - cfg.process_delay;
        if (cfg.is_pilot_sym[d_sym_idx]) return;

        typename ChanEqDemap<N_RX, N_LAYERS>::Inputs chan_eq_demap_in;

        for (size_t rx = 0; rx < N_RX; rx++)
            chan_eq_demap_in.rx_grid[rx] = &rx_grid_[rx][d_sym_idx];
        
        for (size_t rx = 0; rx < N_RX; rx++)
            for (size_t tx = 0; tx < N_LAYERS; tx++)
                chan_eq_demap_in.hp[rx][tx] = &h_[rx][tx];
        
        typename ChanEqDemap<N_RX, N_LAYERS>::Outputs chan_eq_demap_out;
        for (size_t tx = 0; tx < N_LAYERS; tx++)
            chan_eq_demap_out.llrs[tx] = out.llrs[tx];

        chan_eq_demap_.eval(chan_eq_demap_in, chan_eq_demap_out, ctrl.chan_eq_demap, cfg.chan_eq_demap);
        out.valid = true;
    };
}


template<size_t N_RX, size_t N_LAYERS>
void OfdmIRX<N_RX, N_LAYERS>::reset()
{
    for (size_t rx = 0; rx < N_RX; rx++) for (size_t sym = 0; sym < N_SYM; sym++) rx_grid_[rx][sym].samples.set_size(0);
    for (size_t rx = 0; rx < N_RX; rx++) for (size_t tx = 0; tx < N_LAYERS; tx++) h_[rx][tx].samples.set_size(0);

    for (size_t rx = 0; rx < N_RX; rx++) demod_[rx].reset();
    chan_est_.reset();
    chan_eq_demap_.reset();
}

template class OfdmIRX<2,2>;
}
}