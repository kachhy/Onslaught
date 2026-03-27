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
constexpr uint16_t TEMPO_OFFSET = 12;
constexpr uint16_t MOBILITY_OFFSET = 14;
constexpr uint16_t PAWN_PHALANX_OFFSET = 24;
constexpr uint16_t DOUBLED_PAWNS_OFFSET = 26;
constexpr uint16_t BACKWARDS_PAWN_OFFSET = 28;
constexpr uint16_t PAWN_PROTECTION_OFFSET = 30;
constexpr uint16_t PASSED_PAWNS_OFFSET = 42;
constexpr uint16_t KNIGHT_OUTPOST_OFFSET = 58;
constexpr uint16_t KNIGHT_BEHIND_PAWN_OFFSET = 60;
constexpr uint16_t KNIGHT_PAWN_ADJ_OFFSET = 62;
constexpr uint16_t BISHOP_PAIR_OFFSET = 80;
constexpr uint16_t BISHOP_CTRL_PENALTY_OFFSET = 82;
constexpr uint16_t BAD_BISHOP_OFFSET = 84;
constexpr uint16_t BISHOP_BLOCKING_PAWN_OFFSET = 86;
constexpr uint16_t TRAPPED_BISHOP_OFFSET = 88;
constexpr uint16_t ROOK_SEVENTH_OFFSET = 90;
constexpr uint16_t ROOK_OPEN_FILE_OFFSET = 92;
constexpr uint16_t ROOK_SEMI_OPEN_FILE_OFFSET = 94;
constexpr uint16_t ROOK_PAWN_ADJ_OFFSET = 96;
constexpr uint16_t QUEEN_REL_PIN_OFFSET = 114;
constexpr uint16_t NO_OPPONENT_QUEENS_OFFSET = 116;
constexpr uint16_t KING_OPEN_FILE_OFFSET = 118;
constexpr uint16_t KING_SEMI_OPEN_FILE_OFFSET = 120;
constexpr uint16_t PAWN_SHIELD_OFFSET = 122;
constexpr uint16_t PAWN_STORM_OFFSET = 130;
constexpr uint16_t KING_ZONE_ATTACK_OFFSET = 136;
constexpr uint16_t KING_ZONE_WEAK_SQ_OFFSET = 144;
constexpr uint16_t KING_ZONE_WEAK_EXT_OFFSET = 146;
constexpr uint16_t KING_CASTLED_OFFSET = 148;
constexpr uint16_t KING_LOST_CASTLE_OFFSET = 152;
constexpr uint16_t KING_UNCASTLED_OFFSET = 154;
constexpr uint16_t PST_OFFSET = 156;

struct Position {
    Board board;
    Side result;
    Position(const std::string& fen, const Side result) : board(fen), result(result) { }
};

struct TunerParam {
    double value;
    double grad;

    // Adam moment values
    double m;
    double v;
};

class Tuner {
public:
    Tuner() = delete;
    Tuner(const size_t dataset_size);

    void loadDataset(const std::string& filename, const uint32_t max);
    void run(const uint32_t epochs);
private:
    double reconstructScore(const Trace& tr) const;
    void updateGradients(const Trace& tr, double base, double phase);
    double sigmoid(double score) const { return 1.0 / (1.0 + std::exp(-K * score / 400.0)); }
    double computeError() const;
    void computeGradients();
    void updateAdam(const uint32_t epoch);
    void initParams();

    std::vector<Position> dataset;
    std::vector<Trace> traces;
    std::vector<TunerParam> params;

    // Adam parameters
    static constexpr double K = 2.5;
    static constexpr double BETA1 = 0.9;
    static constexpr double BETA2 = 0.999;
    static constexpr double EPSILON = 1e-8;
    static constexpr double LEARNING_RATE = 0.001;
};

#endif // TUNING_H