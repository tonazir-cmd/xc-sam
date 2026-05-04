#pragma once

#include "sam/core/base.hpp"
#include "sam/core/signal_types.hpp"

#include <itpp/itbase.h>
#include <itpp/comm/modulator.h>

#include <cstdint>
#include <cassert>
#include <vector>

namespace sam
{
namespace rx
{

class ChanEqDemap : public IProcessingBlock
{
public:
    // Replaced arrays of vectors with clean matrix/vector structures
    struct Inputs
    {
        // 2D grid of references to the cvecs output by ChanEst. 
        // Dimensions: [n_rx][n_layers]. Each contains an n_sc length cvec.
        std::vector<std::vector<sam::SignalData>> hp;

        // Dimensions: [n_rx]. Each contains an n_sc length cvec.
        std::vector<sam::SignalData> rx_grid;
    };

    struct Outputs
    {
        // LLRs per layer: size n_layers, each n_sc * qm_mode
        std::vector<itpp::vec> llrs;
    };

    struct Config
    {
        uint16_t n_sc;
        uint8_t  qm_mode;
        uint8_t  n_rx;
        uint8_t  n_layers;
        double   n_var;
    };

    ChanEqDemap() = default;
    ~ChanEqDemap() override = default;

    ChanEqDemap(const ChanEqDemap&)            = delete;
    ChanEqDemap& operator=(const ChanEqDemap&) = delete;

    ChanEqDemap(ChanEqDemap&&)            = default;
    ChanEqDemap& operator=(ChanEqDemap&&) = default;

    void reset() override {};

    void eval(const Inputs&      in,
              Outputs&           out,
              const Control&     ctrl,
              const Config&      cfg,
              const ExecContext& ctx);
};

} // namespace rx
} // namespace sam