#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"
#include <vector>

using MoveList = std::vector<Move>;

enum MoveFlag {
  QUIET_MOVE_MOVEGEN = 0b01,
  NOISY_MOVE_MOVEGEN = 0b10,
  LEGAL_MOVE_MOVEGEN = QUIET_MOVE_MOVEGEN | NOISY_MOVE_MOVEGEN,
};

constexpr inline BitBoard getPromoRank(Side side) { return side == WHITE ? RANK_7 : RANK_2; }
constexpr inline BitBoard getDoublePushRank(Side side) { return side == WHITE ? RANK_4 : RANK_5; }

MoveList getQuietMoves(const Board& board);
MoveList getNoisyMoves(const Board& board);
MoveList getPseudoLegalMoves(const Board &board);
MoveList getLegalMoves(const Board &board);

void getMovesPawn(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag);
void getMovesPiece(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag);

#endif // MOVEGEN_H