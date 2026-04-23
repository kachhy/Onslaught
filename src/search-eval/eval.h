#ifndef EVAL_H
#define EVAL_H

#include "board/board.h"

constexpr int SCORE_MAX = 32000;

// TODO: Move these to consteval for C++20
#ifdef TUNING
  #define TRACE_ADD(term, side, val) (trace.term[side] += (val))
  #define TRACE_INC(term, side)      (trace.term[side]++)
  #define TRACE_INC_ONLY(term)      (trace.term++)
#else
  #define TRACE_ADD(term, side, val)
  #define TRACE_INC(term, side)
  #define TRACE_INC_ONLY(term)
#endif

struct Trace {
    // Material
    int16_t material[6][2];                      // [piece][side] //

    // Tempo
    int16_t tempo[2];                            // [side] //
    
    // Mobility (indexed by piece 0-4, then mobility count 0-4)
    int16_t mobility[5][2];                      // [mobility_index][side] //

    int16_t uncontested_central_control[2];                            // [side] //

    // Pawn structure
    int16_t pawn_phalanx[2];                     // [side] //
    int16_t doubled_pawns[2];                    // [side] //
    int16_t backwards_pawn[2];                   // [side] //
    int16_t isolated_pawn[2];                    // [side] //
    int16_t pawn_protection[6][2];               // [piece_being_protected][side] //
    int16_t passed_pawns[8][2];                  // [rank][side] //

    // Knight
    int16_t knight_outpost[2];                   // [side] //
    int16_t knight_behind_pawn[2];               // [side] //
    int16_t knight_pawn_adj[9][2];               // [pawn_count][side] (count of knights on board) //

    // Bishop
    int16_t bishop_pair[2];                      // [side] //
    int16_t bishop_control_penalty[2];           // [side] (count of penalized squares) //
    int16_t bad_bishop[2];                       // [side] (count of blocking pawns) //
    int16_t bishop_blocking_pawn[2];             // [side] //
    int16_t bishop_behind_pawn[2];             // [side] //
    int16_t trapped_bishop[2];                   // [side] //

    // Rook
    int16_t rook_on_seventh_rank[2];             // [side] //
    int16_t rook_on_open_file[2];                // [side] //
    int16_t rook_on_semi_open_file[2];           // [side] //
    int16_t rook_pawn_adj[9][2];                 // [pawn_count][side] //

    // Queen
    int16_t queen_rel_pin[2];                    // [side] //
    int16_t no_opponent_queens[2];               // [side] //

    // King safety - file exposure
    int16_t king_on_open_file[2];                // [side] //
    int16_t king_on_semi_open_file[2];           // [side] //

    // King safety - pawn shield/storm (counts per bucket)
    int16_t pawn_shield[4][2];                   // [shield_bucket][side] //
    int16_t pawn_storm[3][2];                    // [storm_bucket][side] //

    // King safety - zone attacks (counts per attacker type)
    int16_t king_zone_attack[4][2];              // [piece][side] //
    int16_t king_zone_weak_square[2];            // [side] //
    int16_t king_zone_weak_square_extended[2];   // [side] //

    // King castling state
    int16_t king_castled[2][2];                  // [castled_bucket][side] //
    int16_t king_lost_one_castling_right[2];     // [side] //
    int16_t king_uncastled_rights_remain[2];     // [side] //

    // PST
    int16_t pst[12][64];                         // [piece*2+side][square] //

    // Phase (for tapering only)
    int32_t phase;

    // Result
    double result;                                // 1.0, 0.0, 0.5
};

extern Trace trace;

int eval(const Board& position);
void initEval();

#endif // EVAL_H