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
/**
 * @class OfdmDemod
 * @brief Performs OFDM demodulation, including CP removal, FFT, and subcarrier extraction.
 * * This processing block converts time-domain IQ samples into frequency-domain 
 * subcarrier data. It supports front-end signal corrections, optional DC subcarrier 
 * nulling, and IDFT decoding for SC-FDMA (DFT-precoded) signals.
 */
class OfdmDemod : public IProcessingBlock
{
public:
    using Inputs  = SignalData;  // time-domain samples (n_fft + cp)
    using Outputs = SignalData;  // frequency-domain subcarriers (n_sc)

    struct Config
    {
        uint16_t n_fft; // FFT size
        uint16_t n_sc; // Number of subcarriers
        uint16_t cp; // cyclic prefix
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

    /**
     * @brief Transforms time-domain samples into frequency-domain resource grids.
     * * @param in   Input structure containing time-domain IQ samples.
     * @param out  Output structure to be populated with extracted subcarriers.
     * @param ctrl Control metadata for timing and frame synchronization.
     * @param cfg  Functional configuration for FFT, CP, and corrections.
     * @param ctx  Execution context for hardware-specific or environmental state.
     */
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