#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/rx/chan_est.hpp"
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
    std::string input_bin;
    std::string dmrs_bin;
    std::string reference_bin;
    sam::rx::ChannelEstimator::Config chan_est_cfg;
};

TestParams get_test_params(TestArgs args);

int main(int argc, char* argv[]) {
    // Defaults for now, as nothing else supported yet.
    TestArgs args = {Mode::NR5G, Channel::PDSCH, Bandwidth::BW_20, DMRSMode::Mode2};

    ArgsParser::parse(args, argc, argv);

    std::cout << "\n--- CHANNEL ESTIMATER Test ---\n";
    std::cout << args;

    TestParams params = get_test_params(args);
    sam::rx::ChannelEstimator::Config& chan_est_cfg = params.chan_est_cfg;

    itpp::cvec in = read_sc16_as_cvec(params.input_bin, 8, chan_est_cfg.n_sc * N_SYM);
    

    itpp::cvec dmrs_seq = read_sc16_as_cvec(params.dmrs_bin, 15, chan_est_cfg.n_sc / 2);
    itpp::cvec out(chan_est_cfg.n_sc);

    /////////////////////////////////////
    // Channel Estimation - Start
    /////////////////////////////////////

    sam::rx::ChannelEstimator chan_est;
    sam::Control ctrl;

    sam::SignalData rx_grid;
    sam::SignalData dmrs;
    sam::rx::ChannelEstimator::Inputs chan_est_in;
    sam::SignalData chan_est_out;

    const size_t dmrs_sym_idx = 2;
    rx_grid.samples = in.mid(dmrs_sym_idx * chan_est_cfg.n_sc, chan_est_cfg.n_sc);
    dmrs.samples = dmrs_seq;

    chan_est_in.rx = &rx_grid;
    chan_est_in.dmrs = &dmrs;

    chan_est.eval(chan_est_in, chan_est_out, ctrl, chan_est_cfg);
    
    out = chan_est_out.samples;

    /////////////////////////////////////
    // Channel Estimation - End
    /////////////////////////////////////

    itpp::cvec ref = read_sc16_as_cvec(params.reference_bin, 7, chan_est_cfg.n_sc);

    double mse = itpp::mean(itpp::abs(out - ref));
    double snr = 20 * std::log10(itpp::mean(itpp::abs(ref)) / mse);

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

        params.input_bin = test_vector_dir + "5g_output_rxSubframe.bin";
        params.dmrs_bin = test_vector_dir + "5g_input_refsym_dmrs_all.bin";
        params.reference_bin = test_vector_dir + "5g_output_H.bin";

        params.chan_est_cfg.n_sc = 3300;
        params.chan_est_cfg.dmrs_pattern = sam::rx::ChannelEstimator::DMRSPattern::Even;
    }

    return params;
}