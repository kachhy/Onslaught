#ifndef TUNING_H
#define TUNING_H

#include "core/types.h"
#include "search-eval/eval.h"
#include "search-eval/terms.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// Ordered mapping: offset prefix, evaluation value, Trace member. Array lengths
// come from the evaluation values. Each score stores an adjacent MG/EG pair.
// Keep PST separate because black squares share mirrored white parameters.
#define TUNING_TERMS(SCALAR, ARRAY) \
    ARRAY(MATERIAL, material_values, material) \
    SCALAR(TEMPO, TEMPO, tempo) \
    ARRAY(MOBILITY, MOBILITY, mobility) \
    SCALAR(PAWN_PHALANX, PAWN_PHALANX, pawn_phalanx) \
    SCALAR(DOUBLED_PAWNS, DOUBLED_PAWNS, doubled_pawns) \
    SCALAR(BACKWARDS_PAWN, BACKWARDS_PAWN, backwards_pawn) \
    SCALAR(ISOLATED_PAWN, ISOLATED_PAWN, isolated_pawn) \
    ARRAY(PAWN_PROTECTION, PAWN_PROTECTION, pawn_protection) \
    ARRAY(PASSED_PAWNS, PASSED_PAWNS, passed_pawns) \
    SCALAR(KNIGHT_OUTPOST, KNIGHT_OUTPOST, knight_outpost) \
    SCALAR(KNIGHT_BEHIND_PAWN, KNIGHT_BEHIND_PAWN, knight_behind_pawn) \
    ARRAY(KNIGHT_PAWN_ADJ, KNIGHT_PAWN_ADJ, knight_pawn_adj) \
    SCALAR(BISHOP_PAIR, BISHOP_PAIR, bishop_pair) \
    SCALAR(BISHOP_CTRL_PENALTY, BISHOP_CONTROL_PENALTY, bishop_control_penalty) \
    SCALAR(BAD_BISHOP, BAD_BISHOP, bad_bishop) \
    SCALAR(BISHOP_BLOCKING_PAWN, BISHOP_BLOCKING_PAWN, bishop_blocking_pawn) \
    SCALAR(BISHOP_BEHIND_PAWN, BISHOP_BEHIND_PAWN, bishop_behind_pawn) \
    SCALAR(ROOK_SEVENTH, ROOK_ON_SEVENTH_RANK, rook_on_seventh_rank) \
    SCALAR(ROOK_OPEN_FILE, ROOK_ON_OPEN_FILE, rook_on_open_file) \
    SCALAR(ROOK_SEMI_OPEN_FILE, ROOK_ON_SEMI_OPEN_FILE, rook_on_semi_open_file) \
    ARRAY(ROOK_PAWN_ADJ, ROOK_PAWN_ADJ, rook_pawn_adj) \
    SCALAR(QUEEN_REL_PIN, QUEEN_REL_PIN, queen_rel_pin) \
    SCALAR(NO_OPPONENT_QUEENS, NO_OPPONENT_QUEENS, no_opponent_queens) \
    SCALAR(KING_OPEN_FILE, KING_ON_OPEN_FILE, king_on_open_file) \
    SCALAR(KING_SEMI_OPEN_FILE, KING_ON_SEMI_OPEN_FILE, king_on_semi_open_file) \
    ARRAY(PAWN_SHIELD, PAWN_SHIELD, pawn_shield) \
    ARRAY(PAWN_STORM, PAWN_STORM, pawn_storm) \
    ARRAY(KING_ZONE_ATTACK, KING_ZONE_ATTACK, king_zone_attack) \
    ARRAY(KING_CASTLED, KING_CASTLED, king_castled) \
    SCALAR(KING_LOST_CASTLE, KING_LOST_ONE_CASTLING_RIGHT, king_lost_one_castling_right) \
    SCALAR(KING_UNCASTLED, KING_UNCASTLED_RIGHTS_REMAIN, king_uncastled_rights_remain) \
    ARRAY(SAFE_CHECK, SAFE_CHECK, safe_check)

// The next enumerator starts immediately after the preceding MG/EG block.
enum TuningOffset : uint16_t {
#define TUNING_SCALAR_OFFSET(name, term, member) \
    name##_OFFSET, name##_LAST = name##_OFFSET + 1,
#define TUNING_ARRAY_OFFSET(name, term, member) \
    name##_OFFSET, name##_LAST = name##_OFFSET + 2 * std::size(term) - 1,
    TUNING_TERMS(TUNING_SCALAR_OFFSET, TUNING_ARRAY_OFFSET)
#undef TUNING_SCALAR_OFFSET
#undef TUNING_ARRAY_OFFSET
    PST_OFFSET,
    TUNING_PARAM_COUNT = PST_OFFSET + 6 * 64 * 2
};

// Minibatching parameters
constexpr static uint16_t BATCH_SIZE = 16384;

struct TunerParam {
    double value = 0.0;
    double grad = 0.0;

    // Adam moment values
    double m = 0.0;
    double v = 0.0;
};

class Tuner {
public:
    Tuner() = delete;
    Tuner(const size_t dataset_size);

    void loadDataset(const std::string& filename, const uint32_t max);
    void run(const uint32_t epochs, const size_t num_threads, const int perturb_amount = 0);
    void dumpParams(std::ofstream& out) const;
private:
    void perturb(const int amount);
    double reconstructScore(const Trace& tr) const;
    void updateGradients(const Trace& tr, double base, double phase, std::vector<double>& local_grads);
    double sigmoid(double score, double k) const { return 1.0 / (1.0 + std::exp(-k * score / 400.0)); }
    double computeError(const std::vector<Trace>& trace_vec) const;
    double computeError(double k) const;
    void computeGradients(const size_t batch_start, const size_t batch_end, const size_t num_threads);
    void findOptimalK();
    void updateAdam(const uint32_t epoch);
    void initParams();

    std::vector<Trace> traces;
    std::vector<Trace> validation_traces;
    std::vector<TunerParam> params;

    size_t adam_step = 0;

    // Adam parameters
    double K = 2.5;
    double LEARNING_RATE = 0.1;
    static constexpr double BETA1 = 0.9;
    static constexpr double BETA2 = 0.999;
    static constexpr double EPSILON = 1e-8;
    static constexpr double WEIGHT_DECAY = 1e-4;
};

#endif // TUNING_H
