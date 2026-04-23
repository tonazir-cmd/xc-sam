#include "sam/rx/ofdm_demod.h"

#include <itpp/signal/transforms.h>
#include <itpp/base/vec.h>

#include <cassert>
#include <cmath>

namespace sam
{
namespace rx
{

void OFDMDemod::configure(const Config& cfg)
{
    fft_in_.set_size(cfg.n_fft, false);
    fft_out_.set_size(cfg.n_fft, false);
    fd_scratch_.set_size(cfg.n_sc, false);

    reset();
}

void OFDMDemod::reset()
{
    fft_in_.zeros();
    fft_out_.zeros();
    fd_scratch_.zeros();
}

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

    assert(fft_in_.size()   == cfg.n_fft);
    assert(fft_out_.size()  == cfg.n_fft);
    if (cfg.dft_precoding) assert(fd_scratch_.size() == cfg.n_sc);

    // -------------------------------------------------------------------------
    // Lazy resizing of scratch and state buffers.
    // -------------------------------------------------------------------------
    fft_in_.set_size(cfg.n_fft, false);
    fft_out_.set_size(cfg.n_fft, false);
    fd_scratch_.set_size(cfg.n_sc, false);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------

    // Time-Domain Corrections (CP removal, Frequency offset, Phase, Gain)
    apply_front_end_corrections_(in.samples, fft_in_, cfg, ctx);

    itpp::fft(fft_in_, fft_out_);

    if (cfg.dft_precoding)
    {
        extract_subcarriers_(fft_out_, fd_scratch_, cfg.n_fft, cfg.n_sc, cfg.dc);
        apply_idft_precoding_(fd_scratch_, out.samples);
    }
    else
    {
        extract_subcarriers_(fft_out_, out.samples, cfg.n_fft, cfg.n_sc, cfg.dc);
    }
}

// =============================================================================
// Private Helpers
// =============================================================================

void OFDMDemod::apply_front_end_corrections_(const itpp::cvec& in_samples,
                                             itpp::cvec& fft_in,
                                             const Config& cfg,
                                             const ExecContext& ctx)
{
    
    // val * gain * exp(j * (2 * pi * f_o * t + phase))

    const double ts = 1.0 / cfg.sample_rate;
    
    if (cfg.freq_offset == 0.0 && cfg.phase == 0.0)
    {
        for (int i = 0; i < cfg.n_fft; ++i)
            fft_in[i] = in_samples[cfg.cp + i] * cfg.gain;
        return;
    }
    else if (cfg.freq_offset == 0.0)
    {
        std::complex<double> phase_rotation(std::cos(cfg.phase), std::sin(cfg.phase));
        for (int i = 0; i < cfg.n_fft; ++i)
        {
            int src_idx = cfg.cp + i;
            fft_in[i] = in_samples[src_idx] * cfg.gain * phase_rotation;
        }
        return;
    }
    else
    {
        for (int i = 0; i < cfg.n_fft; ++i)
        {
            int src_idx = cfg.cp + i;
            double t = (ctx.sample_count + src_idx) * ts;
            double theta = (2.0 * itpp::pi * cfg.freq_offset * t) + cfg.phase;
            std::complex<double> rotation(std::cos(theta), std::sin(theta));
            
            fft_in[i] = in_samples[src_idx] * cfg.gain * rotation;
        }
    }
}

void OFDMDemod::extract_subcarriers_(const itpp::cvec& fft_out,
                                     itpp::cvec& sc_data,
                                     uint16_t n_fft, uint16_t n_sc, uint16_t dc_offset)
{
    const int half = n_sc / 2;

    for (int i = 0; i < half; i++)
    {
        sc_data[i] = fft_out[n_fft - half + i]; // negative frequencies
        sc_data[half + i] = fft_out[dc_offset + i]; // positive frequencies
    }
}

void OFDMDemod::apply_idft_precoding_(const itpp::cvec& in_sc,
                                      itpp::cvec& out_sc)
{
    out_sc = itpp::ifft(in_sc);
    out_sc *= std::sqrt(static_cast<double>(in_sc.size()));
}

} // namespace rx
} // namespace sam