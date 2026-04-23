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
#include <vector>

// =============================================================================
// Helpers
// =============================================================================

static void generate_window(itpp::vec& win_leading,
                             itpp::vec& win_trailing,
                             uint16_t   n_win)
{
    win_leading.set_size(n_win,  false);
    win_trailing.set_size(n_win, false);

    for (int i = 0; i < n_win; ++i)
    {
        win_leading[i]  = 0.5 * (1.0 - std::cos(itpp::pi * i / n_win)); // 0 → 1
        win_trailing[i] = 0.5 * (1.0 + std::cos(itpp::pi * i / n_win)); // 1 → 0
    }
}

// Generate a deterministic non-trivial subcarrier vector for symbol s
static sam::SignalData make_symbol(int sym_idx, uint16_t n_sc)
{
    sam::SignalData sd;
    sd.samples.set_size(n_sc, false);
    for (int k = 0; k < n_sc; ++k)
    {
        // real: symbol index + subcarrier index scaled, imag: complementary
        sd.samples[k] = std::complex<double>(
            sym_idx + 1.0 + k * 1e-4,
           -(sym_idx + 1.0) + k * 1e-4);
    }
    return sd;
}

// =============================================================================
// Main
// =============================================================================

int main()
{
    std::cout << "=== OFDMMod → OFDMDemod Loopback (14 symbols) ===\n\n";

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
    // Window coefficients — owned here, passed by pointer into Config
    // -------------------------------------------------------------------------
    itpp::vec win_leading, win_trailing;
    generate_window(win_leading, win_trailing, N_WIN);

    // -------------------------------------------------------------------------
    // Mod config
    // -------------------------------------------------------------------------
    sam::tx::OFDMMod::Config mod_cfg;
    mod_cfg.n_fft         = N_FFT;
    mod_cfg.n_sc          = N_SC;
    mod_cfg.cp            = CP_LEN;
    mod_cfg.n_win         = N_WIN;
    mod_cfg.dft_precoding = true;
    mod_cfg.win_leading   = &win_leading;
    mod_cfg.win_trailing  = &win_trailing;

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

    mod.configure(mod_cfg);
    demod.configure(dem_cfg);

    // -------------------------------------------------------------------------
    // Generate input — 14 symbols
    // -------------------------------------------------------------------------
    std::vector<sam::SignalData> tx_in(N_SYMBOLS);
    for (int s = 0; s < N_SYMBOLS; ++s)
        tx_in[s] = make_symbol(s, N_SC);

    // -------------------------------------------------------------------------
    // Pre-size buffers (caller contract — sized once, reused each symbol)
    // -------------------------------------------------------------------------
    sam::SignalData mod_out;
    mod_out.samples.set_size(N_FFT + CP_LEN, false);

    sam::SignalData demod_out;
    demod_out.samples.set_size(N_SC, false);

    sam::Control ctrl;   // enable=true, bypass=false

    // -------------------------------------------------------------------------
    // Run
    // -------------------------------------------------------------------------
    const double tol  = 1e-6;
    int          pass = 0;
    int          fail = 0;

    for (int s = 0; s < N_SYMBOLS; ++s)
    {
        sam::ExecContext ctx;
        ctx.symbol_idx   = s;
        ctx.slot_idx     = 0;
        ctx.frame_idx    = 0;
        ctx.sample_count = static_cast<uint64_t>(s) * (N_FFT + CP_LEN);
        ctx.start_of_frame = (s == 0);
        ctx.end_of_frame   = (s == N_SYMBOLS - 1);

        mod_out.samples.zeros();
        demod_out.samples.zeros();

        mod.eval(tx_in[s], mod_out,   ctrl, mod_cfg, ctx);
        demod.eval(mod_out, demod_out, ctrl, dem_cfg, ctx);

        // --- compare ---
        bool sym_pass    = true;
        int  first_fail_sc = -1;
        double worst_err   =  0.0;

        for (int k = 0; k < N_SC; ++k)
        {
            const double err = std::abs(demod_out.samples[k] - tx_in[s].samples[k]);
            if (err > tol)
            {
                if (sym_pass) first_fail_sc = k;
                if (err > worst_err) worst_err = err;
                sym_pass = false;
            }
        }

        if (sym_pass)
        {
            std::cout << "[PASS] symbol " << s << "\n";
            ++pass;
        }
        else
        {
            std::cout << "[FAIL] symbol " << s
                      << "  first_fail_sc=" << first_fail_sc
                      << "  worst_err="     << worst_err     << "\n";
            ++fail;
        }
    }

    // -------------------------------------------------------------------------
    // Summary
    // -------------------------------------------------------------------------
    std::cout << "\n=== " << pass << "/" << N_SYMBOLS << " symbols passed";
    if (fail > 0) std::cout << "  (" << fail << " failed)";
    std::cout << " ===\n";

    return fail > 0 ? 1 : 0;
}