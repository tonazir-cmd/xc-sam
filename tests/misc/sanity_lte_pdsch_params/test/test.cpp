#include "sam/utils/lte_pdsch_params.h"
#include <iostream>
#include <iomanip>

template<typename T>
static void printVec(const char* label, const std::vector<T>& v) {
    std::cout << label << " [" << v.size() << "] = { ";
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::cout << v[i];
        if (i + 1 < v.size()) std::cout << ", ";
    }
    std::cout << " }\n";
}

int main() {
    try {
        LTEPdschParams cfg = LTEPdschParamsLoader::load("params.txt");

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "=== LTEPdschParams ===\n";
        std::cout << "Nsym         = " << cfg.Nsym         << "\n";
        std::cout << "Nsc          = " << cfg.Nsc          << "\n";
        std::cout << "Nrx          = " << cfg.Nrx          << "\n";
        std::cout << "CellRefP     = " << cfg.CellRefP     << "\n";
        std::cout << "nDLRB        = " << cfg.nDLRB        << "\n";
        std::cout << "nFFT         = " << cfg.nFFT         << "\n";
        std::cout << "NLayers      = " << cfg.NLayers      << "\n";
        printVec(   "Ncp (int)    ", cfg.Ncp);
        std::cout << "TXScheme     = \"" << cfg.TXScheme   << "\"\n";
        std::cout << "DCIFormat    = \"" << cfg.DCIFormat  << "\"\n";
        std::cout << "FreqOffset   = " << cfg.FreqOffset   << "\n";
        std::cout << "TimingOffset = " << cfg.TimingOffset  << "\n";
        std::cout << "CPF          = " << cfg.CPF          << "\n";
        std::cout << "NOISE_Est    = " << cfg.NOISE_Est    << "\n";
        printVec(   "PC_MAT       ", cfg.PC_MAT);
        std::cout << "N_CW         = " << cfg.N_CW         << "\n";
        std::cout << "N_L          = " << cfg.N_L          << "\n";
        std::cout << "N_CB         = " << cfg.N_CB         << "\n";
        std::cout << "RNTI         = " << cfg.RNTI         << "\n";
        printVec(   "data (int)   ", cfg.data);
        std::cout << "err0         = " << cfg.err0         << "\n";
        std::cout << "err1         = " << cfg.err1         << "\n";
        std::cout << "BER_0        = " << cfg.BER_0        << "\n";
        std::cout << "BER_1        = " << cfg.BER_1        << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}