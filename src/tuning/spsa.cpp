#include "spsa.h"
#include "uci/uci.h"
#include <algorithm>
#include <functional>
#include <iostream>

int RFP_MARGIN = 75;
int ASPIRATION_MARGIN = 12;
int FUTILITY_MARGIN = 150;
int RAZOR_MARGIN = 350;
float LMR_VALUE = 1.0f;
float LMR_SCALAR = 2.0f;
int MAX_HISTORY = 16384;
int LMP_BASE = 3;
int NMP_DEPTH_CUTOFF = 3;
int RAZORING_DEPTH_MAX = 3;
int IIR_DEPTH_CUTOFF = 4;
int FP_DEPTH_MAX = 5;
int LMR_CUTOFF = 3;
int LMR_DEPTH_CUTOFF = 3;
int ASPIRATION_DEPTH_CUTOFF = 5;
float ASPIRATION_SCALAR = 1.25;

static int lmr_value_scaled = 100;  // 1.0 × 100
static int lmr_scalar_scaled = 200; // 2.0 × 100
static int aspiration_scalar_scaled = 125;

static std::vector<SPSAParam> registry;

const std::vector<SPSAParam>& spsaParams() { return registry; }

void printSPSAParams() {
    for (const auto& p : registry) {
        // OpenBench SPSA submission format
        std::cout << p.name << ", int, " << p.def << ", " << p.min << ", " << p.max << ", " << p.step << ", " << p.lr << "\n";
    }
    std::cout.flush();
}

static void reg(const std::string& name, int* ptr, int def, int min, int max, double step, double lr, std::function<void(int)> setter = nullptr) {
    registry.push_back({ name, ptr, def, min, max, step, lr });
    if (!setter) {
        setter = [ptr, min, max](int v) { *ptr = std::clamp(v, min, max); };
    }
    setOptions(name, { min, max, def, setter });
}

void initSPSA() {
    reg("RFP_MARGIN", &RFP_MARGIN, 75, 0, 300, 15, 0.002);
    reg("ASPIRATION_MARGIN", &ASPIRATION_MARGIN, 12, 1, 50, 3, 0.002);
    reg("FUTILITY_MARGIN", &FUTILITY_MARGIN, 150, 0, 500, 25, 0.002);
    reg("RAZOR_MARGIN", &RAZOR_MARGIN, 350, 0, 800, 40, 0.002);

    // These are scaled by 100x
    reg("LMR_Value", &lmr_value_scaled, 100, 0, 300, 15, 0.002, [](int v) {
        lmr_value_scaled = std::clamp(v, 0, 300);
        LMR_VALUE = lmr_value_scaled / 100.0f;
    });
    reg("LMR_Scalar", &lmr_scalar_scaled, 200, 50, 500, 25, 0.002, [](int v) {
        lmr_scalar_scaled = std::clamp(v, 50, 500);
        LMR_SCALAR = lmr_scalar_scaled / 100.0f;
    });

    reg("MAX_HISTORY", &MAX_HISTORY, 16384, 1024, 65536, 3277, 0.002);
    reg("LMP_BASE", &LMP_BASE, 3, 1, 10, 1, 0.002);
    reg("NMP_DEPTH_CUTOFF", &NMP_DEPTH_CUTOFF, 3, 1, 5, 1, 0.002);
    reg("RAZORING_DEPTH_MAX", &RAZORING_DEPTH_MAX, 3, 1, 6, 1, 0.002);
    reg("IIR_DEPTH_CUTOFF", &IIR_DEPTH_CUTOFF, 4, 2, 8, 1, 0.002);
    reg("FP_DEPTH_MAX", &FP_DEPTH_MAX, 5, 2, 8, 1, 0.002);
    reg("LMR_CUTOFF", &LMR_CUTOFF, 3, 1, 6, 1, 0.002);
    reg("LMR_DEPTH_CUTOFF", &LMR_DEPTH_CUTOFF, 3, 1, 5, 1, 0.002);
    reg("ASPIRATION_DEPTH_CUTOFF", &ASPIRATION_DEPTH_CUTOFF, 5, 3, 8, 1, 0.002);
    reg("ASPIRATION_SCALAR", &aspiration_scalar_scaled, 125, 100, 250, 5, 0.002, [](int v) {
        aspiration_scalar_scaled = std::clamp(v, 100, 200);
        ASPIRATION_SCALAR = aspiration_scalar_scaled / 100.0f;
    });
}
