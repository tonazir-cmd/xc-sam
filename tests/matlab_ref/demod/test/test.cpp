#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/rx/ofdm_demod.hpp"
#include "sam/utils/vec_io.hpp"
#include "sam/utils/argsparser.hpp"
#include "sam/utils/lte_pdsch_params.hpp"

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

struct TestParams
{
    std::vector<int> cp_array;
    size_t t_off;
    std::string input_bin;
    std::string reference_bin;
    sam::rx::OfdmDemod::Config demod_config;
};

TestParams get_test_params(TestArgs args);

int main(int argc, char* argv[]) {
    TestArgs args = ArgsParser::parse(argc, argv);

    std::cout << "\n--- DEMOD Test ---\n";
    std::cout << args;

    TestParams params = get_test_params(args);
    sam::rx::OfdmDemod::Config& demod_config = params.demod_config;

    itpp::cvec input = read_sc16_as_cvec(params.input_bin, 15);

    sam::rx::OfdmDemod demod;
    sam::Control ctrl;
    sam::ExecContext ctx{0};
    sam::SignalData demod_in, demod_out;

    itpp::cvec output(demod_config.n_sc * N_SYM);
    ctx.sample_count = params.t_off;
    demod_out.samples.set_size(demod_config.n_sc);

    for (size_t sym_idx = 0; sym_idx < N_SYM; sym_idx++) {
        demod_config.cp = params.cp_array[sym_idx];
        ctx.symbol_idx = sym_idx;
        int sym_len = demod_config.n_fft + demod_config.cp;

        demod_in.samples = input.mid(ctx.sample_count, sym_len);
        demod.eval(demod_in, demod_out, ctrl, demod_config, ctx);
        output.set_subvector(sym_idx * demod_config.n_sc, demod_out.samples);

        ctx.sample_count += sym_len;
    }

    itpp::cvec reference = read_sc16_as_cvec(params.reference_bin, 8);

    double mse = itpp::mean(itpp::abs(output - reference));
    double snr = 20 * std::log10(itpp::mean(itpp::abs(reference)) / mse);

    std::cout << "\n--- Result ---\n";
    std::cout << "MSE: " << mse << "\n";
    std::cout << "SNR: " << snr << "\n";
    
    if (snr < 50) {
        std::cerr << "Test failed: SNR is below 50 dB.\n";
        return 1;
    }

    return 0;
}

TestParams get_test_params(TestArgs args)
{
    TestParams params;
    if (args.mode == Mode::LTE)
    {
        const std::string test_vector_dir = args.toVectorsDir("../test_vectors");
        std::cout << "Test Vector Dir: " << test_vector_dir << "\n";
        
        const std::string input_params_txt = test_vector_dir + "lte_input_params.txt";
        LTEPdschParams lte_params = LTEPdschParamsLoader::load(input_params_txt);
            
        params.input_bin = test_vector_dir + "lte_time1_input_rx_signal0.bin";
        params.reference_bin = test_vector_dir + "lte_time1_output_rxSubframe.bin";
        params.t_off = lte_params.TimingOffset;
        params.cp_array = lte_params.Ncp;

        params.demod_config.n_fft = lte_params.nFFT;
        params.demod_config.n_sc = lte_params.Nsc;
        params.demod_config.cp = params.cp_array[0];
        params.demod_config.fo = (lte_params.FreqOffset == 0.007812f)? 0.0078125 : lte_params.FreqOffset;
        params.demod_config.gain = 4.0;
        params.demod_config.phase = 0;
        params.demod_config.dc = true;
        params.demod_config.dft_precoding = false;
    }
    else if (args.mode == Mode::NR5G)
    {
        const std::string test_vector_dir = args.toVectorsDir("../test_vectors");
        std::cout << "Test Vector Dir: " << test_vector_dir << "\n";

        params.input_bin = test_vector_dir + "5g_input_rx_signal0.bin";
        params.reference_bin = test_vector_dir + "5g_output_rxSubframe.bin";
        params.t_off = 0;
        params.cp_array = {544, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288};
        
        params.demod_config.n_fft = 4096;
        params.demod_config.n_sc = 3300;
        params.demod_config.cp = params.cp_array[0];
        params.demod_config.fo = 0.0;
        params.demod_config.gain = 1.0;
        params.demod_config.phase = 0;
        params.demod_config.dc = false;
        params.demod_config.dft_precoding = false;
    }
    

    return params;
}