#include "sam/core/constants.h"
#include "sam/core/signal_types.h"
#include "sam/rx/ofdm_demod.h"
#include "sam/utils/vec_io.h"

#include <itpp/itbase.h>
#include <itpp/fixed/cfix.h>
#include <itpp/base/converters.h>

#include <iostream>
#include <complex>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <limits>
#include <cassert>
#include <cassert>
#include <stdexcept>
#include <string>
#include <fstream>
#include <vector>
#include <complex>

int main() {
    const std::array<uint16_t, N_SYM> cp_array = {160, 144, 144, 144, 144, 144, 144, 160, 144, 144, 144, 144, 144, 144};
    const std::string rx_demod_input = "test_vectors/lte_time1_input_rx_signal0.bin";
    const std::string rx_demod_reference = "test_vectors/lte_time1_output_rxSubframe0.bin";

    size_t t_off = 8;
    sam::rx::OFDMDemod::Config dem_cfg;
    dem_cfg.n_fft         = 2048;
    dem_cfg.n_sc          = 1200;
    dem_cfg.cp            = cp_array[0];
    dem_cfg.dc            = true;
    dem_cfg.dft_precoding = false;
    dem_cfg.fo            = 1.0 / 128.0;
    dem_cfg.gain          = 4.0;

    itpp::cvec input = read_sc16_as_cvec(rx_demod_input, 15);
    
    sam::rx::OFDMDemod demod;
    sam::Control ctrl;
    sam::ExecContext ctx{0};
    sam::SignalData demod_in, demod_out;
    
    itpp::cvec output(dem_cfg.n_sc * N_SYM);
    ctx.sample_count = t_off;
    demod_out.samples.set_size(dem_cfg.n_sc);

    for (size_t sym_idx = 0; sym_idx < N_SYM; sym_idx++) {
        dem_cfg.cp = cp_array[sym_idx];
        ctx.symbol_idx     = sym_idx;
        int sym_len = dem_cfg.n_fft + dem_cfg.cp;

        demod_in.samples = input.mid(ctx.sample_count, sym_len);
        demod.eval(demod_in, demod_out, ctrl, dem_cfg, ctx);
        output.set_subvector(sym_idx * dem_cfg.n_sc, demod_out.samples);

        ctx.sample_count += sym_len;
    }

    itpp::cvec reference = read_sc16_as_cvec(rx_demod_reference, 7);

    double mse = itpp::mean(itpp::abs(output - reference));
    double snr = 20 * std::log10(itpp::mean(itpp::abs(reference)) / mse);

    std::cout << "MSE: " << mse << "\n";
    std::cout << "SNR: " << snr << "\n";
    
    if (snr < 50) {
        std::cerr << "Test failed: SNR is below 50 dB.\n";
        return 1;
    }

    return 0;
}
