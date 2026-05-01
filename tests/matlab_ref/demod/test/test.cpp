#include "sam/core/constants.h"
#include "sam/core/signal_types.h"
#include "sam/rx/ofdm_demod.h"
#include "sam/utils/vec_io.h"
#include "sam/utils/argsparser.h"
#include "sam/utils/lte_pdsch_params.h"

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

int main(int argc, char* argv[]) {
    TestArgs args = ArgsParser::parse(argc, argv);

    std::cout << "\n--- DEMOD Test ---\n";
    std::cout << "Mode:      " << args.mode_str << "\n";
    std::cout << "Channel:   " << args.channel_str << "\n";
    std::cout << "Bandwidth: " << args.bandwidth_str << " MHz\n";
    std::cout << "FilePath:  " << args.toVectorsDir("../test_vectors") << "\n\n";
    const std::string test_vector_dir = args.toVectorsDir("../test_vectors");
    const std::string input_params_txt = test_vector_dir + "lte_input_params.txt";
    const std::string rx_demod_input_bin = test_vector_dir + "lte_time1_input_rx_signal0.bin";
    const std::string rx_demod_reference_bin = test_vector_dir + "lte_time1_output_rxSubframe.bin";

    LTEPdschParams params = LTEPdschParamsLoader::load(input_params_txt);

    const std::vector<int> cp_array = params.Ncp;
    const size_t t_off = params.TimingOffset;
    const float rx_gain = 4.0;

    sam::rx::OFDMDemod::Config dem_cfg;
    dem_cfg.n_fft         = params.nFFT;
    dem_cfg.n_sc          = params.Nsc;
    dem_cfg.cp            = cp_array[0];
    dem_cfg.dc            = (args.mode == Mode::LTE);
    dem_cfg.dft_precoding = false;
    dem_cfg.fo            = (params.FreqOffset == 0.007812f)? 0.0078125 : params.FreqOffset;
    dem_cfg.gain          = rx_gain;

    itpp::cvec input = read_sc16_as_cvec(rx_demod_input_bin, 15);

    sam::rx::OFDMDemod demod;
    sam::Control ctrl;
    sam::ExecContext ctx{0};
    sam::SignalData demod_in, demod_out;

    itpp::cvec output(dem_cfg.n_sc * N_SYM);
    ctx.sample_count = t_off;
    demod_out.samples.set_size(dem_cfg.n_sc);

    for (size_t sym_idx = 0; sym_idx < N_SYM; sym_idx++) {
        dem_cfg.cp = cp_array[sym_idx];
        ctx.symbol_idx = sym_idx;
        int sym_len = dem_cfg.n_fft + dem_cfg.cp;

        demod_in.samples = input.mid(ctx.sample_count, sym_len);
        demod.eval(demod_in, demod_out, ctrl, dem_cfg, ctx);
        output.set_subvector(sym_idx * dem_cfg.n_sc, demod_out.samples);

        ctx.sample_count += sym_len;
    }

    itpp::cvec reference = read_sc16_as_cvec(rx_demod_reference_bin, 7);

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
