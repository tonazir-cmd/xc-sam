// =============================================================================
// ofdm_mod.h
// Xcelerium SAM Framework — OFDM Modulator Block
//
// OFDMMod transforms frequency-domain subcarrier data into a time-domain
// OFDM symbol stream ready for the Digital Front End (DFE / DUC).
//
// Processing Unit: ONE OFDM Symbol per eval() call.
//
// Processing Steps (in order):
//   1. Transform Precoding  — optional DFT spreading (LTE)
//   2. IFFT                 — frequency → time domain conversion
//   3. CP Addition          — prepend last cp samples to mitigate ISI
//   4. Windowing/Filtering  — smooth symbol edges to reduce OOB emissions
//
// State (private member variables):
//   prev_tail_  — tail samples of the previous symbol used for overlap-add
//                 windowing. Cleared by reset().
//   first_symbol_ — true until the first eval() call completes; controls
//                 whether prev_tail_ is added to the current symbol or not.
//
// Zero-Copy Contract:
//   Inputs and Outputs are passed as const/mutable references. The ITPP
//   containers within them must be non-owning views in the call site.
//   Internal scratch buffers are held as member variables to avoid
//   per-call heap allocation.
// =============================================================================

#pragma once

#include "sam/core/base.h"
#include "sam/core/signal_types.h"

#include <itpp/itbase.h>

#include <cstdint>
#include <cassert>

namespace sam {
namespace tx {

// =============================================================================
// class OFDMMod
//
// Inherits: sam::IProcessingBlock
// =============================================================================
class OFDMMod : public IProcessingBlock
{
public:
    // -------------------------------------------------------------------------
    // Framework-standard type aliases
    //   Inputs  — frequency-domain subcarrier data (SignalData::samples is a
    //             non-owning cvec view of n_sc complex symbols)
    //   Outputs — time-domain OFDM symbol with CP  (n_fft + cp samples)
    // -------------------------------------------------------------------------
    using Inputs  = SignalData;
    using Outputs = SignalData;

    // -------------------------------------------------------------------------
    // Config — block-local configuration struct.
    //
    // Fields:
    //   n_fft         — FFT/IFFT size (e.g. 512, 1024, 2048, 4096)
    //   n_sc          — number of active (occupied) subcarriers; must be ≤ n_fft
    //   cp            — cyclic prefix length in samples
    //   n_win         — windowing ramp length (samples per edge); 0 = disabled
    //   dft_precoding — enable or disbale dft precoding (true for LTE)
    //
    // Caller can pass it through the standard eval() interface.
    //
    // -------------------------------------------------------------------------
    struct Config
    {
        uint16_t n_fft  = 2048;   // IFFT size
        uint16_t n_sc   = 1200;   // active subcarriers (must be ≤ n_fft)
        uint16_t cp     = 144;    // cyclic prefix length [samples]
        uint16_t n_win  = 0;      // windowing ramp length [samples]; 0 = none
        bool     dft_precoding = false; // true for LTE
    };

    // -------------------------------------------------------------------------
    // Constructors / lifecycle
    // -------------------------------------------------------------------------
    OFDMMod() = default;
    ~OFDMMod() override = default;

    // Deleted to prevent accidental deep-copies of large internal buffers and passing by value.
    // Prevents OFDMMod a = b;  (copy construction) and OFDMMod a; a = b;  (copy assignment).
    OFDMMod(const OFDMMod&)            = delete;
    OFDMMod& operator=(const OFDMMod&) = delete;

    // Move or passing by reference is allowed.
    OFDMMod(OFDMMod&&)            = default;
    OFDMMod& operator=(OFDMMod&&) = default;

    // -------------------------------------------------------------------------
    // reset()
    //
    // Clears all internal states.
    //
    // -------------------------------------------------------------------------
    void reset() override;

    // -------------------------------------------------------------------------
    // eval()
    //
    // Performs OFDM modulation for ONE symbol.
    //
    // Parameters:
    //   in   — frequency-domain input; in.samples must contain exactly
    //          cfg.n_sc complex subcarrier values (non-owning view).
    //   out  — time-domain output; out.samples will be resized and filled
    //          with (cfg.n_fft + cfg.cp) complex samples.
    //   ctrl — enable / bypass control.
    //   cfg  — per-call configuration (typically matches params_ set at init).
    //   ctx  — execution context (symbol index, slot, frame timing).
    //
    // -------------------------------------------------------------------------
    void eval(const Inputs&     in,
                    Outputs&    out,
              const Control&    ctrl,
              const Config&     cfg,
              const ExecContext& ctx);

private:
    // =========================================================================
    // Private helpers
    // =========================================================================

    // -------------------------------------------------------------------------
    // apply_dft_precoding_()
    //
    // Optional precoding (LTE).
    // Applies an N-point DFT to the input subcarrier data, where N = cfg.n_sc.
    // The output is scaled by 1/sqrt(N) to preserve power.
    //
    // -------------------------------------------------------------------------
    itpp::cvec apply_dft_precoding_(const itpp::cvec& in_sc);

    // -------------------------------------------------------------------------
    // arrange_subcarriers_()
    //
    // Arranges the n_sc active subcarriers into the n_fft-point IFFT input buffer.
    //
    //   IFFT input layout (example n_fft=8, n_sc=6):
    //     bin: 0   1   2   3   4   5   6   7
    //          SC3 SC4 SC5  0   0  SC0 SC1 SC2
    //          ^ positive freq     ^ negative freq
    //
    // Writes directly into ifft_in_ (owning member buffer).
    //
    // -------------------------------------------------------------------------
    itpp::cvec arrange_subcarriers_(const itpp::cvec& sc_data,
                                    uint16_t n_fft, uint16_t n_sc);

    // -------------------------------------------------------------------------
    // add_cyclic_prefix_()
    //
    // Prepends the last `cp` samples of the OFDM symbol to its beginning.
    // Output length = n_fft + cp.
    //
    //   Output: [ time[n_fft-cp .. n_fft-1]  |  time[0 .. n_fft-1] ]
    //            <- cyclic prefix (cp) ->        <- useful symbol ->
    //
    // -------------------------------------------------------------------------
    itpp::cvec add_cyclic_prefix_(const itpp::cvec& time_sym,
                                  uint16_t n_fft, uint16_t cp);

    // -------------------------------------------------------------------------
    // apply_windowing_()
    //
    // Applies a raised-cosine ramp of length n_win to both edges of the
    // CP-extended symbol and performs overlap-add with the tail of the
    // previous symbol stored in prev_tail_.
    //
    // The windowed output length is still (n_fft + cp); the tail of the
    // current symbol is saved into prev_tail_ for the next call.
    //
    // No-op when n_win == 0 or cfg.win == false.
    //
    // -------------------------------------------------------------------------
    itpp::cvec apply_windowing_(const itpp::cvec& sym_with_cp,
                                uint16_t cp, uint16_t n_win, uint16_t symbol_idx);

    // =========================================================================
    // Private state
    // =========================================================================

    // Windowing overlap-add state — persists across eval() calls (IS true state)
    itpp::cvec prev_tail_;          // saved tail of previous windowed symbol
};

} // namespace tx
} // namespace sam