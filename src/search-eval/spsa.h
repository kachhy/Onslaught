#ifndef SPSA_H
#define SPSA_H

#define SEARCH_TUNABLES(INT, REAL)                                             \
    /* Pruning and reduction margins */                                        \
    INT (RFP_MARGIN,                84,    20,   200,    8)                    \
    INT (FUTILITY_MARGIN,           90,    20,   250,   11)                    \
    INT (RAZOR_MARGIN,             492,   150,   900,   37)                    \
    INT (PROB_BETA_OFFSET,         184,    50,   400,   17)                    \
    INT (ASPIRATION_MARGIN,         15,     5,    60,    3)                    \
    INT (CAPTURE_MVV_SCALE,        247,     1,  1024,   16)                    \
    /* Capture history: bonus = min(MUL * depth, MAX), and the SEE             \
       thresholds move by history / DIV */                                     \
    INT (CAPHIST_BONUS_MUL,         99,    16,   512,   24)                    \
    INT (CAPHIST_BONUS_MAX,       1521,   256,  4096,  192)                    \
    INT (CAPHIST_SEE_DIV,           66,    16,   512,   24)                    \
    INT (CAPHIST_PRUNE_DIV,         54,    16,   512,   24)                    \
    /* Depth cutoffs */                                                        \
    INT (NMP_DEPTH_CUTOFF,           3,     1,     8,    1)                    \
    INT (RAZORING_DEPTH_MAX,         2,     1,     6,    1)                    \
    INT (IIR_DEPTH_CUTOFF,           3,     1,     8,    1)                    \
    INT (FP_DEPTH_MAX,               4,     1,    10,    1)                    \
    INT (LMP_DEPTH_MAX,              8,     2,    16,    1)                    \
    INT (LMP_BASE,                   3,     1,     8,    1)                    \
    INT (LMR_MOVES_CUTOFF,           2,     1,     8,    1)                    \
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
    /* LMR reduction adjustments, in 1/LMR_GRAIN (1024) ply */                 \
    INT (LMR_CUTNODE,             1934,     0,  4096,  128)                    \
    INT (LMR_NO_TT_PV,            868,     0,  3072,  128)                    \
    INT (LMR_PV_NODE,             2011,     0,  4096,  128)                    \
    INT (LMR_IMPROVING,           1015,     0,  3072,  128)                    \
    INT (LMR_KILLER_REDUCTION,    929,     0,  3072,  128)                    \
    INT (LMR_GIVES_CHECK_REDUCTION, 669,   0,  3072,  128)                    \
    /* Correction history scalars. */                                         \
    INT (CORRH_PAWN_CONTRIBUTATION,         130,    50,     256,   10)         \
    INT (CORRH_NONPAWN_CONTRIBUTATION,      126,    50,     256,   10)         \
    INT (CORR_MAX_BONUS,                    2253,   1024,   8192,  256)        \
    /* Movepicker/LMR history contributions */                                 \
    INT (PICKER_CONT_HIST_WEIGHT,          154,    16,    1024,   24)          \
    INT (LMR_SCORE_HIST_WEIGHT,            137,    16,    1024,   24)          \
    INT (LMR_CONT_HIST_WEIGHT,             141,    16,    1024,   24)          \
    /* Floating point terms, stored scaled by 1e6. The scale has to hold the   \
       original six-decimal constants exactly */                               \
    REAL(LMR_VALUE,            1020575,  200000, 1600000,  70000, 1000000.0)   \
    REAL(LMR_SCALAR,           1659721, 1000000, 4000000, 150000, 1000000.0)   \
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
