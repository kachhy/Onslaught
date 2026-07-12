#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "movegen/movegen.h"
#include "history.h"
#include <array>

enum MovePickerType {
    REGULAR = 0,
    QSEARCH
};

enum MovePickerStage {
    TT_MOVE = 0,
    INIT_CAPTURES,
    GOOD_CAPTURES,
    KILLER1,
    KILLER2,
    BAD_CAPTURES,
    INIT_QUIET,
    QUIET_MOVES,
    END
};

class MovePicker {
private:
    const Board& board;
    MovePickerType type;
    SearchStack* ss;
    MovePickerStage stage;

    MoveList loud_moves;
    MoveList quiet_moves;

    Move tt_move = NO_MOVE;
    Move killer1 = NO_MOVE;
    Move killer2 = NO_MOVE;

    uint16_t loud_moves_start = 0;
    uint16_t quiet_moves_start = 0;

    std::array<int, 255> loud_scores;
    std::array<int, 255> quiet_scores;

    int scoreQuietMove(Move move);
    int scoreLoudMove(Move move);
public:
    MovePicker(const Board& board, MovePickerType type, Move tt_move, Move killer1, Move killer2, SearchStack* ss) :
        board(board), type(type), ss(ss), tt_move(tt_move), killer1(killer1), killer2(killer2) {
        stage = (tt_move == NO_MOVE) ? INIT_CAPTURES : TT_MOVE;
    }

    void initCaptures();
    void initQuiets();
    Move nextMove();
};

#endif // MOVEPICKER_H