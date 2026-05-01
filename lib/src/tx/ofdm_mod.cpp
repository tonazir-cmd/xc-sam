// =============================================================================
// ofdm_mod.cpp
// Xcelerium SAM Framework — OFDMMod Implementation
// =============================================================================

#include "sam/tx/ofdm_mod.hpp"

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

void OFDMMod::reset()
{
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
    assert(cfg.n_win <= cfg.cp);
    
    if (cfg.n_win > 0 && ctx.symbol_idx > 0) assert(prev_tail_.size() == cfg.n_win);

    assert(in.samples.size() == cfg.n_sc);
    assert(out.samples.size() == cfg.n_fft + cfg.cp);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------

    itpp::cvec subcarriers = (cfg.dft_precoding) ? apply_dft_precoding_(in.samples) : in.samples;
    itpp::cvec ifft_in = arrange_subcarriers_(subcarriers, cfg.n_fft, cfg.n_sc, cfg.dc);
    itpp::cvec ifft_out = itpp::ifft(ifft_in);
    out.samples = add_cyclic_prefix_(ifft_out, cfg.n_fft, cfg.cp);
    if (cfg.n_win > 0) out.samples = apply_windowing_(out.samples, cfg.cp, cfg.n_win, ctx.symbol_idx);
}

// =============================================================================
// Private Helpers
// =============================================================================

itpp::cvec OFDMMod::apply_dft_precoding_(const itpp::cvec &in_sc)
{
    return itpp::fft(in_sc) / std::sqrt(static_cast<double>(in_sc.size()));
}

itpp::cvec OFDMMod::arrange_subcarriers_(const itpp::cvec &sc_data,
                                         uint16_t n_fft, uint16_t n_sc, bool dc)
{
    const int half = n_sc / 2; // positive-negative frequency half
    int pos_start_idx = dc ? 1 : 0; // incase of dc nulling, start positive frequencies from index 1 instead of 0
    itpp::cvec ifft_in(n_fft);
    ifft_in.zeros();
    ifft_in.set_subvector(n_fft - half, sc_data.mid(0, half)); // negative frequencies
    ifft_in.set_subvector(pos_start_idx, sc_data.mid(half, half)); // positive frequencies
    return ifft_in;
}

itpp::cvec OFDMMod::add_cyclic_prefix_(const itpp::cvec &time_sym,
                                    uint16_t n_fft, uint16_t cp)
{
    itpp::cvec sym_with_cp(n_fft + cp);
    sym_with_cp.set_subvector(0, time_sym.mid(n_fft - cp, cp)); // cyclic prefix
    sym_with_cp.set_subvector(cp, time_sym); // useful symbol
    return sym_with_cp;
}

itpp::cvec OFDMMod::apply_windowing_(const itpp::cvec &sym_with_cp,
                                    uint16_t cp, uint16_t n_win, uint16_t symbol_idx)
{
    itpp::vec cos_curve = itpp::cos(itpp::pi * itpp::linspace(0, n_win - 1, n_win) / n_win);
    itpp::cvec win_leading  = itpp::to_cvec(0.5 * (1.0 - cos_curve)); // 0 → 1
    itpp::cvec win_trailing = itpp::to_cvec(0.5 * (1.0 + cos_curve)); // 1 → 0

    itpp::cvec windowed_sym = sym_with_cp;
    itpp::cvec leading_edge = itpp::elem_mult(sym_with_cp.mid(0, n_win), win_leading);
    windowed_sym.set_subvector(0, leading_edge);

    if (symbol_idx > 0)
    {
        itpp::cvec overlap = itpp::elem_mult(prev_tail_, win_trailing);
        windowed_sym.set_subvector(0, windowed_sym.mid(0, n_win) + overlap);
    }

    prev_tail_ = sym_with_cp.mid(cp, n_win);
    return windowed_sym;
}

} // namespace tx
} // namespace sam