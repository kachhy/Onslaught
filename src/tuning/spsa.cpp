#include "spsa.h"
#include "uci/uci.h"
#include <algorithm>
#include <functional>
#include <iostream>

int RFP_MARGIN = 84;
int ASPIRATION_MARGIN = 20;
int FUTILITY_MARGIN = 90;
int RAZOR_MARGIN = 492;
float LMR_VALUE = 0.828061;
float LMR_SCALAR = 2.172229;
int HIST_LMR_DIVISOR = 8192; // history score -> LMR reduction adjustment
int NMP_DEPTH_CUTOFF = 3;
int RAZORING_DEPTH_MAX = 2;
int IIR_DEPTH_CUTOFF = 3;
int FP_DEPTH_MAX = 4;
int LMP_DEPTH_MAX = 8;
int LMP_BASE = 3;
int LMR_MOVES_CUTOFF = 6;
int LMR_DEPTH_CUTOFF = 1;
int ASPIRATION_DEPTH_CUTOFF = 3;
float ASPIRATION_SCALAR = 1.290771;
int SEE_VALUES[6] = { 100, 300, 300, 500, 900, 20000 }; // PBNRQK
int SEE_DEPTH_MAX = 8;

static int lmr_value_scaled = 83;  // 1.0 × 100
static int lmr_scalar_scaled = 217; // 2.0 × 100

static std::vector<SPSAParam> registry;

const std::vector<SPSAParam>& spsaParams() { return registry; }

void printSPSAParams() {
    for (const auto& p : registry) {
        // OpenBench SPSA submission format
        std::cout << p.name << ", int, " << p.def << ", " << p.min << ", " << p.max << ", " << p.step << ", " << p.lr << "\n";
    }
}

static void reg(const std::string& name, int* ptr, int def, int min, int max, double step, double lr, std::function<void(int)> setter = nullptr) {
    registry.push_back({ name, ptr, def, min, max, step, lr });
    if (!setter) {
        setter = [ptr, min, max](int v) { *ptr = std::clamp(v, min, max); };
    }
    setOption(name, SpinOption{ min, max, def, setter });
}

void initSPSA() {
    reg("RFP_MARGIN", &RFP_MARGIN, 84, 0, 300, 15, 0.002);
    reg("ASPIRATION_MARGIN", &ASPIRATION_MARGIN, 20, 1, 50, 3, 0.002);
    reg("FUTILITY_MARGIN", &FUTILITY_MARGIN, 90, 0, 500, 25, 0.002);
    reg("RAZOR_MARGIN", &RAZOR_MARGIN, 492, 0, 800, 40, 0.002);

    // These are scaled by 100x
    reg("LMR_Value", &lmr_value_scaled, 83, 0, 300, 15, 0.002, [](int v) {
        lmr_value_scaled = std::clamp(v, 0, 300);
        LMR_VALUE = lmr_value_scaled / 100.0f;
    });
    reg("LMR_Scalar", &lmr_scalar_scaled, 217, 50, 500, 25, 0.002, [](int v) {
        lmr_scalar_scaled = std::clamp(v, 50, 500);
        LMR_SCALAR = lmr_scalar_scaled / 100.0f;
    });

    reg("HIST_LMR_DIVISOR", &HIST_LMR_DIVISOR, 8192, 1024, 65536, 3277, 0.002);
    reg("NMP_DEPTH_CUTOFF", &NMP_DEPTH_CUTOFF, 3, 1, 10, 1, 0.002);
    reg("RAZORING_DEPTH_MAX", &NMP_DEPTH_CUTOFF, 2, 1, 10, 1, 0.002);
    reg("IIR_DEPTH_CUTOFF", &IIR_DEPTH_CUTOFF, 3, 1, 10, 1, 0.002);
    reg("FP_DEPTH_MAX", &FP_DEPTH_MAX, 4, 1, 10, 1, 0.002);
    reg("LMP_DEPTH_MAX", &LMP_DEPTH_MAX, 8, 1, 10, 1, 0.002);
    reg("LMP_BASE", &LMP_BASE, 3, 1, 10, 1, 0.002);
    reg("LMR_MOVES_CUTOFF", &LMR_MOVES_CUTOFF, 6, 1, 10, 1, 0.002);
    reg("LMR_DEPTH_CUTOFF", &LMR_DEPTH_CUTOFF, 1, 1, 10, 1, 0.002);
    reg("ASPIRATION_DEPTH_CUTOFF", &ASPIRATION_DEPTH_CUTOFF, 3, 1, 10, 1, 0.002);
    
    reg("SEE_PAWN", &SEE_VALUES[0], 100, 50, 200, 30, 0.002);
    reg("SEE_KNIGHT", &SEE_VALUES[1], 300, 200, 400, 40, 0.002);
    reg("SEE_BISHOP", &SEE_VALUES[2], 300, 200, 400, 40, 0.002);
    reg("SEE_ROOK", &SEE_VALUES[3], 500, 400, 800, 50, 0.002);
    reg("SEE_QUEEN", &SEE_VALUES[4], 900, 800, 1200, 50, 0.002);
    
    reg("SEE_DEPTH_MAX", &SEE_DEPTH_MAX, 3, 1, 10, 1, 0.002);
}
