#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/rx/ofdm_irx.hpp"
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
#include <numeric>
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

#define N_RX 2
#define N_LAYERS 2

using OfdmIRX = sam::rx::OfdmIRX<N_RX, N_LAYERS>;
using DMRSPattern = sam::rx::ChannelEstimator::DMRSPattern;

struct TestParams
{
    std::string input_rx0_bin;
    std::string input_rx1_bin;
    std::string dmrs_bin;
    std::string reference_bin;
    
    itpp::ivec cp_array;
    size_t n_fft;
    size_t n_sc;
    size_t t_off;
    size_t pilot_sym_id; // TEMPORARILY FOR THIS TEST
    double fo;
    double gain;
    double phase;
    bool dc;
    bool dft_precoding;
    DMRSPattern dmrs_pattern[N_LAYERS];
    double n_var;
    double qm_mode;
};

TestParams get_test_params(TestArgs args);
OfdmIRX::Config populate_config(const TestParams& params);

int main(int argc, char* argv[]) {
    //////////////////////////////////////
    // Input Argument processing
    //////////////////////////////////////
    
    TestArgs args = {Mode::NR5G, Channel::PDSCH, Bandwidth::BW_20, DMRSMode::Mode2};
    ArgsParser::parse(args, argc, argv);

    std::cout << "\n--- DEMOD Test ---\n";
    std::cout << args;

    TestParams params = get_test_params(args);
    OfdmIRX::Config config = populate_config(params);

    //////////////////////////////////////
    // Loading RX Input
    //////////////////////////////////////

    itpp::cvec input_rx0 = read_sc16_as_cvec(params.input_rx0_bin, 15, N_SYM * params.n_fft + itpp::sum(params.cp_array));
    itpp::cvec input_rx1 = read_sc16_as_cvec(params.input_rx1_bin, 15, N_SYM * params.n_fft + itpp::sum(params.cp_array));
    sam::SignalData rx_grid_in[N_RX][N_SYM];

    size_t total_symbols = 0;
    for (size_t sym = 0; sym < N_SYM; sym++)
    {
        rx_grid_in[0][sym].samples = input_rx0.mid(total_symbols, params.n_fft + params.cp_array[sym]);
        rx_grid_in[1][sym].samples = input_rx1.mid(total_symbols, params.n_fft + params.cp_array[sym]);
        total_symbols += params.n_fft + params.cp_array[sym];
    }

    //////////////////////////////////////
    // Loading DMRS
    //////////////////////////////////////
    
    itpp::cvec input_dmrs = read_sc16_as_cvec(params.dmrs_bin, 15, params.n_sc);
    sam::SignalData dmrs[N_LAYERS];
    dmrs[0].samples = input_dmrs.mid(0, params.n_sc / 2);
    dmrs[1].samples = input_dmrs.mid(params.n_sc / 2, params.n_sc / 2);

    //////////////////////////////////////
    // Initiating output
    //////////////////////////////////////

    sam::RealData llrs_out[N_LAYERS][N_SYM - 1];
    for (size_t tx = 0; tx < N_LAYERS; tx++)
        for (size_t sym = 0; sym < (N_SYM - 1); sym++)
            llrs_out[tx][sym].samples.set_size(params.n_sc * params.qm_mode);

    
    //////////////////////////////////////
    // Processing
    //////////////////////////////////////

    OfdmIRX ofdm_irx;
    OfdmIRX::Control ctrl;
    OfdmIRX::Inputs in;
    OfdmIRX::Outputs out;

    sam::ExecContext ctx{0};
    ctx.sample_count = params.t_off;

    for (size_t tx = 0; tx < N_LAYERS; tx++)
        in.dmrs[tx] = &dmrs[tx];

    size_t out_sym_idx = 0;
    for (size_t sym_idx = 0; sym_idx < N_SYM; sym_idx++) {
        size_t sym_len = params.n_fft + params.cp_array[sym_idx];
        ctx.symbol_idx = sym_idx;
        config.demod.cp = params.cp_array[sym_idx];

        for (size_t rx = 0; rx < N_RX; rx++) in.rx[rx] = &rx_grid_in[rx][sym_idx];
        for (size_t tx = 0; tx < N_LAYERS; tx++) out.llrs[tx] = &llrs_out[tx][out_sym_idx];
        ofdm_irx.eval(in, out, ctrl, config, ctx);

        if (out.valid) out_sym_idx++;
        ctx.sample_count += sym_len;
    }

    for (size_t sym_idx = N_SYM; sym_idx < N_SYM + config.process_delay; sym_idx++) {
        ctx.symbol_idx = sym_idx;
        
        for (size_t tx = 0; tx < N_LAYERS; tx++)
            out.llrs[tx] = &llrs_out[tx][out_sym_idx];

        ofdm_irx.eval(in, out, ctrl, config, ctx);

        if (out.valid) out_sym_idx++;
    }

    itpp::ivec reference = read_int8_as_ivec(params.reference_bin, params.n_sc * params.qm_mode * (N_SYM - 1) * N_LAYERS);
    
    const auto n_sc = params.n_sc;
    const auto n_bits = params.qm_mode;

    itpp::ivec output(params.n_sc * params.qm_mode * (N_SYM - 1) * N_LAYERS);
    for (size_t sym = 0; sym < (N_SYM - 1); sym++)
        for (size_t sc = 0; sc < n_sc; sc++)
            for (size_t tx = 0; tx < N_LAYERS; tx++)
                output.set_subvector(
                    (sym * N_LAYERS * n_bits * n_sc) + (sc * N_LAYERS * n_bits) + (tx * n_bits),
                    itpp::to_ivec(llrs_out[tx][sym].samples.mid(sc * n_bits, n_bits))
                );

    double mse = itpp::mean(itpp::abs(output - reference));
    double snr = 20 * std::log10(itpp::mean(itpp::abs(reference)) / mse);

    std::cout << "\n--- Result ---\n";
    std::cout << "MSE: " << mse << "\n";
    std::cout << "SNR: " << snr << "\n";
    
    if (snr < 30) {
        std::cerr << "Test failed: SNR is below 30 dB.\n";
        return 1;
    }

    return 0;
}

TestParams get_test_params(TestArgs args)
{
    TestParams params;
    if (args.mode == Mode::LTE && args.dmrs_mode != DMRSMode::Mode2)
    {
        std::runtime_error("Only 5g dmrs mode 2 is supported at the moment");
        return params;
    }

    const std::string test_vector_dir = args.toVectorsDir("../test_vectors");
    std::cout << "Test Vector Dir: " << test_vector_dir << "\n";

    params.input_rx0_bin = test_vector_dir + "5g_input_rx_signal0.bin";
    params.input_rx1_bin = test_vector_dir + "5g_input_rx_signal1.bin";
    params.dmrs_bin = test_vector_dir + "5g_input_refsym_dmrs_all.bin";
    params.reference_bin = test_vector_dir + "5g_output_demodulated.bin";
    params.cp_array = "544 288 288 288 288 288 288 288 288 288 288 288 288 288";
    
    params.n_fft = 4096;
    params.n_sc = 3300;
    params.t_off = 0;
    params.fo = 0.0;
    params.gain = 1.0;
    params.phase = 0;
    params.dc = false;
    params.dft_precoding = false;
    params.dmrs_pattern[0] = DMRSPattern::Even;
    params.dmrs_pattern[1] = DMRSPattern::Odd;
    params.n_var = 0.00023516;
    params.qm_mode = 4;
    params.pilot_sym_id = 2;

    return params;
}

OfdmIRX::Config populate_config(const TestParams& params)
{
    OfdmIRX::Config config;
    config.process_delay = params.pilot_sym_id + 1;
    config.is_pilot_sym[params.pilot_sym_id] = true;

    config.demod.n_fft = params.n_fft;
    config.demod.n_sc = params.n_sc;
    config.demod.cp = params.cp_array[0];
    config.demod.fo = params.fo;
    config.demod.gain = params.gain;
    config.demod.phase = params.phase;
    config.demod.dc = params.dc;
    config.demod.dft_precoding = params.dft_precoding;

    for (auto tx = 0; tx < N_LAYERS; tx++)
    {
        config.chan_est[tx].n_sc = params.n_sc;
        config.chan_est[tx].dmrs_pattern = params.dmrs_pattern[tx];
    }

    config.chan_eq_demap.n_sc = params.n_sc;
    config.chan_eq_demap.n_var = params.n_var;
    config.chan_eq_demap.qm_mode = params.qm_mode;
    return config;
}