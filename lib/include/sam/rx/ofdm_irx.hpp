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
        const SignalData* rx[N_RX];
        const SignalData* dmrs[N_LAYERS];
    };

    struct Outputs
    {
        RealData* llrs [N_LAYERS];
        bool valid = false;
    };

    struct Control
    {
        sam::Control irx;
        sam::Control demod[N_RX];
        sam::Control chan_est[N_RX][N_LAYERS];
        sam::Control chan_eq_demap;
    };
    
    struct Config
    {
        OfdmDemod::Config demod;
        ChannelEstimator::Config chan_est[N_LAYERS];
        typename ChanEqDemap<N_RX, N_LAYERS>::Config chan_eq_demap;
        size_t process_delay = 0;
        bool is_pilot_sym[N_SYM] = {false};
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
    ChannelEstimator chan_est_;
    ChanEqDemap<N_RX, N_LAYERS> chan_eq_demap_;
    SignalData rx_grid_[N_RX][N_SYM];
    SignalData h_[N_RX][N_LAYERS];
};

}
}