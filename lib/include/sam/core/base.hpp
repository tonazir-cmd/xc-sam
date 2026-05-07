// =============================================================================
// base.h
// Xcelerium System Algorithm Modeling (SAM) Framework
// Core base structures and interfaces.
//
// All processing blocks must inherit from IProcessingBlock and conform to the
// interface discipline defined here.
// =============================================================================

#pragma once

#include <cstdint>

namespace sam {

// =============================================================================
// struct Array
//
// Purpose:
//   Base tag type for all block Input and Output structures.
//   Individual blocks derive strongly-typed I/O containers from this struct.
//   Enables the same buffer to be passed directly from one block's Outputs
//   to the next block's Inputs with zero copying.
//
// Design Rule:
//   ITPP containers held inside derived Array structs that are passed through
//   eval() MUST be initialised as non-owning views (shallow references) to
//   enforce the Zero-Copy Mandate.
//
// =============================================================================
struct Array
{
    // Intentionally empty — acts as a structural base / tag only.
    // Derived structs add strongly-typed ITPP container members.
};

// =============================================================================
// struct Control
//
// Purpose:
//   Carries per-call control and mode signals into eval().
//   Prevents external orchestration decisions from leaking into object state.
//
// Fields:
//   enable  — when false the block performs no computation and returns
//             immediately (output is left in its prior state).
//   bypass  — when true the block short-circuits: input is forwarded to
//             output unchanged (identity pass-through).
//
// Extension:
//   Block-specific control is added by inheriting from this struct.
//   Example:
//       struct EqualizerControl : public sam::Control {
//           bool update_estimate = false;
//           int  symbol_idx      = 0;
//       };
//
// =============================================================================
struct Control
{
    bool enable = true;
    bool bypass = false;
};


// =============================================================================
// struct ExecContext
//
// Purpose:
//   Carries formalised execution-time information into every eval() call.
//   Separates global time progression from local block state so that
//   no processing block needs to own or advance global time.
//
// Responsibility:
//   Top-level orchestrators (e.g. PhyTx, PhyRx) are responsible for
//   constructing and advancing ExecContext. All sub-blocks consume it
//   read-only via const reference.
//
// Fields:
//   sample_count    — absolute sample index since system start (monotonic)
//   symbol_idx      — OFDM symbol index within the current slot  [0..13]
//   slot_idx        — slot index within the current frame
//   frame_idx       — frame index
//   numerology      — 5G NR numerology (0=15kHz, 1=30kHz, ...)
//   start_of_frame  — true on the very first symbol of a new frame
//   end_of_frame    — true on the very last symbol of the current frame
//
// =============================================================================
struct ExecContext
{
    uint64_t sample_count   = 0;
    uint8_t  symbol_idx     = 0;
    uint8_t  slot_idx       = 0;
    uint16_t frame_idx      = 0;
    uint8_t  numerology     = 0;
    bool     start_of_frame = false;
    bool     end_of_frame   = false;
};


// =============================================================================
// class IProcessingBlock
//
// Purpose:
//   Lightweight abstract base class for all stateful processing components.
//
// Design Decision:
//   eval() is intentionally NOT declared here because each concrete block
//   requires strongly-typed Inputs, Outputs, Config, and Control structures.
//
// Contract:
//   Every concrete block MUST implement reset(). After reset() returns,
//   the block's behaviour must be identical to that of a freshly
//   constructed instance — deterministic initial conditions are mandatory.
//
// =============================================================================
class IProcessingBlock
{
public:
    virtual ~IProcessingBlock() = default;

    // -------------------------------------------------------------------------
    // reset()
    //
    // Reinitialises ALL internal state (filter memory, tracking variables,
    // decoder states, etc.). Must fully restore deterministic initial
    // conditions. Calling eval() after reset() must produce the same
    // output as calling eval() on a fresh instance with identical inputs.
    //
    // -------------------------------------------------------------------------
    virtual void reset() = 0;
};

} // namespace sam