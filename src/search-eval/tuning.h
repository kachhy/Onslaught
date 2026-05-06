#ifndef TUNING_H
#define TUNING_H

#include "core/types.h"
#include "search-eval/eval.h"
#include "search-eval/terms.h"
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

constexpr uint16_t MATERIAL_OFFSET = 0;
constexpr uint16_t TEMPO_OFFSET = MATERIAL_OFFSET + 12;
constexpr uint16_t MOBILITY_OFFSET = TEMPO_OFFSET + 2;
constexpr uint16_t PAWN_PHALANX_OFFSET = MOBILITY_OFFSET + 10;
constexpr uint16_t DOUBLED_PAWNS_OFFSET = PAWN_PHALANX_OFFSET + 2;
constexpr uint16_t BACKWARDS_PAWN_OFFSET = DOUBLED_PAWNS_OFFSET + 2;
constexpr uint16_t PAWN_PROTECTION_OFFSET = BACKWARDS_PAWN_OFFSET + 2;
constexpr uint16_t PASSED_PAWNS_OFFSET = PAWN_PROTECTION_OFFSET + 12;
constexpr uint16_t KNIGHT_OUTPOST_OFFSET = PASSED_PAWNS_OFFSET + 16;
constexpr uint16_t KNIGHT_BEHIND_PAWN_OFFSET = KNIGHT_OUTPOST_OFFSET + 2;
constexpr uint16_t KNIGHT_PAWN_ADJ_OFFSET = KNIGHT_BEHIND_PAWN_OFFSET + 2;
constexpr uint16_t BISHOP_PAIR_OFFSET = KNIGHT_PAWN_ADJ_OFFSET + 18;
constexpr uint16_t BISHOP_CTRL_PENALTY_OFFSET = BISHOP_PAIR_OFFSET + 2;
constexpr uint16_t BAD_BISHOP_OFFSET = BISHOP_CTRL_PENALTY_OFFSET + 2;
constexpr uint16_t BISHOP_BLOCKING_PAWN_OFFSET = BAD_BISHOP_OFFSET + 2;
constexpr uint16_t BISHOP_BEHIND_PAWN_OFFSET = BISHOP_BLOCKING_PAWN_OFFSET + 2;
constexpr uint16_t ROOK_SEVENTH_OFFSET = BISHOP_BEHIND_PAWN_OFFSET + 2;
constexpr uint16_t ROOK_OPEN_FILE_OFFSET = ROOK_SEVENTH_OFFSET + 2;
constexpr uint16_t ROOK_SEMI_OPEN_FILE_OFFSET = ROOK_OPEN_FILE_OFFSET + 2;
constexpr uint16_t ROOK_PAWN_ADJ_OFFSET = ROOK_SEMI_OPEN_FILE_OFFSET + 2;
constexpr uint16_t QUEEN_REL_PIN_OFFSET = ROOK_PAWN_ADJ_OFFSET + 18;
constexpr uint16_t NO_OPPONENT_QUEENS_OFFSET = QUEEN_REL_PIN_OFFSET + 2;
constexpr uint16_t KING_OPEN_FILE_OFFSET = NO_OPPONENT_QUEENS_OFFSET + 2;
constexpr uint16_t KING_SEMI_OPEN_FILE_OFFSET = KING_OPEN_FILE_OFFSET + 2;
constexpr uint16_t PAWN_SHIELD_OFFSET = KING_SEMI_OPEN_FILE_OFFSET + 2;
constexpr uint16_t PAWN_STORM_OFFSET = PAWN_SHIELD_OFFSET + 8;
constexpr uint16_t KING_ZONE_ATTACK_OFFSET = PAWN_STORM_OFFSET + 6;
constexpr uint16_t KING_CASTLED_OFFSET = KING_ZONE_ATTACK_OFFSET + 8;
constexpr uint16_t KING_LOST_CASTLE_OFFSET = KING_CASTLED_OFFSET + 4;
constexpr uint16_t KING_UNCASTLED_OFFSET = KING_LOST_CASTLE_OFFSET + 2;
constexpr uint16_t PST_OFFSET = KING_UNCASTLED_OFFSET + 2;

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
    void run(const uint32_t epochs, const size_t num_threads);
    void dumpParams(std::ofstream& out) const;
private:
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