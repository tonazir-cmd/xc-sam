#include "sam/rx/ofdm_demod.hpp"

#include <itpp/signal/transforms.h>
#include <itpp/base/vec.h>

#include <cassert>
#include <cmath>

namespace sam
{
namespace rx
{

void OFDMDemod::eval(const Inputs&      in,
                     Outputs&           out,
                     const Control&     ctrl,
                     const Config&      cfg,
                     const ExecContext& ctx)
{
    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------
    if (!ctrl.enable) return;
    if (ctrl.bypass) { out.samples = in.samples; return; }

    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    assert(cfg.n_sc % 12 == 0);
    assert(cfg.n_sc <= cfg.n_fft);

    assert(in.samples.size() == cfg.n_fft + cfg.cp);
    assert(out.samples.size() == cfg.n_sc);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------

    // Time-Domain Corrections (CP removal, Frequency offset, Phase, Gain)
    itpp::cvec fft_in = apply_front_end_corrections_(in.samples, cfg, ctx);
    itpp::cvec fft_out = itpp::fft(fft_in);

    if (cfg.dft_precoding)
    {
        out.samples = extract_subcarriers_(fft_out, cfg.n_fft, cfg.n_sc, cfg.dc);
        out.samples = apply_idft_precoding_(out.samples);
    }
    else
    {
        out.samples = extract_subcarriers_(fft_out, cfg.n_fft, cfg.n_sc, cfg.dc);
    }
}

// =============================================================================
// Private Helpers
// =============================================================================

itpp::cvec OFDMDemod::apply_front_end_corrections_(const itpp::cvec& in_samples,
                                             const Config& cfg,
                                             const ExecContext& ctx)
{
    // val * gain * exp(j * (2 * pi * f_o * t + phase))
    
    const size_t num_samples = in_samples.length();
    const double global_history_phase = cfg.fo * ctx.sample_count;

    itpp::vec indices = itpp::linspace(0, num_samples - 1, num_samples);
    itpp::vec total_phase_offset = (cfg.fo * indices) + global_history_phase + cfg.phase;
    itpp::cvec phasor_correction = cfg.gain * itpp::exp(std::complex<double>(0, 1) * (itpp::m_2pi * total_phase_offset));
    itpp::cvec rotated_samples = itpp::elem_mult(in_samples, phasor_correction);

    // To be removed // added when windowing method is decided
    if (cfg.curr_sym_windowing) return rotated_samples.mid(cfg.cp, cfg.n_fft);

    itpp::cvec output(cfg.n_fft);
    const size_t half_cp = cfg.cp / 2;
    output.set_subvector(0, rotated_samples.mid(cfg.cp, cfg.n_fft - half_cp));
    output.set_subvector(cfg.n_fft - half_cp, rotated_samples.mid(half_cp, half_cp));
    return output;
}

itpp::cvec OFDMDemod::extract_subcarriers_(const itpp::cvec& fft_out,
                                     uint16_t n_fft, uint16_t n_sc, bool dc)
{
    const int half = n_sc / 2;
    const int pos_start_idx = dc ? 1 : 0; // incase of dc nulling, start positive frequencies from index 1 instead of 0
    
    itpp::cvec sc_data(n_sc);
    sc_data.set_subvector(0, fft_out.mid(n_fft - half, half)); // Negative frequencies
    sc_data.set_subvector(half, fft_out.mid(pos_start_idx, half)); // Positive frequencies
    return sc_data;
}

itpp::cvec OFDMDemod::apply_idft_precoding_(const itpp::cvec& in_sc)
{
    itpp::cvec out_sc = itpp::ifft(in_sc);
    out_sc *= std::sqrt(static_cast<double>(in_sc.size()));
    return out_sc;
}

} // namespace rx
} // namespace sam