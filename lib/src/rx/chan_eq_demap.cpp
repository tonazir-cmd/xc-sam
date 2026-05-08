#include "sam/rx/chan_eq_demap.hpp"
#include "sam/utils/matlab_ported.hpp"
#include <itpp/base/specmat.h>
#include <itpp/base/algebra/inv.h>

namespace sam
{
namespace rx
{
template<size_t N_RX, size_t N_LAYERS>
void ChanEqDemap<N_RX, N_LAYERS>::eval(const Inputs&      in,
                                       Outputs&           out,
                                       const Control&     ctrl,
                                       const Config&      cfg)
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
    
    for (size_t rx = 0; rx < N_RX; rx++)
        assert(in.rx_grid[rx]->samples.length() == cfg.n_sc);

    for (size_t rx = 0; rx < N_RX; rx++)
        for (size_t layer = 0; layer < N_LAYERS; layer++)
            assert(in.hp[rx][layer]->samples.length() == cfg.n_sc);

    for (size_t layer = 0; layer < N_LAYERS; layer++)
        assert(out.llrs[layer]->samples.length() == cfg.n_sc * cfg.qm_mode);

    // -------------------------------------------------------------------------
    // Processing
    // -------------------------------------------------------------------------

    itpp::QAM modem(1 << cfg.qm_mode);
    itpp::cmat I = itpp::eye_c(N_LAYERS);
    std::complex<double> noise_var(cfg.n_var, 0.0);

    itpp::cmat Hp(N_RX, N_LAYERS);
    itpp::cvec r_k(N_RX);

    for (uint16_t k = 0; k < cfg.n_sc; ++k)
    {
        // 1. Assemble the Hp matrix for subcarrier k
        for (uint8_t rx = 0; rx < N_RX; ++rx) {
            for (uint8_t layer = 0; layer < N_LAYERS; ++layer) {
                Hp(rx, layer) = in.hp[rx][layer]->samples(k);
            }
        }

        // 2. Assemble the received vector for subcarrier k
        for (uint8_t rx = 0; rx < N_RX; ++rx) {
            r_k(rx) = in.rx_grid[rx]->samples(k);
        }

        // 3. Matrix Math
        itpp::cmat Hp_H = Hp.hermitian_transpose();
        itpp::cmat T = Hp_H * Hp + noise_var * I;
        itpp::cmat W = itpp::inv(T) * Hp_H;
        itpp::cvec req = W * r_k;

        // 4. Soft Demapping
        for (uint8_t s = 0; s < N_LAYERS; ++s)
        {
            itpp::cvec sym(1);
            sym(0) = req(s);
            
            itpp::vec bits = matlab_ported::demapper_5g(sym, cfg.n_var, (1 << cfg.qm_mode)).get_row(0);
            uint32_t offset = k * cfg.qm_mode;
            
            out.llrs[s]->samples.set_subvector(offset, bits);
        }
    }
}

template class ChanEqDemap<2, 2>;

} // namespace rx
} // namespace sam