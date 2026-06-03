#include "history.h"
#include <cstring>

int score_history[2][64][64]; // [stm][from][to] (butterfly history)

void resetHistory() {
    memset(score_history, 0, sizeof(score_history));
}

void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    auto& h = score_history[stm][From(move)][To(move)];
    h += bonus - (h * std::abs(bonus) / MAX_HISTORY);

    // malus: penalize all quiet moves searched before this cutoff
    for (const Move quiet_move : quiets_tried) {
        if (quiet_move == move) { // We just gave this move a bonus
            continue;
        }

        auto& h = score_history[stm][From(quiet_move)][To(quiet_move)];
        h += -bonus - (h * std::abs(bonus) / MAX_HISTORY);
    }
}

int getScoreHistory(Side stm, Move move) {
    return score_history[stm][From(move)][To(move)];
}

void updateContHistory(int depth, Side stm, Move move, Move prev_move, const MoveList& quiets_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    auto& h = cont_hist[stm][Piece(prev_move)][To(prev_move)][Piece(move)][To(move)];
    h += bonus - (h * std::abs(bonus) / MAX_HISTORY);

    // malus: penalize all quiet moves searched before this cutoff
    for (const Move quiet_move : quiets_tried) {
        if (quiet_move == move) { // We just gave this move a bonus
            continue;
        }
        
        auto& h = cont_hist[stm][Piece(prev_move)][To(prev_move)][Piece(quiet_move)][To(quiet_move)];
        h += -bonus - (h * std::abs(bonus) / MAX_HISTORY);
    }
}