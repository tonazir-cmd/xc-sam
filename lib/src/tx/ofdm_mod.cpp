// =============================================================================
// ofdm_mod.cpp
// Xcelerium SAM Framework — OFDMMod Implementation
// =============================================================================

#include "sam/tx/ofdm_mod.h"

#include <itpp/signal/transforms.h> // itpp::ifft, itpp::fft
#include <itpp/base/vec.h>
#include <itpp/base/converters.h>

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace sam
{
namespace tx
{

void OFDMMod::configure(const Config &cfg)
{
    ifft_in_.set_size(cfg.n_fft, false);
    ifft_out_.set_size(cfg.n_fft, false);
    fd_scratch_.set_size(cfg.n_sc, false);
    if (cfg.n_win > 0) prev_tail_.set_size(cfg.n_win, false);
}

void OFDMMod::reset()
{
    // Zero-fill the scratch buffers.
    fd_scratch_.zeros();
    ifft_in_.zeros();
    ifft_out_.zeros();

    // Clean and reset the windowing states.
    prev_tail_.zeros();
}

void OFDMMod::eval(const Inputs &in,
                    Outputs &out,
                    const Control &ctrl,
                    const Config &cfg,
                    const ExecContext &ctx)
{
    // -------------------------------------------------------------------------
    // Control handling: enable / bypass
    // -------------------------------------------------------------------------
    if (!ctrl.enable) return;
    if (ctrl.bypass){out.samples = in.samples; return;}

    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    assert(cfg.n_sc % 12 == 0);
    assert(cfg.n_sc <= cfg.n_fft);

    if (cfg.n_win > 0)
    {
        assert(cfg.n_win <= cfg.cp);
        assert(cfg.win_leading != nullptr);
        assert(cfg.win_trailing != nullptr);
        assert(cfg.win_leading->size() == cfg.n_win);
        assert(cfg.win_trailing->size() == cfg.n_win);
        // We should not allow window resizing between symbols
        // as the previous tail would be of a different length than the new window size.
        if (ctx.symbol_idx > 0)  assert(prev_tail_.size() == cfg.n_win);
    }

    assert(in.samples.size() == cfg.n_sc);
    assert(out.samples.size() == cfg.n_fft + cfg.cp);
    
    assert(fd_scratch_.length() == cfg.n_sc);
    assert(ifft_in_.length() == cfg.n_fft);
    assert(ifft_out_.length() == cfg.n_fft);

    // -------------------------------------------------------------------------
    // Lazy resizing of scratch and state buffers.
    // -------------------------------------------------------------------------
    ifft_in_.set_size(cfg.n_fft, false);
    ifft_out_.set_size(cfg.n_fft, false);
    fd_scratch_.set_size(cfg.n_sc, false);
    if (cfg.n_win > 0 && ctx.symbol_idx == 0) prev_tail_.set_size(cfg.n_win, false);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------

    if (cfg.dft_precoding)
    {
        apply_dft_precoding_(in.samples, fd_scratch_);
        arrange_subcarriers_(fd_scratch_, ifft_in_, cfg.n_fft, cfg.n_sc);
    }
    else
    {
        arrange_subcarriers_(in.samples, ifft_in_, cfg.n_fft, cfg.n_sc);
    }

    itpp::ifft(ifft_in_, ifft_out_);

    add_cyclic_prefix_(ifft_out_, out.samples, cfg.n_fft, cfg.cp);

    if (cfg.n_win > 0)
    {
        apply_windowing_(out.samples, *cfg.win_leading, *cfg.win_trailing, cfg.cp, cfg.n_win, ctx.symbol_idx);
    }
}

// =============================================================================
// Private Helpers
// =============================================================================

void OFDMMod::apply_dft_precoding_(const itpp::cvec &in_sc,
                                    itpp::cvec &out_sc)
{
    // TODO: Verify this implementation
    out_sc = itpp::fft(in_sc);
    out_sc /= std::sqrt(static_cast<double>(in_sc.size()));
}

void OFDMMod::arrange_subcarriers_(const itpp::cvec &sc_data,
                                    itpp::cvec &ifft_in,
                                    uint16_t n_fft, uint16_t n_sc)
{
    const int half = n_sc / 2; // positive-negative frequency half
    
    ifft_in.zeros();

    for (int i = 0; i < half; ++i)
    {
        ifft_in[n_fft - half + i] = sc_data[i]; // negative frequencies
        ifft_in[i] = sc_data[half + i];         // positive frequencies
    }

}

void OFDMMod::add_cyclic_prefix_(const itpp::cvec &time_sym,
                                    itpp::cvec &sym_with_cp,
                                    uint16_t n_fft, uint16_t cp)
{
    for (int i = 0; i < cp; ++i) sym_with_cp[i] = time_sym[n_fft - cp + i];
    for (int i = 0; i < n_fft; ++i) sym_with_cp[cp + i] = time_sym[i];
}

void OFDMMod::apply_windowing_(itpp::cvec &sym_with_cp,
                              itpp::vec &win_leading,
                              itpp::vec &win_trailing,
                              uint16_t cp, uint16_t n_win, uint16_t symbol_idx)
{
    for (int i = 0; i < n_win; ++i) sym_with_cp[i] *= win_leading[i];

    if (symbol_idx > 0)
    {
        for (int i = 0; i < n_win; ++i) sym_with_cp[i] += prev_tail_[i] * win_trailing[i];
    }

    for (int i = 0; i < n_win; i++) prev_tail_[i] = sym_with_cp[cp + i];
}

} // namespace tx
} // namespace sam