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

template<size_t N_RX, size_t N_LAYERS>
class ChanEqDemap : public IProcessingBlock
{
public:
    // Replaced arrays of vectors with clean matrix/vector structures
    struct Inputs
    {
        // 2D grid of references to the cvecs output by ChanEst. 
        // Dimensions: [n_rx][n_layers]. Each contains an n_sc length cvec.
        sam::SignalData* hp[N_RX][N_LAYERS];

        // Dimensions: [n_rx]. Each contains an n_sc length cvec.
        sam::SignalData* rx_grid[N_RX];
    };

    struct Outputs
    {
        // LLRs per layer: size n_layers, each n_sc * qm_mode
        sam::RealData* llrs[N_LAYERS];
    };

    struct Config
    {
        uint16_t n_sc;
        uint8_t  qm_mode;
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
              const Config&      cfg);
};

} // namespace rx
} // namespace sam