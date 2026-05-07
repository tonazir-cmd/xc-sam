#pragma once

#include "sam/core/base.hpp"
#include "sam/core/signal_types.hpp"

#include <itpp/signal/transforms.h>
#include <itpp/base/vec.h>

#include <cstdint>
#include <cassert>

namespace sam
{
namespace rx
{

class OfdmDemod : public IProcessingBlock
{
public:
    using Inputs  = SignalData;  // time-domain samples (n_fft + cp)
    using Outputs = SignalData;  // frequency-domain subcarriers (n_sc)

    struct Config
    {
        uint16_t n_fft;
        uint16_t n_sc;
        uint16_t cp;
        bool dft_precoding; // enable idft decoding

        // Front-end signal correction parameters
        bool dc;      // enable dc subcarrier nulling (true for LTE)
        double phase; // normalized static phase rotation
        double fo;    // normalized frequency offset
        double gain;  // amplitude scaling gain

        // Temp var, will be removed once windowing method is decided
        bool curr_sym_windowing = false;
    };

    OfdmDemod() = default;
    ~OfdmDemod() override = default;

    OfdmDemod(const OfdmDemod&)            = delete;
    OfdmDemod& operator=(const OfdmDemod&) = delete;

    OfdmDemod(OfdmDemod&&)            = default;
    OfdmDemod& operator=(OfdmDemod&&) = default;

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