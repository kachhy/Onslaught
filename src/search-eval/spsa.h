#ifndef SPSA_H
#define SPSA_H

#define SEARCH_TUNABLES(INT, REAL)                                             \
    /* Pruning and reduction margins */                                        \
    INT (RFP_MARGIN,                84,    20,   200,    8)                    \
    INT (FUTILITY_MARGIN,           90,    20,   250,   11)                    \
    INT (RAZOR_MARGIN,             492,   150,   900,   37)                    \
    INT (PROB_BETA_OFFSET,         184,    50,   400,   17)                    \
    INT (ASPIRATION_MARGIN,         15,     5,    60,    3)                    \
    INT (HIST_LMR_DIVISOR,        8192,  2048, 16384,  700)                    \
    INT (LMR_CUTNODE,                2,     0,     4,    1)                    \
    /* Depth cutoffs */                                                        \
    INT (NMP_DEPTH_CUTOFF,           3,     1,     8,    1)                    \
    INT (RAZORING_DEPTH_MAX,         2,     1,     6,    1)                    \
    INT (IIR_DEPTH_CUTOFF,           3,     1,     8,    1)                    \
    INT (FP_DEPTH_MAX,               4,     1,    10,    1)                    \
    INT (LMP_DEPTH_MAX,              8,     2,    16,    1)                    \
    INT (LMP_BASE,                   3,     1,     8,    1)                    \
    INT (LMR_MOVES_CUTOFF,           6,     2,    12,    1)                    \
    INT (LMR_DEPTH_CUTOFF,           1,     1,     6,    1)                    \
    INT (ASPIRATION_DEPTH_CUTOFF,    3,     1,     8,    1)                    \
    INT (SEE_DEPTH_MAX,              8,     3,    15,    1)                    \
    INT (SEE_CAPTURE_MARGIN,        20,     5,    60,    3)                    \
    INT (TT_HIT_DEPTH_BONUS,         6,     0,    12,    1)                    \
    INT (RFP_DEPTH_MAX,              6,     3,    12,    1)                    \
    /* Null move reduction: NMP_BASE + depth / NMP_DEPTH_DIVISOR */            \
    INT (NMP_BASE,                   3,     1,     6,    1)                    \
    INT (NMP_DEPTH_DIVISOR,          6,     2,    12,    1)                    \
    /* ProbCut */                                                              \
    INT (PROBCUT_DEPTH_MIN,          6,     3,    12,    1)                    \
    INT (PROBCUT_TT_DEPTH_MARGIN,    4,     1,     6,    1)                    \
    INT (PROBCUT_REDUCTION,          4,     2,     8,    1)                    \
    /* LMR reduction adjustments, in plies */                                  \
    INT (LMR_DEPTH_CAP,              2,     1,     5,    1)                    \
    INT (LMR_TT_CAPTURE,             1,     0,     3,    1)                    \
    INT (LMR_IMPROVING,              1,     0,     3,    1)                    \
    INT (LMR_NO_TT_PV,               1,     0,     3,    1)                    \
    /* Floating point terms, stored scaled by 1e6. The scale has to hold the   \
       original six-decimal constants exactly */                               \
    REAL(LMR_VALUE,             828061,  200000, 1600000,  70000, 1000000.0)   \
    REAL(LMR_SCALAR,           2172229, 1000000, 4000000, 150000, 1000000.0)   \
    REAL(ASPIRATION_SCALAR,    1214100,  500000, 3000000, 125000, 1000000.0)

#ifdef SPSA_TUNE

// A scaled integer that reads as a double everywhere it is used
struct TunableReal {
    int raw;
    double scale;
    constexpr operator double() const { return static_cast<float>(raw / scale); }
};

#define TUNABLE_DECL_INT(name, def, min, max, cend) extern int name;
#define TUNABLE_DECL_REAL(name, def, min, max, cend, scale) extern TunableReal name;

#else

#define TUNABLE_DECL_INT(name, def, min, max, cend) constexpr int name = def;
#define TUNABLE_DECL_REAL(name, def, min, max, cend, scale) constexpr double name = static_cast<float>((def) / (scale));

#endif // SPSA_TUNE

SEARCH_TUNABLES(TUNABLE_DECL_INT, TUNABLE_DECL_REAL)

#undef TUNABLE_DECL_INT
#undef TUNABLE_DECL_REAL

// Registers a UCI spin option per tunable
void registerTunables();

// Prints the OpenBench SPSA input block for every parameter
void printSPSAInput();

#endif // SPSA_H
