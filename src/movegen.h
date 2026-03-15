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
constexpr inline BitBoard getAttackingEPRank(Side side) { return side == WHITE ? RANK_5 : RANK_4; }

MoveList getQuietMoves(const Board& board);
MoveList getNoisyMoves(const Board& board);
MoveList addPieceLegalMoves(const Board &board);
MoveList getLegalMoves(const Board &board);

void addLegalPawnMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag);
void addPieceLegalMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag);
void addEPLegalMoves(MoveList& moves, const Board& board);
void addLegalKingMoves(MoveList& moves, const Board& board);

#endif // MOVEGEN_H