#include "sam/utils/lte_pdsch_params.h"

std::string LTEPdschParamsLoader::asciiVecToString(const std::vector<float>& v) {
    std::string s;
    s.reserve(v.size());
    for (float f : v) {
        int c = static_cast<int>(f);
        if (c >= 32 && c < 127) s += static_cast<char>(c);
    }
    return s;
}

LTEPdschParams LTEPdschParamsLoader::load(const std::string& filepath) {
    ParamsReader p(filepath);
    LTEPdschParams  out;

    // ---- Grid / dimensions ----
    out.Nsym     = p.get<int>("Nsym");
    out.Nsc      = p.get<int>("Nsc");
    out.Nrx      = p.get<int>("Nrx");
    out.CellRefP = p.get<int>("CellRefP");
    out.nDLRB    = p.get<int>("nDLRB");
    out.nFFT     = p.get<int>("nFFT");
    out.NLayers  = p.get<int>("NLayers");

    // ---- Cyclic prefix: integer samples ----
    out.Ncp = p.getArray<int>("Ncp");
    if (out.Ncp.size() * 2 == static_cast<size_t>(out.Nsym))
    {
        out.Ncp.reserve(out.Ncp.size() * 2);
        out.Ncp.insert(out.Ncp.end(), out.Ncp.begin(), out.Ncp.end());
    }

    if (out.Ncp.size() != static_cast<size_t>(out.Nsym))
        std::runtime_error("Ncp size does not match Nsym or Nsym / 2");

    // ---- TX scheme / DCI format: ASCII float arrays -> string ----
    out.TXScheme  = asciiVecToString(p.getArray<float>("TXScheme"));
    out.DCIFormat = asciiVecToString(p.getArray<float>("PDSCH.DCIFormat"));

    // ---- Frequency / timing ----
    out.FreqOffset   = p.get<float>("Freq Offset");
    out.TimingOffset = p.get<int>  ("Timing Offset");
    out.CPF          = p.get<float>("CPF");

    // ---- Noise ----
    out.NOISE_Est = p.get<float>("NOISE_Est");

    // ---- Precoding matrix: float (fractional weights) ----
    out.PC_MAT = p.getArray<float>("PC_MAT");

    // ---- Counts ----
    out.N_CW = p.get<int>("N_CW");
    out.N_L  = p.get<int>("N_L");
    out.N_CB = p.get<int>("N_CB");

    // ---- RNTI ----
    out.RNTI = p.get<uint16_t>("RNTI");

    // ---- Transport block bits: binary int ----
    out.data = p.getArray<int>("data");

    // ---- Error / BER ----
    out.err0  = p.get<float>("err0");
    out.err1  = p.get<float>("err1");
    out.BER_0 = p.get<float>("BER_0");
    out.BER_1 = p.get<float>("BER_1");

    return out;
}