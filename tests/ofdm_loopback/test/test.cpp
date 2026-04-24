// =============================================================================
// test.cpp — OFDMMod → OFDMDemod loopback test
// 14 symbols, n_fft=4096, DFT precoding, windowing
// =============================================================================

#include "sam/core/signal_types.h"
#include "sam/tx/ofdm_mod.h"
#include "sam/rx/ofdm_demod.h"

#include <itpp/itbase.h>

#include <iostream>
#include <complex>
#include <cmath>
#include <algorithm> // for std::max

// =============================================================================
// Main
// =============================================================================

int main()
{
    std::cout << "=== OFDMMod -> OFDMDemod Loopback (14 symbols) ===\n\n";

    // -------------------------------------------------------------------------
    // Parameters — 5G NR-like μ=1 (30kHz SCS), 100MHz BW
    // -------------------------------------------------------------------------
    const uint16_t N_FFT     = 4096;
    const uint16_t N_SC      = 3276;    // 273 RBs × 12 subcarriers
    const uint16_t CP_LEN    = 288;     // normal CP for μ=1
    const uint16_t N_WIN     = 64;      // windowing ramp length (must be ≤ CP)
    const int      N_SYMBOLS = 14;
    const double   FS        = 122.88e6; // N_FFT × 30kHz

    // -------------------------------------------------------------------------
    // Mod config
    // -------------------------------------------------------------------------
    sam::tx::OFDMMod::Config mod_cfg;
    mod_cfg.n_fft         = N_FFT;
    mod_cfg.n_sc          = N_SC;
    mod_cfg.cp            = CP_LEN;
    mod_cfg.n_win         = N_WIN;
    mod_cfg.dft_precoding = true;

    // -------------------------------------------------------------------------
    // Demod config
    // -------------------------------------------------------------------------
    sam::rx::OFDMDemod::Config dem_cfg;
    dem_cfg.n_fft         = N_FFT;
    dem_cfg.n_sc          = N_SC;
    dem_cfg.cp            = CP_LEN;
    dem_cfg.dc            = 0;      // no DC subcarrier
    dem_cfg.dft_precoding = true;
    dem_cfg.sample_rate   = FS;
    dem_cfg.freq_offset   = 0.0;
    dem_cfg.phase         = 0.0;
    dem_cfg.gain          = 1.0;

    // -------------------------------------------------------------------------
    // Blocks
    // -------------------------------------------------------------------------
    sam::tx::OFDMMod   mod;
    sam::rx::OFDMDemod demod;

    // -------------------------------------------------------------------------
    // Generate Input & Pre-allocate Output
    // -------------------------------------------------------------------------
    
    // Store all TX symbols in a single flat IT++ vector
    itpp::cvec tx_all(N_SYMBOLS * N_SC);
    for (int s = 0; s < N_SYMBOLS; ++s)
    {
        for (int k = 0; k < N_SC; ++k)
        {
            tx_all[s * N_SC + k] = std::complex<double>(
                s + 1.0 + k * 1e-4,
               -(s + 1.0) + k * 1e-4);
        }
    }

    // Pre-allocate the full RX vector to hold the output
    itpp::cvec rx_all(N_SYMBOLS * N_SC);
    rx_all.zeros();

    // Loop buffers
    sam::SignalData tx_sym;
    sam::SignalData mod_out;
    sam::SignalData demod_out;
    
    mod_out.samples.set_size(N_FFT + CP_LEN, false);
    demod_out.samples.set_size(N_SC, false);

    sam::Control ctrl;   // enable=true, bypass=false

    // -------------------------------------------------------------------------
    // Run Loopback
    // -------------------------------------------------------------------------
    for (int s = 0; s < N_SYMBOLS; ++s)
    {
        sam::ExecContext ctx;
        ctx.symbol_idx   = s;
        ctx.slot_idx     = 0;
        ctx.frame_idx    = 0;
        ctx.sample_count = static_cast<uint64_t>(s) * (N_FFT + CP_LEN);
        ctx.start_of_frame = (s == 0);
        ctx.end_of_frame   = (s == N_SYMBOLS - 1);

        // Extract the current symbol from the flat TX vector
        tx_sym.samples = tx_all.mid(s * N_SC, N_SC);

        mod_out.samples.zeros();
        demod_out.samples.zeros();

        mod.eval(tx_sym, mod_out,   ctrl, mod_cfg, ctx);
        demod.eval(mod_out, demod_out, ctrl, dem_cfg, ctx);

        // Store the demodulated symbol into the flat RX vector
        rx_all.set_subvector(s * N_SC, demod_out.samples);
    }

    // -------------------------------------------------------------------------
    // Error Calculation (SNR)
    // -------------------------------------------------------------------------
    itpp::cvec error_vec = tx_all - rx_all;

    double sig_power = itpp::sum_sqr(itpp::abs(tx_all));
    double noise_power = itpp::sum_sqr(itpp::abs(error_vec));
    double snr_db = 10.0 * std::log10(sig_power / noise_power);

    std::cout << "--- Loopback Performance ---\n";
    std::cout << "Calculated SNR : " << snr_db << " dB\n";

    const double THRESHOLD_DB = 100.0;
    if (snr_db >= THRESHOLD_DB)
    {
        std::cout << "[PASS] SNR exceeds the " << THRESHOLD_DB << " dB threshold.\n\n";
        return 0;
    }
    else
    {
        std::cout << "[FAIL] SNR is below the " << THRESHOLD_DB << " dB threshold.\n\n";
        return 1;
    }
}