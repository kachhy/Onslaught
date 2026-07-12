#ifndef SEE_H
#define SEE_H

#include "board/board.h"
#include "movegen/attacks.h"

constexpr int SEE_VALUES[6] = { 100, 300, 300, 500, 900, 20000 }; // PBNRQK

// SEE
inline int staticExchangeEval(const Board& board, Move move, int threshold) {
    if (Castle(move) || IsEP(move) || Prom(move)) {
        return 1;
    }

    Square from = From(move);
    Square to = To(move);

    int v = SEE_VALUES[makeDefaultPiece(board.pieceAt(to))] - threshold;
    if (v < 0) {
        return 0;
    }

    v = SEE_VALUES[makeDefaultPiece(MovePiece(move))] - v;
    if (v <= 0) {
        return 1;
    }

    int stm = board.getSTM();

    BitBoard occ = board.getOcc(BOTH) ^ (BitBoard(1) << from) ^ (BitBoard(1) << to);
    BitBoard attackers = getAttackers(board, to, occ);
    BitBoard mine, lowest_attacker;

    const BitBoard diag = board.getPieceBB(WHITE_BISHOP) | board.getPieceBB(BLACK_BISHOP) | board.getPieceBB(WHITE_QUEEN) | board.getPieceBB(BLACK_QUEEN);
    const BitBoard straight = board.getPieceBB(WHITE_ROOK) | board.getPieceBB(BLACK_ROOK) | board.getPieceBB(WHITE_QUEEN) | board.getPieceBB(BLACK_QUEEN);

    int result = 1;

    while (true) {
        stm ^= 1;
        attackers &= occ;

        if (!(mine = (attackers & board.getOcc(static_cast<Side>(stm))))) {
            break;
        }

        result ^= 1;

        int pt;
        for (pt = PAWN; pt <= QUEEN; pt++) {
            lowest_attacker = mine & board.getPieceBB(makePiece(static_cast<DefaultPiece>(pt), static_cast<Side>(stm)));
            if (lowest_attacker) {
                break;
            }
        }

        if (pt > QUEEN) {
            return (attackers & ~board.getOcc(static_cast<Side>(stm))) ? result ^ 1 : result;
        }

        if ((v = SEE_VALUES[pt] - v) < result) {
            break;
        }

        occ ^= (lowest_attacker & -lowest_attacker);
        if (pt == PAWN || pt == BISHOP || pt == QUEEN) {
            attackers |= getBishopAttacks(to, occ) & diag;
        }
        if (pt == ROOK || pt == QUEEN) {
            attackers |= getRookAttacks(to, occ) & straight;
        }
    }

    return result;
}


#endif // SEE_H