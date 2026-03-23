#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board/board.h"
#include "core/movelist.h"

enum MoveFlag {
    QUIET_MOVE_MOVEGEN = 0b01,
    NOISY_MOVE_MOVEGEN = 0b10,
    LEGAL_MOVE_MOVEGEN = QUIET_MOVE_MOVEGEN | NOISY_MOVE_MOVEGEN,
};

constexpr inline BitBoard getDoublePushRank(Side side) { return side == WHITE ? RANK_4 : RANK_5; }
constexpr inline BitBoard getAttackingEPRank(Side side) { return side == WHITE ? RANK_5 : RANK_4; }

MoveList getQuietMoves(const Board& board);
MoveList getNoisyMoves(const Board& board);
MoveList getLegalMoves(const Board& board);

void addLegalPawnMoves(MoveList& moves, const Board& board, MoveFlag move_flag);
void addEPLegalMoves(MoveList& moves, const Board& board);
void addPieceLegalMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag);
void addLegalKingMoves(MoveList& moves, const Board& board, MoveFlag move_flag);

void addAllLegalPieceMoves(MoveList& moves, const Board& board);
void addAllNoisyPieceMoves(MoveList& moves, const Board& board);
void addAllQuietPieceMoves(MoveList& moves, const Board& board);

#endif // MOVEGEN_H