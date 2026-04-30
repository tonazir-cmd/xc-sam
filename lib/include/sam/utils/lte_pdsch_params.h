#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "sam/utils/params_reader.h"

struct LTEPdschParams {
    // ---- Grid / dimensions ----
    int      Nsym     = 0;
    int      Nsc      = 0;
    int      Nrx      = 0;
    int      CellRefP = 0;
    int      nDLRB    = 0;
    int      nFFT     = 0;
    int      NLayers  = 0;

    // ---- Cyclic prefix lengths — integer samples ----
    std::vector<int> Ncp;       // [0]=160 (extended), [1..6]=144 (normal)

    // ---- TX scheme / DCI format (ASCII-decoded from float arrays) ----
    std::string TXScheme;       // e.g. "SpatialMux"
    std::string DCIFormat;      // e.g. "Format2"

    // ---- Frequency / timing ----
    float    FreqOffset   = 0.f; // fractional freq offset (float: sub-sample precision)
    int      TimingOffset = 0;   // sample-accurate timing offset
    float    CPF          = 0.f; // CP fraction

    // ---- Noise ----
    float    NOISE_Est = 0.f;

    // ---- Precoding matrix (flat, column-major) ----
    std::vector<float> PC_MAT;

    // ---- Code-word / layer / CB counts ----
    int N_CW = 0;
    int N_L  = 0;
    int N_CB = 0;

    // ---- RNTI ----
    uint16_t RNTI = 0;

    // ---- Transport block bits (binary: 0/1) ----
    std::vector<int> data;

    // ---- Error / BER ----
    float err0  = 0.f;
    float err1  = 0.f;
    float BER_0 = 0.f;
    float BER_1 = 0.f;
};

class LTEPdschParamsLoader {
public:
    static LTEPdschParams load(const std::string& filepath);

private:
    static std::string asciiVecToString(const std::vector<float>& v);
};