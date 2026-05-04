#include "sam/rx/chan_eq_demap.hpp"

#include <itpp/base/specmat.h>
#include <itpp/base/algebra/inv.h>

namespace sam
{
namespace rx
{

void ChanEqDemap::eval(const Inputs&      in,
                       Outputs&           out,
                       const Control&     ctrl,
                       const Config&      cfg,
                       const ExecContext& ctx)
{
    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------
    if (!ctrl.enable) return;
    if (ctrl.bypass) return;

    // -------------------------------------------------------------------------
    // Assertions
    // -------------------------------------------------------------------------
    assert(cfg.n_sc > 0);
    assert(in.hp.size() == cfg.n_rx);
    assert(in.rx_grid.size() == cfg.n_rx);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------
    
    out.llrs.assign(cfg.n_layers, itpp::zeros(cfg.n_sc * cfg.qm_mode));

    itpp::QAM modem(1 << cfg.qm_mode);
    itpp::cmat I = itpp::eye_c(cfg.n_layers);
    std::complex<double> noise_var(cfg.n_var, 0.0);

    itpp::cmat Hp(cfg.n_rx, cfg.n_layers);
    itpp::cvec r_k(cfg.n_rx);

    for (uint16_t k = 0; k < cfg.n_sc; ++k)
    {
        // 1. Assemble the Hp matrix for subcarrier k
        for (uint8_t rx = 0; rx < cfg.n_rx; ++rx) {
            for (uint8_t tx = 0; tx < cfg.n_layers; ++tx) {
                Hp(rx, tx) = in.hp[rx][tx].samples(k);
            }
        }

        // 2. Assemble the received vector for subcarrier k
        for (uint8_t rx = 0; rx < cfg.n_rx; ++rx) {
            r_k(rx) = in.rx_grid[rx].samples(k);
        }

        // 3. Matrix Math
        itpp::cmat Hp_H = Hp.hermitian_transpose();
        itpp::cmat T = Hp_H * Hp + noise_var * I;
        itpp::cmat W = itpp::inv(T) * Hp_H;
        itpp::cvec req = W * r_k;

        // 4. Soft Demapping
        for (uint8_t s = 0; s < cfg.n_layers; ++s)
        {
            itpp::cvec sym(1);
            sym(0) = req(s);
            
            itpp::vec bits = modem.demodulate_soft_bits(sym, cfg.n_var);
            uint32_t offset = k * cfg.qm_mode;
            
            out.llrs[s].set_subvector(offset, bits);
        }
    }
}

} // namespace rx
} // namespace sam