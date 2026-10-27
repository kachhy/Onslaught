#include "accumulator.h"
#include "uci/uci.h"
#include "board/board.h"

void Accumulator::applyDelta(const Accumulator& prev) {
    accumulator_dirty = false;
    if (move == NO_MOVE) {
        seedFrom(prev); // null move, so the position is unchanged
        return;
    }

    const DefaultPiece moved_dp = makeDefaultPiece(MovePiece(move));
    const Side us = stm;
    const Side them = static_cast<Side>(us ^ 1);
    const Square from = From(move);
    Square to = To(move);
    if (c960 && Castle(move)) {
        to = castle_destination_to_actual(move, stm);
    }

    if (moved_dp == KING) { // same bucket + mirror only
        king_sq[us] = static_cast<uint8_t>(to);
    }

    if (Castle(move)) {
        Square rook_from = NO_SQUARE, rook_to = NO_SQUARE;
        switch (to) {
        case G1:
            rook_from = castling.rstart[0];
            rook_to = F1;
            break;
        case C1:
            rook_from = castling.rstart[1];
            rook_to = D1;
            break;
        case G8:
            rook_from = castling.rstart[2];
            rook_to = F8;
            break;
        case C8:
            rook_from = castling.rstart[3];
            rook_to = D8;
            break;
        default: seedFrom(prev); return;
        }

        addAddSubSub(prev, KING, us, to, ROOK, us, rook_to, KING, us, from, ROOK, us, rook_from);
        return;
    }

    if (Prom(move)) {
        sub(prev, PAWN, us, from);
        add(*this, promPiece(move), us, to);
        if (Capture(move)) {
            sub(*this, makeDefaultPiece(captured), them, to);
        }

        return;
    }

    if (IsEP(move)) {
        const Square cap_sq = static_cast<Square>(static_cast<int>(to) - (us == WHITE ? NORTH : SOUTH));
        sub(prev, PAWN, us, from);
        add(*this, PAWN, us, to);
        sub(*this, PAWN, them, cap_sq);
        return;
    }

    if (Capture(move)) {
        addSubSub(prev, moved_dp, us, from, makeDefaultPiece(captured), to);
        return;
    }

    addSub(prev, moved_dp, us, from, to);
    return;
}