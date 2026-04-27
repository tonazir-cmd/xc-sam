// =============================================================================
// OFDM Demodulator File-Based Test
// Merges config/file reading with the OFDM Demodulator execution and validation.
// =============================================================================

#include <cstdio>
#include <iostream>
#include <complex>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <cassert>

#include "config.h"
#include "sam/core/signal_types.h"
#include "sam/rx/ofdm_demod.h"

#include <itpp/itbase.h>

int main() {
    std::string config_dir = "test_vectors/configs/";
    std::string input_dir = "test_vectors/inputs/";
    std::string ref_dir = "test_vectors/refs/";

    std::vector<std::string> config_files;
    if (!get_all_configs(config_dir, config_files)) {
        printf("Failed to get all config files.\n");
        return 1;
    }

    size_t read = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;

    std::cout << "=== OFDM Demodulator File-Based Validation ===\n\n";

    for (const auto& cfg_name : config_files) {
        printf("--------------------------------------------------\n");
        printf("Running Test: %s\n", cfg_name.c_str());
        
        std::string config_file = config_dir + cfg_name + ".txt";
        std::string input_file = input_dir + cfg_name + ".bin";
        std::string ref_file = ref_dir + cfg_name + ".bin";

        Config my_cfg;
        itpp::cvec input_samples; // Time-domain rx waveform
        itpp::cvec ref_samples;   // Expected freq-domain subcarriers
        
        // 1. Load Configurations and Data
        if (!read_config(config_file, my_cfg)) {
            printf("Failed to load configuration from %s\n", config_file.c_str());
            return 1;
        }

        if (!read_complex_binary(input_file, input_samples)) {
            printf("Failed to load input samples from %s\n", input_file.c_str());
            return 1;
        }
        
        if (!read_complex_binary(ref_file, ref_samples)) {
            printf("Failed to load reference samples from %s\n", ref_file.c_str());
            return 1;
        }

        // 2. Derive Parameters
        const int sym_len = my_cfg.n_fft + my_cfg.cp;
        
        // Calculate number of symbols based on input file size and time offset
        int n_sym = (input_samples.length()) / sym_len;
        
        if (n_sym <= 0 || (input_samples.length()) % sym_len != 0) {
            printf("Error: Input length doesn't cleanly match an integer number of symbols based on N_FFT, and CP.\n");
            return 1;
        }

        // Calculate sample rate: SCS (kHz) * 1000 * N_FFT
        double sample_rate = static_cast<double>(my_cfg.scs) * 1e3 * my_cfg.n_fft;

        // 3. Configure the Demodulator
        sam::rx::OFDMDemod::Config dem_cfg;
        dem_cfg.n_fft         = my_cfg.n_fft;
        dem_cfg.n_sc          = my_cfg.n_sc;
        dem_cfg.cp            = my_cfg.cp;
        dem_cfg.dc            = my_cfg.lte;
        dem_cfg.dft_precoding = my_cfg.lte;
        dem_cfg.sample_rate   = sample_rate;
        dem_cfg.fo            = my_cfg.fo;
        dem_cfg.to            = my_cfg.to;
        dem_cfg.gain          = my_cfg.gain;

        sam::rx::OFDMDemod demod;
        sam::Control       ctrl;

        itpp::cvec rx_output(n_sym * my_cfg.n_sc);
        rx_output.zeros();

        sam::SignalData demod_in, demod_out;
        demod_in.samples.set_size(sym_len, false);
        demod_out.samples.set_size(my_cfg.n_sc, false);

        // 4. Run the Demodulator Symbol-by-Symbol
        for (int s = 0; s < n_sym; ++s) {
            sam::ExecContext ctx;
            ctx.symbol_idx     = s;
            ctx.slot_idx       = 0;
            ctx.frame_idx      = 0;
            ctx.sample_count   = static_cast<uint64_t>(s) * sym_len;
            ctx.start_of_frame = (s == 0);
            ctx.end_of_frame   = (s == n_sym - 1);

            demod_in.samples.zeros();
            demod_out.samples.zeros();

            int start_idx = s * sym_len;
            demod_in.samples = input_samples.mid(start_idx, sym_len);

            demod.eval(demod_in, demod_out, ctrl, dem_cfg, ctx);
            rx_output.set_subvector(s * my_cfg.n_sc, demod_out.samples);
        }

        // 5. Compare with Reference Vectors
        // Check if output length matches reference length
        if (rx_output.length() != ref_samples.length()) {
            printf("FAIL: Output length (%d) does not match reference length (%d)\n", 
                   rx_output.length(), ref_samples.length());
            failed_tests++;
            continue;
        }

        itpp::cvec err = ref_samples - rx_output;
        double max_error = itpp::max(itpp::abs(err)); // Using absolute error magnitude

        if (max_error <= my_cfg.threshold) {
            printf("PASS: Max Error = %.2e (Threshold = %.2e)\n", max_error, my_cfg.threshold);
            passed_tests++;
        } else {
            printf("FAIL: Max Error = %.2e (Exceeds Threshold of %.2e)\n", max_error, my_cfg.threshold);
            failed_tests++;
        }

        read++;
    }

    printf("\n=== Summary ===\n");
    printf("Total Processed: %zu\n", read);
    printf("Passed: %zu\n", passed_tests);
    printf("Failed: %zu\n", failed_tests);

    return failed_tests == 0 ? 0 : 1;
}