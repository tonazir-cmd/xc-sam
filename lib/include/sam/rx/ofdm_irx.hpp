#pragma once

#include "sam/core/base.hpp"
#include "sam/core/constants.hpp"
#include "sam/core/signal_types.hpp"

#include "sam/rx/ofdm_demod.hpp"
#include "sam/rx/chan_est.hpp"
#include "sam/rx/chan_eq_demap.hpp"

#include <itpp/itbase.h>

namespace sam 
{
namespace rx
{ 

template <size_t N_RX, size_t N_LAYERS>
class OfdmIRX : public IProcessingBlock 
{
public:
    struct Inputs
    {
        SignalData rx[N_RX];
        SignalData dmrs[N_RX];
    };

    struct Outputs
    {
        RealData llrs[N_LAYERS];
    };

    struct Control
    {
        Control irx;
        Control demod[N_RX];
        Control chan_est[N_RX][N_LAYERS];
        Control chan_eq_demap;
    }
    
    struct Config
    {
        OfdmDemod::Config demod[N_RX];
        ChannelEstimator::Config chan_est[N_RX][N_LAYERS];
        ChanEqDemap<N_RX, N_LAYERS> chan_eq_demap;
    };

    OfdmIRX() = default;
    ~OfdmIRX() override = default;

    OfdmIRX(const OfdmIRX&)            = delete;
    OfdmIRX& operator=(const OfdmIRX&) = delete;

    OfdmIRX(OfdmIRX&&)            = default;
    OfdmIRX& operator=(OfdmIRX&&) = default;

    void reset();

    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg,
              const ExecContext& ctx);

private:
    OfdmDemod demod_[N_RX];
    ChannelEstimator chan_est_[N_RX][N_LAYERS];
    ChanEqDemap<N_RX, N_LAYERS> chan_eq_demap_;


};

}
}