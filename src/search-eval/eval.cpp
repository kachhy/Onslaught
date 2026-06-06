#include "eval.h"
#include "board/rules.h"
#include "movegen/attacks.h"
#include "nnue/nnue.h"
#include "terms.h"
#include <array>

Trace trace;
bool use_nnue = true;

static const std::array<int, 201> fmr_scale = [](){
    std::array<int, 201> t;
    for (int i = 0; i <= 200; i++) {
        t[i] = 200 - i;
    }
    return t;
}();

constexpr size_t TABLE_SIZE_MB = 4;
constexpr size_t TARGET_BYTES = TABLE_SIZE_MB * MEGABYTE;

struct PawnEntry {
    uint64_t hash = 0;
    Score eval;
};

constexpr size_t PAWN_TABLE_SIZE = TARGET_BYTES / sizeof(PawnEntry); // 4 MB
constexpr size_t PAWN_TABLE_MASK = PAWN_TABLE_SIZE - 1;              // 262144 - 1 for 4 MB
alignas(64) static PawnEntry pawn_evals[PAWN_TABLE_SIZE];            // Pawn hash table: 262144 entries, which is 2 ^ 18

struct PieceCounts {
    int wp, bp, wn, bn, wb, bb, wr, br, wq, bq;
};

static BitBoard knight_outpost_table[2][64];
static BitBoard king_zones[2][64]; // SIDE -> SQUARE
static BitBoard king_critical_files[8];

static inline PieceCounts getPieceCounts(const Board& board) {
    return {
        bitCount(board.getPieceBB(WHITE_PAWN)),   bitCount(board.getPieceBB(BLACK_PAWN)),   bitCount(board.getPieceBB(WHITE_KNIGHT)),
        bitCount(board.getPieceBB(BLACK_KNIGHT)), bitCount(board.getPieceBB(WHITE_BISHOP)), bitCount(board.getPieceBB(BLACK_BISHOP)),
        bitCount(board.getPieceBB(WHITE_ROOK)),   bitCount(board.getPieceBB(BLACK_ROOK)),   bitCount(board.getPieceBB(WHITE_QUEEN)),
        bitCount(board.getPieceBB(BLACK_QUEEN)),
    };
}

static inline bool probePawns(const Board& board, Score& score) {
    const uint64_t index = board.pawnHash() & PAWN_TABLE_MASK;
    if (pawn_evals[index].hash == board.pawnHash()) {
        score = pawn_evals[index].eval;
        return true;
    }
    return false;
}

static inline void storePawnEval(const Board& board, const Score score) {
    const uint64_t index = board.pawnHash() & PAWN_TABLE_MASK;
    pawn_evals[index].hash = board.pawnHash();
    pawn_evals[index].eval = score;
}

static inline Score applyPST(const Board& board, const Piece piece_w, const Piece piece_b) {
    Score score{};
    BitBoard white_piece_bb = board.getPieceBB(piece_w);
    BitBoard black_piece_bb = board.getPieceBB(piece_b);

    while (white_piece_bb) {
        const uint8_t sq = popLSB(white_piece_bb);
        score += pst[piece_w][sq];
        TRACE_INC_ONLY(pst[piece_w][sq]);
    }

    while (black_piece_bb) {
        const uint8_t sq = popLSB(black_piece_bb);
        score -= pst[piece_b][sq];
        TRACE_INC_ONLY(pst[piece_b][sq]);
    }

    return score;
}

static inline Score evaluatePawns(const Board& board, const EvalInfo& info) {
    Score score{};

#ifndef TUNING
    if (probePawns(board, score)) {
        return score;
    }
#endif

    BitBoard wp = board.getPieceBB(WHITE_PAWN);
    BitBoard bp = board.getPieceBB(BLACK_PAWN);
    BitBoard wp_protected = info.piece_attacks[WHITE_PAWN];
    BitBoard bp_protected = info.piece_attacks[BLACK_PAWN];

    const uint8_t wp_phalanx = bitCount(shiftWest(wp) & wp);
    const uint8_t bp_phalanx = bitCount(shiftWest(bp) & bp);
    score += PAWN_PHALANX * wp_phalanx;
    score -= PAWN_PHALANX * bp_phalanx;
    TRACE_ADD(pawn_phalanx, WHITE, wp_phalanx);
    TRACE_ADD(pawn_phalanx, BLACK, bp_phalanx);

    BitBoard wp_ff = wp;
    wp_ff |= wp_ff >> 8;
    wp_ff |= wp_ff >> 16;
    wp_ff |= wp_ff >> 32;
    const uint8_t dpw = bitCount(wp) - bitCount(wp_ff & RANK_1);

    BitBoard bp_ff = bp;
    bp_ff |= bp_ff >> 8;
    bp_ff |= bp_ff >> 16;
    bp_ff |= bp_ff >> 32;
    const uint8_t dpb = bitCount(bp) - bitCount(bp_ff & RANK_1);

    score += (dpw - dpb) * DOUBLED_PAWNS;
    TRACE_ADD(doubled_pawns, WHITE, dpw);
    TRACE_ADD(doubled_pawns, BLACK, dpb);

    // Passed pawns & backwards pawns
    BitBoard temp_wp = wp;
    while (temp_wp) {
        const Square sq = static_cast<Square>(popLSB(temp_wp));
        const int rank = getRank(sq);
        const BitBoard forward_ray = H_FILE >> (63 - sq);
        const BitBoard flanks = board.getPieceBB(WHITE_PAWN) & knight_outpost_table[BLACK][sq];

        if (rank >= 1 && rank <= 6 && !(forward_ray & (bp | bp_protected))) {
            score += PASSED_PAWNS[rank - 1];
            TRACE_INC(passed_pawns[rank - 1], WHITE);
        } else if (sq < 56) {
            // Backwards pawns
            // We can re-use knight outpost table because it will cover pawns that can potentially cover us
            const Square forward_sq = static_cast<Square>(static_cast<uint8_t>(sq) - 8);
            if (!flanks && getBit(bp_protected, forward_sq)) {
                score += BACKWARDS_PAWN;
                TRACE_INC(backwards_pawn, WHITE);
            }
        }
        
        // Isolated pawn
        if (!flanks) {
            score += ISOLATED_PAWN;
            TRACE_INC(isolated_pawn, WHITE);
        }
    }

    BitBoard temp_bp = bp;

    while (temp_bp) {
        const Square sq = static_cast<Square>(popLSB(temp_bp));
        const int rank = getRank(sq);
        const BitBoard forward_ray = A_FILE << sq;
        const BitBoard flanks = board.getPieceBB(BLACK_PAWN) & knight_outpost_table[WHITE][sq];

        if (rank >= 1 && rank <= 6 && !(forward_ray & (wp | wp_protected))) {
            score -= PASSED_PAWNS[6 - rank];
            TRACE_INC(passed_pawns[6 - rank], BLACK);
        } else if (sq >= 8) {
            // Backwards pawns
            // We can re-use knight outpost table because it will cover pawns that can potentially cover us
            const Square forward_sq = static_cast<Square>(static_cast<uint8_t>(sq) + 8);
            if (!flanks && getBit(wp_protected, forward_sq)) {
                score -= BACKWARDS_PAWN;
                TRACE_INC(backwards_pawn, BLACK);
            }
        }

        // Isolated pawn
        if (!flanks) {
            score -= ISOLATED_PAWN;
            TRACE_INC(isolated_pawn, BLACK);
        }
    }

    // Pawn PST
#ifdef TUNING
    score += applyPST(board, WHITE_PAWN, BLACK_PAWN);
#endif
#ifndef TUNING
    storePawnEval(board, score);
#endif
    return score;
}

static inline Score applyAllPST(const Board& board) {
    Score score = applyPST(board, WHITE_KNIGHT, BLACK_KNIGHT);
    score += applyPST(board, WHITE_BISHOP, BLACK_BISHOP);
    score += applyPST(board, WHITE_ROOK, BLACK_ROOK);
    score += applyPST(board, WHITE_QUEEN, BLACK_QUEEN);
    score += applyPST(board, WHITE_KING, BLACK_KING);
    return score;
}

static inline Score applyPawnProtection(const Board& board, const EvalInfo& info) {
    Score score{};
    BitBoard wp_protected = info.piece_attacks[WHITE_PAWN];
    BitBoard bp_protected = info.piece_attacks[BLACK_PAWN];

    while (wp_protected) {
        uint8_t sq = popLSB(wp_protected);
        if (getBit(board.getOcc(WHITE), sq)) {
            const Piece pc = board.pieceAt(sq);
            const int dpc = makeDefaultPiece(pc);
            score += PAWN_PROTECTION[dpc];
            TRACE_INC(pawn_protection[dpc], WHITE);
        }
    }

    while (bp_protected) {
        uint8_t sq = popLSB(bp_protected);
        if (getBit(board.getOcc(BLACK), sq)) {
            const Piece pc = board.pieceAt(sq);
            const int dpc = makeDefaultPiece(pc);
            score -= PAWN_PROTECTION[dpc];
            TRACE_INC(pawn_protection[dpc], BLACK);
        }
    }

    return score;
}

static inline Score applyMaterial(const PieceCounts& pc) {
    Score score{};
    score += material_values[PAWN] * (pc.wp - pc.bp);
    score += material_values[KNIGHT] * (pc.wn - pc.bn);
    score += material_values[BISHOP] * (pc.wb - pc.bb);
    score += material_values[ROOK] * (pc.wr - pc.br);
    score += material_values[QUEEN] * (pc.wq - pc.bq);
    TRACE_ADD(material[PAWN], WHITE, pc.wp);
    TRACE_ADD(material[KNIGHT], WHITE, pc.wn);
    TRACE_ADD(material[BISHOP], WHITE, pc.wb);
    TRACE_ADD(material[ROOK], WHITE, pc.wr);
    TRACE_ADD(material[QUEEN], WHITE, pc.wq);
    TRACE_ADD(material[PAWN], BLACK, pc.bp);
    TRACE_ADD(material[KNIGHT], BLACK, pc.bn);
    TRACE_ADD(material[BISHOP], BLACK, pc.bb);
    TRACE_ADD(material[ROOK], BLACK, pc.br);
    TRACE_ADD(material[QUEEN], BLACK, pc.bq);
    return score;
}

static inline Score evaluateKnights(const Board& board, const EvalInfo& info) {
    Score score{};
    const uint8_t wn_behind_pawn = bitCount(shiftNorth(board.getPieceBB(WHITE_KNIGHT)) & board.getPieceBB(WHITE_PAWN));
    const uint8_t bn_behind_pawn = bitCount(shiftSouth(board.getPieceBB(BLACK_KNIGHT)) & board.getPieceBB(BLACK_PAWN));
    score += wn_behind_pawn * KNIGHT_BEHIND_PAWN;
    score -= bn_behind_pawn * KNIGHT_BEHIND_PAWN;
    TRACE_ADD(knight_behind_pawn, WHITE, wn_behind_pawn);
    TRACE_ADD(knight_behind_pawn, BLACK, bn_behind_pawn);

    BitBoard wn = board.getPieceBB(WHITE_KNIGHT);
    while (wn) {
        uint8_t sq = popLSB(wn);
        if (getBit(WHITE_OUTPOST_RANKS, sq)) {
            BitBoard potential_attackers = knight_outpost_table[WHITE][sq] & board.getPieceBB(BLACK_PAWN);
            score += (!potential_attackers) * KNIGHT_OUTPOST;
            TRACE_ADD(knight_outpost, WHITE, (!potential_attackers));
        }
        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[KNIGHT] * attack_count;
        TRACE_ADD(mobility[KNIGHT], WHITE, attack_count);
    }

    BitBoard bn = board.getPieceBB(BLACK_KNIGHT);
    while (bn) {
        uint8_t sq = popLSB(bn);
        if (getBit(BLACK_OUTPOST_RANKS, sq)) {
            BitBoard potential_attackers = knight_outpost_table[BLACK][sq] & board.getPieceBB(WHITE_PAWN);
            score -= (!potential_attackers) * KNIGHT_OUTPOST;
            TRACE_ADD(knight_outpost, BLACK, (!potential_attackers));
        }
        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(BLACK);
        const uint8_t attack_count = bitCount(attacks);
        score -= MOBILITY[KNIGHT] * attack_count;
        TRACE_ADD(mobility[KNIGHT], BLACK, attack_count);
    }

    return score;
}

static inline Score evaluateBishops(const PieceCounts& pc, const Board& board, const EvalInfo& info) {
    Score score{};
    if (pc.wb >= 2) {
        score += BISHOP_PAIR;
        TRACE_INC(bishop_pair, WHITE);
    }

    if (pc.bb >= 2) {
        score -= BISHOP_PAIR;
        TRACE_INC(bishop_pair, BLACK);
    }

    if ((board.getPieceBB(WHITE_BISHOP) & LIGHT_SQUARES) && !(board.getPieceBB(WHITE_BISHOP) & DARK_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(WHITE_PAWN) & LIGHT_SQUARES);
        score += pawns_same_color * BISHOP_CONTROL_PENALTY;
        TRACE_ADD(bishop_control_penalty, WHITE, pawns_same_color);
    } else if ((board.getPieceBB(WHITE_BISHOP) & DARK_SQUARES) && !(board.getPieceBB(WHITE_BISHOP) & LIGHT_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(WHITE_PAWN) & DARK_SQUARES);
        score += pawns_same_color * BISHOP_CONTROL_PENALTY;
        TRACE_ADD(bishop_control_penalty, WHITE, pawns_same_color);
    }
    if ((board.getPieceBB(BLACK_BISHOP) & LIGHT_SQUARES) && !(board.getPieceBB(BLACK_BISHOP) & DARK_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(BLACK_PAWN) & LIGHT_SQUARES);
        score -= pawns_same_color * BISHOP_CONTROL_PENALTY;
        TRACE_ADD(bishop_control_penalty, BLACK, pawns_same_color);
    } else if ((board.getPieceBB(BLACK_BISHOP) & DARK_SQUARES) && !(board.getPieceBB(BLACK_BISHOP) & LIGHT_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(BLACK_PAWN) & DARK_SQUARES);
        score -= pawns_same_color * BISHOP_CONTROL_PENALTY;
        TRACE_ADD(bishop_control_penalty, BLACK, pawns_same_color);
    }

    const uint8_t wb_behind_pawn = bitCount(shiftNorth(board.getPieceBB(WHITE_BISHOP)) & board.getPieceBB(WHITE_PAWN));
    const uint8_t bb_behind_pawn = bitCount(shiftSouth(board.getPieceBB(BLACK_BISHOP)) & board.getPieceBB(BLACK_PAWN));
    score += wb_behind_pawn * BISHOP_BEHIND_PAWN;
    score -= bb_behind_pawn * BISHOP_BEHIND_PAWN;
    TRACE_ADD(bishop_behind_pawn, WHITE, wb_behind_pawn);
    TRACE_ADD(bishop_behind_pawn, BLACK, bb_behind_pawn);

    BitBoard wb = board.getPieceBB(WHITE_BISHOP);
    while (wb) {
        BitBoard attacks = info.square_attacks[popLSB(wb)];

        const uint8_t blocking_pawns = bitCount(attacks & board.getPieceBB(WHITE_PAWN));
        score += blocking_pawns * BAD_BISHOP;
        TRACE_ADD(bad_bishop, WHITE, blocking_pawns);

        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[BISHOP] * attack_count;
        TRACE_ADD(mobility[BISHOP], WHITE, attack_count);
    }

    BitBoard bb = board.getPieceBB(BLACK_BISHOP);
    while (bb) {
        BitBoard attacks = info.square_attacks[popLSB(bb)];

        const uint8_t blocking_pawns = bitCount(attacks & board.getPieceBB(BLACK_PAWN));
        score -= blocking_pawns * BAD_BISHOP;
        TRACE_ADD(bad_bishop, BLACK, blocking_pawns);

        attacks &= ~board.getOcc(BLACK);
        const uint8_t attack_count = bitCount(attacks);
        score -= MOBILITY[BISHOP] * attack_count;
        TRACE_ADD(mobility[BISHOP], BLACK, attack_count);
    }

    const uint8_t white_bishop_blockers = bitCount(shiftSouth(board.getPieceBB(WHITE_BISHOP)) & board.getPieceBB(WHITE_PAWN));
    const uint8_t black_bishop_blockers = bitCount(shiftNorth(board.getPieceBB(BLACK_BISHOP)) & board.getPieceBB(BLACK_PAWN));
    score += white_bishop_blockers * BISHOP_BLOCKING_PAWN;
    score -= black_bishop_blockers * BISHOP_BLOCKING_PAWN;
    TRACE_ADD(bishop_blocking_pawn, WHITE, white_bishop_blockers);
    TRACE_ADD(bishop_blocking_pawn, BLACK, black_bishop_blockers);

    return score;
}

static inline Score evaluateRooks(const Board& board, const EvalInfo& info) {
    Score score{};
    const BitBoard wp = board.getPieceBB(WHITE_PAWN);
    const BitBoard bp = board.getPieceBB(BLACK_PAWN);
    const uint8_t wr_seventh_rank = bitCount(board.getPieceBB(WHITE_ROOK) & RANK_7);
    const uint8_t br_seventh_rank = bitCount(board.getPieceBB(BLACK_ROOK) & RANK_2);
    score += wr_seventh_rank * ROOK_ON_SEVENTH_RANK;
    score -= br_seventh_rank * ROOK_ON_SEVENTH_RANK;
    TRACE_ADD(rook_on_seventh_rank, WHITE, wr_seventh_rank);
    TRACE_ADD(rook_on_seventh_rank, BLACK, br_seventh_rank);
    BitBoard wr = board.getPieceBB(WHITE_ROOK);
    BitBoard br = board.getPieceBB(BLACK_ROOK);

    while (wr) {
        const uint8_t sq = popLSB(wr);
        const BitBoard file_mask = A_FILE << getFile(sq);

        if (!(wp & file_mask)) {
            if (!(bp & file_mask)) {
                score += ROOK_ON_OPEN_FILE;
                TRACE_INC(rook_on_open_file, WHITE);
            } else {
                score += ROOK_ON_SEMI_OPEN_FILE;
                TRACE_INC(rook_on_semi_open_file, WHITE);
            }
        }

        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[ROOK] * attack_count;
        TRACE_ADD(mobility[ROOK], WHITE, attack_count);
    }

    while (br) {
        const uint8_t sq = popLSB(br);
        const BitBoard file_mask = A_FILE << getFile(sq);

        if (!(bp & file_mask)) {
            if (!(wp & file_mask)) {
                score -= ROOK_ON_OPEN_FILE;
                TRACE_INC(rook_on_open_file, BLACK);
            } else {
                score -= ROOK_ON_SEMI_OPEN_FILE;
                TRACE_INC(rook_on_semi_open_file, BLACK);
            }
        }

        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(BLACK);
        const uint8_t attack_count = bitCount(attacks);
        score -= MOBILITY[ROOK] * attack_count;
        TRACE_ADD(mobility[ROOK], BLACK, attack_count);
    }

    return score;
}

static inline Score evaluateQueens(const Board& board, const EvalInfo& info) {
    Score score{};
    BitBoard wq = board.getPieceBB(WHITE_QUEEN);
    BitBoard bq = board.getPieceBB(BLACK_QUEEN);
    while (wq) {
        const Square sq = static_cast<Square>(popLSB(wq));
        if (board.getDiscoveryAttacks(sq, WHITE)) {
            score += QUEEN_REL_PIN;
            TRACE_INC(queen_rel_pin, WHITE);
        }

        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[QUEEN] * attack_count;
        TRACE_ADD(mobility[QUEEN], WHITE, attack_count);
    }
    while (bq) {
        const Square sq = static_cast<Square>(popLSB(bq));
        if (board.getDiscoveryAttacks(sq, BLACK)) {
            score -= QUEEN_REL_PIN;
            TRACE_INC(queen_rel_pin, BLACK);
        }

        BitBoard attacks = info.square_attacks[sq];
        attacks &= ~board.getOcc(BLACK);
        const uint8_t attack_count = bitCount(attacks);
        score -= MOBILITY[QUEEN] * attack_count;
        TRACE_ADD(mobility[QUEEN], BLACK, attack_count);
    }
    return score;
}

static inline Score evaluatePawnAdjustments(const PieceCounts& pc) {
    Score score{};
    score += KNIGHT_PAWN_ADJ[pc.wp] * pc.wn;
    score -= KNIGHT_PAWN_ADJ[pc.bp] * pc.bn;
    score += ROOK_PAWN_ADJ[pc.wp] * pc.wr;
    score -= ROOK_PAWN_ADJ[pc.bp] * pc.br;
    TRACE_ADD(knight_pawn_adj[pc.wp], WHITE, pc.wn);
    TRACE_ADD(knight_pawn_adj[pc.bp], BLACK, pc.bn);
    TRACE_ADD(rook_pawn_adj[pc.wp], WHITE, pc.wr);
    TRACE_ADD(rook_pawn_adj[pc.bp], BLACK, pc.br);
    return score;
}

static inline Score getPieceKingZoneAttacks(const Board& board, const BitBoard king_zone, const Piece piece, const EvalInfo& info, const Side attacked_side) {
    Score score{};
    BitBoard bb = board.getPieceBB(piece);
    const DefaultPiece pc = static_cast<DefaultPiece>(makeDefaultPiece(piece) - 1); // Left by 1 since we start at KNIGHT

    while (bb) {
        const uint8_t count = bitCount(info.square_attacks[popLSB(bb)] & king_zone);
        score += count * KING_ZONE_ATTACK[pc];
        TRACE_ADD(king_zone_attack[pc], attacked_side, count);
    }

    return score;
}

static inline Score kingSafety(const PieceCounts& pc, const Board& board, const EvalInfo& info) {
    Score score{};
    const int w_king_sq = getLSB(board.getPieceBB(WHITE_KING));
    const int b_king_sq = getLSB(board.getPieceBB(BLACK_KING));
    const int w_king_rank = getRank(w_king_sq);
    const int b_king_rank = getRank(b_king_sq);
    const BitBoard wp = board.getPieceBB(WHITE_PAWN);
    const BitBoard bp = board.getPieceBB(BLACK_PAWN);
    score += NO_OPPONENT_QUEENS * !(pc.bq);
    score -= NO_OPPONENT_QUEENS * !(pc.wq);
    TRACE_ADD(no_opponent_queens, WHITE, !(pc.bq));
    TRACE_ADD(no_opponent_queens, BLACK, !(pc.wq));

    if (getBit(G1_H1, w_king_sq)) {
        for (const uint8_t sq : { F2, G2, H2 }) {
            if (getBit(wp, sq)) {
                score += PAWN_SHIELD[1];
                TRACE_INC(pawn_shield[1], WHITE);
            } else if (getBit(wp, sq - 8)) {
                score += PAWN_SHIELD[2];
                TRACE_INC(pawn_shield[2], WHITE);
            } else if (getBit(wp, sq - 16)) {
                score += PAWN_SHIELD[3];
                TRACE_INC(pawn_shield[3], WHITE);
            } else {
                score += PAWN_SHIELD[0];
                TRACE_INC(pawn_shield[0], WHITE);
            }
        }
    } else if (getBit(A1_B1_C1, w_king_sq)) {
        for (const uint8_t sq : { B2, C2, D2 }) {
            if (getBit(wp, sq)) {
                score += PAWN_SHIELD[1];
                TRACE_INC(pawn_shield[1], WHITE);
            } else if (getBit(wp, sq - 8)) {
                score += PAWN_SHIELD[2];
                TRACE_INC(pawn_shield[2], WHITE);
            } else if (getBit(wp, sq - 16)) {
                score += PAWN_SHIELD[3];
                TRACE_INC(pawn_shield[3], WHITE);
            } else {
                score += PAWN_SHIELD[0];
                TRACE_INC(pawn_shield[0], WHITE);
            }
        }
    }

    if (getBit(G8_H8, b_king_sq)) {
        for (const uint8_t sq : { F7, G7, H7 }) {
            if (getBit(bp, sq)) {
                score -= PAWN_SHIELD[1];
                TRACE_INC(pawn_shield[1], BLACK);
            } else if (getBit(bp, sq + 8)) {
                score -= PAWN_SHIELD[2];
                TRACE_INC(pawn_shield[2], BLACK);
            } else if (getBit(bp, sq + 16)) {
                score -= PAWN_SHIELD[3];
                TRACE_INC(pawn_shield[3], BLACK);
            } else {
                score -= PAWN_SHIELD[0];
                TRACE_INC(pawn_shield[0], BLACK);
            }
        }
    } else if (getBit(A8_B8_C8, b_king_sq)) {
        for (const uint8_t sq : { B7, C7, D7 }) {
            if (getBit(bp, sq)) {
                score -= PAWN_SHIELD[1];
                TRACE_INC(pawn_shield[1], BLACK);
            } else if (getBit(bp, sq + 8)) {
                score -= PAWN_SHIELD[2];
                TRACE_INC(pawn_shield[2], BLACK);
            } else if (getBit(bp, sq + 16)) {
                score -= PAWN_SHIELD[3];
                TRACE_INC(pawn_shield[3], BLACK);
            } else {
                score -= PAWN_SHIELD[0];
                TRACE_INC(pawn_shield[0], BLACK);
            }
        }
    }

    BitBoard bp_storm = bp & king_critical_files[getFile(w_king_sq)];
    while (bp_storm) {
        const int dist = w_king_rank - getRank(popLSB(bp_storm));
        if (dist < 3) {
            score += PAWN_STORM[0];
            TRACE_INC(pawn_storm[0], WHITE);
        } else if (dist < 5) {
            score += PAWN_STORM[1];
            TRACE_INC(pawn_storm[1], WHITE);
        } else {
            score += PAWN_STORM[2];
            TRACE_INC(pawn_storm[2], WHITE);
        }
    }

    BitBoard wp_storm = wp & king_critical_files[getFile(b_king_sq)];
    while (wp_storm) {
        const int dist = getRank(popLSB(wp_storm)) - b_king_rank;
        if (dist < 3) {
            score -= PAWN_STORM[0];
            TRACE_INC(pawn_storm[0], BLACK);
        } else if (dist < 5) {
            score -= PAWN_STORM[1];
            TRACE_INC(pawn_storm[1], BLACK);
        } else {
            score -= PAWN_STORM[2];
            TRACE_INC(pawn_storm[2], BLACK);
        }
    }

    const BitBoard w_file_mask = A_FILE << getFile(w_king_sq);
    if (!(wp & w_file_mask)) {
        if (!(bp & w_file_mask)) {
            score += KING_ON_OPEN_FILE;
            TRACE_INC(king_on_open_file, WHITE);
        } else {
            score += KING_ON_SEMI_OPEN_FILE;
            TRACE_INC(king_on_semi_open_file, WHITE);
        }
    }

    const BitBoard b_file_mask = A_FILE << getFile(b_king_sq);
    if (!(bp & b_file_mask)) {
        if (!(wp & b_file_mask)) {
            score -= KING_ON_OPEN_FILE;
            TRACE_INC(king_on_open_file, BLACK);
        } else {
            score -= KING_ON_SEMI_OPEN_FILE;
            TRACE_INC(king_on_semi_open_file, BLACK);
        }
    }

    const BitBoard w_king_zone = king_zones[WHITE][w_king_sq];
    const BitBoard b_king_zone = king_zones[BLACK][b_king_sq];

    if (board.getThreatenedBy(BLACK) & w_king_zone) {
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_KNIGHT, info, WHITE);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_BISHOP, info, WHITE);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_ROOK, info, WHITE);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_QUEEN, info, WHITE);
    }
    if (board.getThreatenedBy(WHITE) & b_king_zone) {
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_KNIGHT, info, BLACK);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_BISHOP, info, BLACK);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_ROOK, info, BLACK);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_QUEEN, info, BLACK);
    }

    CastlingRights cr = board.getCastlingRights();
    const bool w_lost_both_cr = !(cr & (WHITE_KS | WHITE_QS));
    const bool w_lost_one_cr = !w_lost_both_cr && (cr & (WHITE_KS | WHITE_QS)) != (WHITE_KS | WHITE_QS);
    const bool w_king_non_castled_sq = !((1ULL << w_king_sq) & (G1_H1 | A1_B1_C1));

    if (w_lost_both_cr) {
        score += KING_CASTLED[w_king_non_castled_sq];
        TRACE_INC(king_castled[w_king_non_castled_sq], WHITE);
    } else if (w_lost_one_cr) {
        score += KING_LOST_ONE_CASTLING_RIGHT;
        TRACE_INC(king_lost_one_castling_right, WHITE);
    } else if (w_king_non_castled_sq) {
        score += KING_UNCASTLED_RIGHTS_REMAIN;
        TRACE_INC(king_uncastled_rights_remain, WHITE);
    }

    const bool b_lost_both_cr = !(cr & (BLACK_KS | BLACK_QS));
    const bool b_lost_one_cr = !b_lost_both_cr && (cr & (BLACK_KS | BLACK_QS)) != (BLACK_KS | BLACK_QS);
    const bool b_king_non_castled_sq = !((1ULL << b_king_sq) & (G8_H8 | A8_B8_C8));

    if (b_lost_both_cr) {
        score -= KING_CASTLED[b_king_non_castled_sq];
        TRACE_INC(king_castled[b_king_non_castled_sq], BLACK);
    } else if (b_lost_one_cr) {
        score -= KING_LOST_ONE_CASTLING_RIGHT;
        TRACE_INC(king_lost_one_castling_right, BLACK);
    } else if (b_king_non_castled_sq) {
        score -= KING_UNCASTLED_RIGHTS_REMAIN;
        TRACE_INC(king_uncastled_rights_remain, BLACK);
    }

    // Safe checks
    const BitBoard occ = board.getOcc(BOTH);

    // White pieces delivering check to the black king (good for White).
    BitBoard safe_black = ~board.getThreatenedBy(BLACK) | (info.piece_attacks[WHITE_PAWN] & ~info.multi_defended[BLACK]);
    BitBoard w_rook_checks = getPieceAttacks(WHITE_ROOK, static_cast<Square>(b_king_sq), occ) & safe_black;
    BitBoard w_bishop_checks = getPieceAttacks(WHITE_BISHOP, static_cast<Square>(b_king_sq), occ) & safe_black;
    BitBoard w_knight_checks = getPieceAttacks(WHITE_KNIGHT, static_cast<Square>(b_king_sq), occ) & safe_black;
    BitBoard w_queen_checks = getPieceAttacks(WHITE_QUEEN, static_cast<Square>(b_king_sq), occ) & safe_black;

    const int w_knight_count = bitCount(w_knight_checks & info.piece_attacks[WHITE_KNIGHT]);
    const int w_bishop_count = bitCount(w_bishop_checks & info.piece_attacks[WHITE_BISHOP]);
    const int w_rook_count = bitCount(w_rook_checks & info.piece_attacks[WHITE_ROOK]);
    const int w_queen_count = bitCount(w_queen_checks & info.piece_attacks[WHITE_QUEEN]);
    score += SAFE_CHECK[KNIGHT] * w_knight_count;
    score += SAFE_CHECK[BISHOP] * w_bishop_count;
    score += SAFE_CHECK[ROOK] * w_rook_count;
    score += SAFE_CHECK[QUEEN] * w_queen_count;
    TRACE_ADD(safe_check[KNIGHT], WHITE, w_knight_count);
    TRACE_ADD(safe_check[BISHOP], WHITE, w_bishop_count);
    TRACE_ADD(safe_check[ROOK], WHITE, w_rook_count);
    TRACE_ADD(safe_check[QUEEN], WHITE, w_queen_count);

    // Black pieces delivering check to the white king (bad for White).
    BitBoard safe_white = ~board.getThreatenedBy(WHITE) | (info.piece_attacks[BLACK_PAWN] & ~info.multi_defended[WHITE]);
    BitBoard b_rook_checks = getPieceAttacks(BLACK_ROOK, static_cast<Square>(w_king_sq), occ) & safe_white;
    BitBoard b_bishop_checks = getPieceAttacks(BLACK_BISHOP, static_cast<Square>(w_king_sq), occ) & safe_white;
    BitBoard b_knight_checks = getPieceAttacks(BLACK_KNIGHT, static_cast<Square>(w_king_sq), occ) & safe_white;
    BitBoard b_queen_checks = getPieceAttacks(BLACK_QUEEN, static_cast<Square>(w_king_sq), occ) & safe_white;

    const int b_knight_count = bitCount(b_knight_checks & info.piece_attacks[BLACK_KNIGHT]);
    const int b_bishop_count = bitCount(b_bishop_checks & info.piece_attacks[BLACK_BISHOP]);
    const int b_rook_count = bitCount(b_rook_checks & info.piece_attacks[BLACK_ROOK]);
    const int b_queen_count = bitCount(b_queen_checks & info.piece_attacks[BLACK_QUEEN]);
    score -= SAFE_CHECK[KNIGHT] * b_knight_count;
    score -= SAFE_CHECK[BISHOP] * b_bishop_count;
    score -= SAFE_CHECK[ROOK] * b_rook_count;
    score -= SAFE_CHECK[QUEEN] * b_queen_count;
    TRACE_ADD(safe_check[KNIGHT], BLACK, b_knight_count);
    TRACE_ADD(safe_check[BISHOP], BLACK, b_bishop_count);
    TRACE_ADD(safe_check[ROOK], BLACK, b_rook_count);
    TRACE_ADD(safe_check[QUEEN], BLACK, b_queen_count);

    return score;
}

int eval(const Board& board) {
    if (isMaterialDraw(board)) {
        return 0;
    }

    // if (use_nnue) {
    //     int nnue_score = evaluate(board.getAccumulator(), board.getSTM());
    //     nnue_score = nnue_score * fmr_scale[board.getFMR()] / 200;
    //     return nnue_score;
    // }

    EvalInfo info = board.getEvalInfo();
    PieceCounts pc = getPieceCounts(board);
#ifdef TUNING
    Score score = applyMaterial(pc);
    score += applyAllPST(board);
#else
    Score score = board.getMaterialPST();
#endif
    score += evaluateKnights(board, info);
    score += evaluateBishops(pc, board, info); // - 0.26 MNPS
    score += evaluateRooks(board, info);       // - .198 MNPS
    score += evaluateQueens(board, info);      // - 0.284 MNPS
    score += evaluatePawnAdjustments(pc);      // inacc
    score += evaluatePawns(board, info);       // - 0.322 MNPS
    score += applyPawnProtection(board, info);
    score += kingSafety(pc, board, info); // inacc

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? TEMPO : -TEMPO;
    TRACE_INC(tempo, board.getSTM());

    int r_score = T(score, board.phase());
    r_score = r_score * fmr_scale[board.getFMR()] / 200;

    return board.getSTM() == WHITE ? r_score : -r_score;
}

void initEval() {
    for (uint8_t sq = 0; sq < 64; ++sq) {
        const int file = getFile(sq);
        const int rank = getRank(sq);
        BitBoard above = rank > 0 ? (1ULL << (rank * 8)) - 1 : 0ULL;
        BitBoard below = rank < 7 ? (~0ULL << ((rank + 1) * 8)) : 0ULL;
        BitBoard adj_files = 0ULL;

        if (file > 0) {
            adj_files |= (A_FILE << (file - 1));
        }
        if (file < 7) {
            adj_files |= (A_FILE << (file + 1));
        }

        knight_outpost_table[WHITE][sq] = above & adj_files;
        knight_outpost_table[BLACK][sq] = below & adj_files;

        if (sq < 8) {
            adj_files |= (A_FILE << file);
            king_critical_files[sq] = adj_files;
        }

        BitBoard w_king_zone = 1ULL << sq;
        w_king_zone |= shiftEast(w_king_zone) | shiftWest(w_king_zone);
        w_king_zone |= shiftNorth(w_king_zone) | shiftSouth(w_king_zone);
        w_king_zone |= shiftNorth(w_king_zone);

        BitBoard b_king_zone = 1ULL << sq;
        b_king_zone |= shiftEast(b_king_zone) | shiftWest(b_king_zone);
        b_king_zone |= shiftNorth(b_king_zone) | shiftSouth(b_king_zone);
        b_king_zone |= shiftSouth(b_king_zone);

        king_zones[WHITE][sq] = w_king_zone;
        king_zones[BLACK][sq] = b_king_zone;
    }
}