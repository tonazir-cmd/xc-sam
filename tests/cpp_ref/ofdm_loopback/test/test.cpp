// =============================================================================
// test.cpp — OFDMMod -> Channel -> OFDMDemod Parametric Sweep
// Extracted inner loopback into run_ofdm_loopback(); main sweeps test_configs.
// =============================================================================

#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/tx/ofdm_mod.hpp"
#include "sam/rx/ofdm_demod.hpp"

#include <itpp/itbase.h>

#include <iostream>
#include <complex>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <cassert>


// =============================================================================
// OFDMTestConfig
// =============================================================================
struct OFDMTestConfig {
    bool is_lte;
    
    double sample_rate;
    uint16_t n_fft;
    uint16_t n_sc;
    uint16_t cp;
    uint16_t n_win;

    double gain;
    double fo;
    size_t to;

    // Overloading the insertion operator
    friend std::ostream& operator<<(std::ostream& os, const OFDMTestConfig& cfg) {
        os << "--- OFDM Test Configuration ---\n"
           << std::left << std::setw(15) << "Mode:"        << (cfg.is_lte ? "LTE" : "5G") << "\n"
           << std::setw(15) << "Sample Rate:" << cfg.sample_rate / 1e6 << " MHz\n"
           << std::setw(15) << "FFT Size:"    << cfg.n_fft << "\n"
           << std::setw(15) << "Subcarriers:" << cfg.n_sc << "\n"
           << std::setw(15) << "CP Length:"   << cfg.cp << "\n"
           << std::setw(15) << "Symbols:"     << N_SYM << "\n"
           << std::setw(15) << "Windowing:"   << cfg.n_win << "\n"
           << std::setw(15) << "Gain:"        << cfg.gain << "\n"
           << std::setw(15) << "Freq Offset:" << cfg.fo << " Hz\n"
           << std::setw(15) << "Time Offset:" << cfg.to << " samples\n"
           << "-------------------------------";
        return os;
    }
};

std::vector<OFDMTestConfig> generate_configs() {
    
    const double scs = 15e3; // subcarrier spacing

    struct NFFTGroupConfig {
        uint16_t n_fft;
        uint16_t n_sc;
        uint16_t cp;
    };

    const std::array<NFFTGroupConfig, 6> configs_nfft_group = {{
        {128,  72,   9},
        {256,  132,  18},
        {512,  252,  36},
        {1024, 516,  72},
        {2048, 1020, 144},
        {4096, 3300, 288}
    }};

    const std::array<uint16_t, 2> configs_n_win = {0, 64};
    const std::array<double, 3> configs_gain = {1.0, 2.0, 5.0};
    const std::array<double, 3> configs_fo = {0.0, 100.0, -100.0};
    const std::array<size_t, 3> configs_to = {0, 5, 10};
    const std::array<bool, 2> configs_lte = {true, false};

    std::vector<OFDMTestConfig> all_configs;
    all_configs.reserve(
        configs_nfft_group.size() * configs_n_win.size() * configs_gain.size() * configs_fo.size() * configs_to.size() * configs_lte.size()
    ); // Pre-allocate memory

    for (const auto& nfft_group : configs_nfft_group) {
        for (auto n_win : configs_n_win) {
            for (auto gain : configs_gain) {
                for (auto fo : configs_fo) {
                    for (auto to : configs_to) {
                        for (auto lte : configs_lte) {
                            if (n_win > nfft_group.cp) continue; // Skip invalid config
                            double sample_rate = nfft_group.n_fft * scs;
                            all_configs.push_back(OFDMTestConfig{
                                lte,
                                sample_rate,
                                nfft_group.n_fft,
                                nfft_group.n_sc,
                                nfft_group.cp,
                                n_win,
                                gain,
                                fo,
                                to
                            });
                        }
                    }
                }
            }
        }
    }
    return all_configs;
}

// =============================================================================
// ChannelSimulator
// =============================================================================
static void apply_channel(const itpp::cvec& in, itpp::cvec& out,
                           double rx_gain, double rx_fo, double rx_to, double fs)
{
    double ch_gain = 1.0 / rx_gain;
    double ch_fo = -rx_fo;

    assert(rx_to >= 0 && rx_to < in.length());

    // Time offset
    itpp::cvec sig = itpp::concat(itpp::zeros_c(static_cast<int>(rx_to)), in);

    // Gain offset
    sig *= ch_gain;

    // Frequency offset
    int n = sig.length();
    itpp::vec idx   = itpp::linspace(0, n - 1, n);
    itpp::vec phase = (2.0 * M_PI * ch_fo / fs) * idx;
    itpp::cvec rot  = itpp::to_cvec(itpp::cos(phase), itpp::sin(phase));
    out = elem_mult(sig, rot);
}

void populate_ofdm_cfgs(const OFDMTestConfig& cfg, sam::tx::OFDMMod::Config& mod_cfg, sam::rx::OFDMDemod::Config& dem_cfg) {
    mod_cfg.n_fft         = cfg.n_fft;
    mod_cfg.n_sc          = cfg.n_sc;
    mod_cfg.n_win         = cfg.n_win;
    mod_cfg.cp            = cfg.cp;
    mod_cfg.dft_precoding = cfg.is_lte;
    mod_cfg.dc            = cfg.is_lte;

    dem_cfg.n_fft         = cfg.n_fft;
    dem_cfg.n_sc          = cfg.n_sc;
    dem_cfg.cp            = cfg.cp;
    dem_cfg.dc            = cfg.is_lte;
    dem_cfg.dft_precoding = cfg.is_lte;
    dem_cfg.fo            = cfg.fo / cfg.sample_rate; // normalized
    dem_cfg.gain          = cfg.gain;
    dem_cfg.phase         = 0.0;
    dem_cfg.curr_sym_windowing = true;
}

// =============================================================================
// run_ofdm_loopback
//
// Parameters
//   cfg      — all OFDM + channel parameters
//   tx_input — input subcarrier bins, length must be N_SYM * cfg.n_sc
//
// Returns received signal
// =============================================================================
itpp::cvec run_ofdm_loopback(const OFDMTestConfig& cfg, const itpp::cvec& tx_input)
{
    const int sym_len   = cfg.n_fft + cfg.cp;

    assert(static_cast<int>(tx_input.length()) == N_SYM * cfg.n_sc);

    // --- Build mod / demod configs -------------------------------------------
    sam::tx::OFDMMod::Config mod_cfg;
    sam::rx::OFDMDemod::Config dem_cfg;
    populate_ofdm_cfgs(cfg, mod_cfg, dem_cfg);

    sam::tx::OFDMMod   mod;
    sam::rx::OFDMDemod demod;
    sam::Control       ctrl;

    itpp::cvec tx_waveform(N_SYM * sym_len);
    itpp::cvec rx_waveform;
    itpp::cvec rx_output(N_SYM * cfg.n_sc);
    tx_waveform.zeros();
    rx_output.zeros();

    sam::SignalData tx_sym, mod_out, demod_in, demod_out;
    mod_out.samples.set_size(sym_len,    false);
    demod_in.samples.set_size(sym_len,   false);
    demod_out.samples.set_size(cfg.n_sc, false);

    // =========================================================================
    // LOOP 1: Modulate
    // =========================================================================
    for (int s = 0; s < N_SYM; ++s)
    {
        sam::ExecContext ctx;
        ctx.symbol_idx     = s;
        ctx.slot_idx       = 0;
        ctx.frame_idx      = 0;
        ctx.sample_count   = static_cast<uint64_t>(s) * sym_len;
        ctx.start_of_frame = (s == 0);
        ctx.end_of_frame   = (s == N_SYM - 1);

        tx_sym.samples = tx_input.mid(s * cfg.n_sc, cfg.n_sc);
        mod_out.samples.zeros();

        mod.eval(tx_sym, mod_out, ctrl, mod_cfg, ctx);
        tx_waveform.set_subvector(s * sym_len, mod_out.samples);
    }

    // =========================================================================
    // LOOP 2: Apply channel
    // =========================================================================
    apply_channel(tx_waveform, rx_waveform,
                  cfg.gain, cfg.fo, cfg.to, cfg.sample_rate);

    // =========================================================================
    // LOOP 3: Demodulate (with integer time-offset compensation)
    // =========================================================================
    for (int s = 0; s < N_SYM; ++s)
    {
        sam::ExecContext ctx;
        ctx.symbol_idx     = s;
        ctx.slot_idx       = 0;
        ctx.frame_idx      = 0;
        ctx.sample_count   = (static_cast<uint64_t>(s) * sym_len) + cfg.to;
        ctx.start_of_frame = (s == 0);
        ctx.end_of_frame   = (s == N_SYM - 1);

        demod_in.samples.zeros();
        demod_out.samples.zeros();

        demod_in.samples = rx_waveform.mid(ctx.sample_count, sym_len);

        demod.eval(demod_in, demod_out, ctrl, dem_cfg, ctx);
        rx_output.set_subvector(s * cfg.n_sc, demod_out.samples);
    }

    return rx_output;
}

// =============================================================================
// Main — parameter sweep
// =============================================================================
int main()
{
    std::cout << "=== OFDM Parametric Loopback Sweep ===\n\n";

    size_t total = 0;
    size_t failed = 0;

    const size_t pass_threshold_db = 100;
    std::vector<OFDMTestConfig> test_configs = generate_configs();

    for (const auto& cfg : test_configs)
    {
        itpp::cvec tx_input = itpp::randn_c(N_SYM * cfg.n_sc);
        itpp::cvec rx_output = run_ofdm_loopback(cfg, tx_input);

        itpp::cvec err = tx_input - rx_output;
        double sig_pwr = itpp::sum_sqr(itpp::abs(tx_input));
        double err_pwr = itpp::sum_sqr(itpp::abs(err));
        double snr_db = 10.0 * std::log10(sig_pwr / err_pwr);

        bool passed = (snr_db >= pass_threshold_db);
        total++;
        if (!passed) {
            std::cerr << "Test failed for config:\n" << cfg << "\n";
            std::cerr << "SNR: " << snr_db << " dB (below threshold of " << pass_threshold_db << " dB)\n";
            failed++;
        }
    }

    std::cout << "Result:\n"
     << "Total: " << total << " | Passed: " << (total - failed) << " | Failed: " << failed << "\n"
     << "Overall: " << (failed == 0 ? "PASS" : "FAIL") << "\n";

    return failed == 0 ? 0 : 1;
}