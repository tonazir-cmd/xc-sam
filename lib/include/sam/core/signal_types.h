// =============================================================================
// signal_types.h
// Xcelerium SAM Framework — Common Signal Data Structures
//
// These types form the standard I/O vocabulary shared across all blocks.
// They derive from sam::Array so that any block Output can be passed
// directly as the next block's Input without copying.
//
// Zero-Copy Mandate:
//   When used as eval() arguments, the ITPP containers inside these structs
//   MUST be initialised as non-owning views (shallow references) pointing
//   into an externally owned buffer.  Blocks that need to accumulate output
//   across multiple eval() calls hold their own owning containers as private
//   member variables.
// =============================================================================

#pragma once

#include "sam/core/base.h"

#include <itpp/itbase.h>    // itpp::vec, itpp::cvec

#include <vector>
#include <complex>

namespace sam {

// -----------------------------------------------------------------------------
// SignalData
//
// General-purpose complex signal container used for:
//   - Time-domain IQ samples  (post-IFFT, post-CP-add, post-DUC)
//   - Frequency-domain subcarrier data  (post-FFT, post-resource-mapping)
//
// The 'samples' field is an itpp::cvec. When passed as eval() Inputs or
// Outputs it must be a non-owning view. When held as a member variable it
// is a full owning container.
// -----------------------------------------------------------------------------
struct SignalData : public Array
{
    itpp::cvec samples;
};

} // namespace sam