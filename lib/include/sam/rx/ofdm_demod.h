#pragma once

#include "sam/core/base.h"
#include "sam/core/signal_types.h"

#include <itpp/signal/transforms.h>
#include <itpp/base/vec.h>

#include <cstdint>
#include <cassert>

namespace sam
{
namespace rx
{

class OFDMDemod : public IProcessingBlock
{
public:
    using Inputs  = SignalData;  // time-domain samples (n_fft + cp)
    using Outputs = SignalData;  // frequency-domain subcarriers (n_sc)

    struct Config
    {
        uint16_t n_fft;
        uint16_t n_sc;
        uint16_t cp;
        bool dft_precoding; // true for LTE

        // Front-end signal correction parameters
        bool dc;            // enable dc subcarrier nulling (true for LTE)
        double phase;       // static phase rotation (radians)
        double fo;          // frequency offset (Hz)
        size_t to;           // time offset (samples) for time delay compensation  
        double gain;        // amplitude scaling gain
        double sample_rate; // sample rate (Hz)
    };

    OFDMDemod() = default;
    ~OFDMDemod() override = default;

    OFDMDemod(const OFDMDemod&)            = delete;
    OFDMDemod& operator=(const OFDMDemod&) = delete;

    OFDMDemod(OFDMDemod&&)            = default;
    OFDMDemod& operator=(OFDMDemod&&) = default;

    void reset() override {};

    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg,
              const ExecContext& ctx);

private:
    // -------------------------------------------------------------------------
    // apply_front_end_corrections_()
    // Performs CP removal with Gain, Phase, and Frequency offset.
    // -------------------------------------------------------------------------
    itpp::cvec apply_front_end_corrections_(const itpp::cvec& in_samples,
                                            const Config& cfg,
                                            const ExecContext& ctx);

    // -------------------------------------------------------------------------
    // extract_subcarriers_()
    // Pulls n_sc subcarriers back out of the FFT output bins and nulls DC.
    // -------------------------------------------------------------------------
    itpp::cvec  extract_subcarriers_(const itpp::cvec& fft_out,
                              uint16_t n_fft, uint16_t n_sc, bool dc);

    // -------------------------------------------------------------------------
    // apply_idft_precoding_()
    // Inverse of DFT precoding applied at TX (LTE/5G NR UL only).
    // -------------------------------------------------------------------------
    itpp::cvec apply_idft_precoding_(const itpp::cvec& in_sc);
};

} // namespace rx
} // namespace sam