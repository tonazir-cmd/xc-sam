#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"
#include "sam/rx/chan_eq_demap.hpp"

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

int main() 
{
    std::cout << "\n--- CHAN_EQ_DEMAP Test ---\n";

    // -------------------------------------------------------------------------
    // 1. Config
    // -------------------------------------------------------------------------
    sam::rx::ChanEqDemap::Config cfg;
    cfg.n_sc     = 1200;
    cfg.qm_mode  = 6;       // 64-QAM -> 6 LLRs per symbol
    cfg.n_rx     = 2;
    cfg.n_layers = 2;
    cfg.n_var    = 0.01;

    // -------------------------------------------------------------------------
    // 2. Set up Interface Structures
    // -------------------------------------------------------------------------
    sam::rx::ChanEqDemap::Inputs in;
    sam::rx::ChanEqDemap::Outputs out;
    
    sam::Control ctrl;
    ctrl.enable = true;
    ctrl.bypass = false;
    
    sam::ExecContext ctx{0};

    // -------------------------------------------------------------------------
    // 3. Initialize Dummy Data
    // -------------------------------------------------------------------------
    // Allocate [n_rx][n_layers] for hp and [n_rx] for rx_grid
    in.hp.resize(cfg.n_rx, std::vector<sam::SignalData>(cfg.n_layers));
    in.rx_grid.resize(cfg.n_rx);

    // Resize the inner itpp::cvecs to n_sc
    for (uint8_t r = 0; r < cfg.n_rx; ++r) {
        in.rx_grid[r].samples.set_size(cfg.n_sc);
        for (uint8_t l = 0; l < cfg.n_layers; ++l) {
            in.hp[r][l].samples.set_size(cfg.n_sc);
        }
    }

    // Mock Channel Estimate (Identity Matrix across all subcarriers)
    // Using ITPP vector instructions to populate the entire 1200 subcarriers instantly
    // h[0][0] = 1, h[0][1] = 0
    // h[1][0] = 0, h[1][1] = 1
    in.hp[0][0].samples.ones();
    in.hp[0][1].samples.zeros();
    in.hp[1][0].samples.zeros();
    in.hp[1][1].samples.ones();

    // Mock RX Grid (Constant received symbols across all subcarriers)
    for (uint16_t k = 0; k < cfg.n_sc; ++k)
    {
        in.rx_grid[0].samples(k) = std::complex<double>( 0.7, 0.7);
        in.rx_grid[1].samples(k) = std::complex<double>(-0.7, 0.7);
    }

    // -------------------------------------------------------------------------
    // 4. Execution
    // -------------------------------------------------------------------------
    sam::rx::ChanEqDemap chan_eq_demap;
    chan_eq_demap.eval(in, out, ctrl, cfg, ctx);

    // -------------------------------------------------------------------------
    // 5. Assertions and Verifications
    // -------------------------------------------------------------------------
    const int expected_llr_len = cfg.n_sc * cfg.qm_mode; 
    
    assert(out.llrs.size() == cfg.n_layers);
    assert(out.llrs[0].length() == expected_llr_len);
    assert(out.llrs[1].length() == expected_llr_len);

    std::cout << "LLR sizes OK: llr0=" << out.llrs[0].length()
              << " llr1=" << out.llrs[1].length() << "\n";

    // Print first subcarrier's LLRs using itpp subvector utility
    std::cout << "\n[k=0] Layer-0 LLRs: " << out.llrs[0].left(cfg.qm_mode) << "\n";
    std::cout << "[k=0] Layer-1 LLRs: " << out.llrs[1].left(cfg.qm_mode) << "\n";

    std::cout << "\nPASS\n";
    return 0;
}