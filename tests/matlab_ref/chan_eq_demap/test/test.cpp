#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/rx/chan_eq_demap.hpp"
#include "sam/utils/vec_io.hpp"
#include "sam/utils/argsparser.hpp"

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
#include <stdexcept>

#define N_RX 2
#define N_LAYERS 2

using ChanEqDemap = sam::rx::ChanEqDemap<N_RX, N_LAYERS>;

struct TestParams
{
    std::string input_rx_grid_bin;
    std::string input_hp_bin;
    std::string reference_llp_bin;
    ChanEqDemap::Config chan_eqd_cfg;
};

TestParams get_test_params(TestArgs args);

int main(int argc, char* argv[]) {
    // Defaults for now, as nothing else supported yet.
    TestArgs args = {Mode::NR5G, Channel::PDSCH, Bandwidth::BW_20, DMRSMode::Mode2};

    ArgsParser::parse(args, argc, argv);

    std::cout << "\n--- CHANNEL ESTIMATER Test ---\n";
    std::cout << args;

    TestParams params = get_test_params(args);
    ChanEqDemap::Config& chan_eqd_cfg = params.chan_eqd_cfg;

    itpp::cvec in_rx_grid = read_sc16_as_cvec(params.input_rx_grid_bin, 8, chan_eqd_cfg.n_sc * N_SYM * N_RX);
    itpp::cvec in_hp = read_sc16_as_cvec(params.input_hp_bin, 7, chan_eqd_cfg.n_sc * N_RX * N_LAYERS);
    itpp::vec out_llrs(chan_eqd_cfg.n_sc * chan_eqd_cfg.qm_mode * N_LAYERS);

    /////////////////////////////////////
    // Channel Estimation - Start
    /////////////////////////////////////

    ChanEqDemap chan_eqd;
    sam::Control ctrl;
    ChanEqDemap::Inputs chan_eqd_in;
    ChanEqDemap::Outputs chan_eqd_out;

    for (auto rx = 0; rx < N_RX; rx++)
        chan_eqd_in.rx_grid[rx].samples = in_rx_grid.mid(rx * N_SYM * chan_eqd_cfg.n_sc, chan_eqd_cfg.n_sc);

    for (auto layer = 0; layer < N_LAYERS; layer++)
        for (auto rx = 0; rx < N_RX; rx++)
            chan_eqd_in.hp[rx][layer].samples = in_hp.mid((rx + (layer * 2)) * chan_eqd_cfg.n_sc, chan_eqd_cfg.n_sc);

    chan_eqd.eval(chan_eqd_in, chan_eqd_out, ctrl, chan_eqd_cfg);

    int bits = params.chan_eqd_cfg.qm_mode;
    for (auto sc = 0; sc < chan_eqd_cfg.n_sc; sc++)
        for (auto layer = 0; layer < N_LAYERS; layer++)
            out_llrs.set_subvector((sc * N_LAYERS * bits) + (layer * bits), chan_eqd_out.llrs[layer].samples.mid(sc * bits, bits));
    
    for (auto sc = 0; sc < 10; sc++)
        for (auto layer = 0; layer < N_LAYERS; layer++)
            std::cout << (sc * N_LAYERS * bits) + (layer * bits) << std::endl;

    /////////////////////////////////////
    // Channel Estimation - End
    /////////////////////////////////////

    itpp::ivec ref_llrs = read_int8_as_ivec(params.reference_llp_bin, chan_eqd_cfg.n_sc * chan_eqd_cfg.qm_mode * N_LAYERS);

    double mse = itpp::mean(itpp::abs(itpp::round_i(out_llrs) - ref_llrs));
    double snr = 20 * std::log10(itpp::mean(itpp::abs(ref_llrs)) / mse);

    std::cout << "\n--- Result ---\n";
    std::cout << "MSE: " << mse << "\n";
    std::cout << "SNR: " << snr << "\n";
    
    const double threshold = 40.0;
    if (snr < threshold) {
        std::cerr << "Test failed: SNR is below " << threshold << " dB.\n";
        return 1;
    }

    return 0;
}

TestParams get_test_params(TestArgs args)
{
    TestParams params;
    if (args.mode == Mode::LTE || args.dmrs_mode != DMRSMode::Mode2)
    {
        std::runtime_error("LTE or DMRS Mode other than 2 are not supported yet");
    }
    else if (args.mode == Mode::NR5G)
    {
        const std::string test_vector_dir = args.toVectorsDir("../test_vectors");
        std::cout << "Test Vector Dir: " << test_vector_dir << "\n";

        params.input_rx_grid_bin = test_vector_dir + "5g_output_rxSubframe.bin";
        params.input_hp_bin = test_vector_dir + "5g_output_H.bin";
        params.reference_llp_bin = test_vector_dir + "5g_output_demodulated.bin";

        params.chan_eqd_cfg.n_sc = 3300;
        params.chan_eqd_cfg.qm_mode = 4;
        params.chan_eqd_cfg.n_var = 0.00023516;
    }

    return params;
}