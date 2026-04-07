#include "eval.h"
#include "board/rules.h"
#include "movegen/attacks.h"
#include "terms.h"

Trace trace;

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

struct EvalInfo {
    BitBoard pawn_attacks[2];
    BitBoard piece_attacks[64]; // Since there can only ever be one type of piece per square, we can just get their attacks.
};

static BitBoard knight_outpost_table[2][64];
static BitBoard king_zones[2][3][64]; // SIDE -> REG(0) or EXTENDED(1) -> SQUARE
static BitBoard king_critical_files[8];

static inline PieceCounts getPieceCounts(const Board& board) {
    return {
        bitCount(board.getPieceBB(WHITE_PAWN)),   bitCount(board.getPieceBB(BLACK_PAWN)),   bitCount(board.getPieceBB(WHITE_KNIGHT)),
        bitCount(board.getPieceBB(BLACK_KNIGHT)), bitCount(board.getPieceBB(WHITE_BISHOP)), bitCount(board.getPieceBB(BLACK_BISHOP)),
        bitCount(board.getPieceBB(WHITE_ROOK)),   bitCount(board.getPieceBB(BLACK_ROOK)),   bitCount(board.getPieceBB(WHITE_QUEEN)),
        bitCount(board.getPieceBB(BLACK_QUEEN)),
    };
}

static inline EvalInfo precomputeEvalInfo(const Board& board) {
    EvalInfo info;
    info.pawn_attacks[WHITE] = shiftPawnAttacks(board.getPieceBB(WHITE_PAWN), WHITE);
    info.pawn_attacks[BLACK] = shiftPawnAttacks(board.getPieceBB(BLACK_PAWN), BLACK);

    for (uint8_t piece = KNIGHT; piece <= QUEEN; ++piece) {
        const Piece wpc = makePiece(static_cast<DefaultPiece>(piece), WHITE);
        const Piece bpc = makePiece(static_cast<DefaultPiece>(piece), BLACK);
        BitBoard temp = board.getPieceBB(wpc);
        while (temp) {
            const uint8_t sq = popLSB(temp);
            info.piece_attacks[sq] = getPieceAttacks(wpc, static_cast<Square>(sq), board.getOcc(BOTH));
        }
        temp = board.getPieceBB(bpc);
        while (temp) {
            const uint8_t sq = popLSB(temp);
            info.piece_attacks[sq] = getPieceAttacks(bpc, static_cast<Square>(sq), board.getOcc(BOTH));
        }
    }

    return info;
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

    const uint8_t wp_phalanx = bitCount(shiftWest(wp) & wp);
    const uint8_t bp_phalanx = bitCount(shiftWest(bp) & bp);
    score += PAWN_PHALANX * wp_phalanx;
    score -= PAWN_PHALANX * bp_phalanx;
    TRACE_ADD(pawn_phalanx, WHITE, wp_phalanx);
    TRACE_ADD(pawn_phalanx, BLACK, bp_phalanx);

    const uint8_t dpw = std::max(bitCount(A_FILE & wp) - 1, 0) + std::max(bitCount(B_FILE & wp) - 1, 0) + std::max(bitCount(C_FILE & wp) - 1, 0) +
                        std::max(bitCount(D_FILE & wp) - 1, 0) + std::max(bitCount(E_FILE & wp) - 1, 0) + std::max(bitCount(F_FILE & wp) - 1, 0) +
                        std::max(bitCount(G_FILE & wp) - 1, 0) + std::max(bitCount(H_FILE & wp) - 1, 0);
    const uint8_t dpb = std::max(bitCount(A_FILE & bp) - 1, 0) + std::max(bitCount(B_FILE & bp) - 1, 0) + std::max(bitCount(C_FILE & bp) - 1, 0) +
                        std::max(bitCount(D_FILE & bp) - 1, 0) + std::max(bitCount(E_FILE & bp) - 1, 0) + std::max(bitCount(F_FILE & bp) - 1, 0) +
                        std::max(bitCount(G_FILE & bp) - 1, 0) + std::max(bitCount(H_FILE & bp) - 1, 0);

    score += (dpw - dpb) * DOUBLED_PAWNS;
    TRACE_ADD(doubled_pawns, WHITE, dpw);
    TRACE_ADD(doubled_pawns, BLACK, dpb);

    BitBoard wp_protected = info.pawn_attacks[WHITE];
    BitBoard bp_protected = info.pawn_attacks[BLACK];

    for (uint8_t piece = PAWN; piece <= KING; piece++) {
        const uint8_t pprot_white = bitCount(board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), WHITE)) & wp_protected);
        const uint8_t pprot_black = bitCount(board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), BLACK)) & bp_protected);
        score += PAWN_PROTECTION[piece] * pprot_white;
        score -= PAWN_PROTECTION[piece] * pprot_black;
        TRACE_ADD(pawn_protection[piece], WHITE, pprot_white);
        TRACE_ADD(pawn_protection[piece], BLACK, pprot_black);
    }

    // Passed pawns & backwards pawns
    BitBoard temp_wp = wp;
    while (temp_wp) {
        const Square sq = static_cast<Square>(popLSB(temp_wp));
        const int rank = getRank(sq);
        const BitBoard forward_ray = H_FILE >> (63 - sq);
        if (rank >= 1 && rank <= 6 && !(forward_ray & (bp | bp_protected))) {
            score += PASSED_PAWNS[rank - 1];
            TRACE_INC(passed_pawns[rank - 1], WHITE);
        } else if (sq < 56) {
            // Backwards pawns
            // We can re-use knight outpost table because it will cover pawns that can potentially cover us
            BitBoard potential_defenders = board.getPieceBB(WHITE_PAWN) & knight_outpost_table[BLACK][sq];
            const Square forward_sq = static_cast<Square>(static_cast<uint8_t>(sq) - 8);
            if (!potential_defenders && getBit(bp_protected, forward_sq)) {
                score += BACKWARDS_PAWN;
                TRACE_INC(backwards_pawn, WHITE);
            }
        }
    }
    BitBoard temp_bp = bp;
    while (temp_bp) {
        const Square sq = static_cast<Square>(popLSB(temp_bp));
        const int rank = getRank(sq);
        const BitBoard forward_ray = A_FILE << sq;
        if (rank >= 1 && rank <= 6 && !(forward_ray & (wp | wp_protected))) {
            score -= PASSED_PAWNS[6 - rank];
            TRACE_INC(passed_pawns[6 - rank], BLACK);
        } else if (sq >= 8) {
            // Backwards pawns
            // We can re-use knight outpost table because it will cover pawns that can potentially cover us
            BitBoard potential_defenders = board.getPieceBB(BLACK_PAWN) & knight_outpost_table[WHITE][sq];
            const Square forward_sq = static_cast<Square>(static_cast<uint8_t>(sq) + 8);
            if (!potential_defenders && getBit(wp_protected, forward_sq)) {
                score -= BACKWARDS_PAWN;
                TRACE_INC(backwards_pawn, BLACK);
            }
        }
    }

    // Pawn PST
    score += applyPST(board, WHITE_PAWN, BLACK_PAWN);
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
    BitBoard wn_candidates = board.getPieceBB(WHITE_KNIGHT) & WHITE_OUTPOST_RANKS;
    while (wn_candidates) {
        BitBoard potential_attackers = knight_outpost_table[WHITE][popLSB(wn_candidates)] & board.getPieceBB(BLACK_PAWN);
        score += (!potential_attackers) * KNIGHT_OUTPOST;
        TRACE_ADD(knight_outpost, WHITE, (!potential_attackers));
    }
    BitBoard bn_candidates = board.getPieceBB(BLACK_KNIGHT) & BLACK_OUTPOST_RANKS;
    while (bn_candidates) {
        BitBoard potential_attackers = knight_outpost_table[BLACK][popLSB(bn_candidates)] & board.getPieceBB(WHITE_PAWN);
        score -= (!potential_attackers) * KNIGHT_OUTPOST;
        TRACE_ADD(knight_outpost, BLACK, (!potential_attackers));
    }
    const uint8_t wn_behind_pawn = bitCount(shiftNorth(board.getPieceBB(WHITE_KNIGHT)) & board.getPieceBB(WHITE_PAWN));
    const uint8_t bn_behind_pawn = bitCount(shiftSouth(board.getPieceBB(BLACK_KNIGHT)) & board.getPieceBB(BLACK_PAWN));
    score += wn_behind_pawn * KNIGHT_BEHIND_PAWN;
    score -= bn_behind_pawn * KNIGHT_BEHIND_PAWN;
    TRACE_ADD(knight_behind_pawn, WHITE, wn_behind_pawn);
    TRACE_ADD(knight_behind_pawn, BLACK, bn_behind_pawn);

    BitBoard wn = board.getPieceBB(WHITE_KNIGHT);
    while (wn) {
        BitBoard attacks = info.piece_attacks[popLSB(wn)];
        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[KNIGHT] * attack_count;
        TRACE_ADD(mobility[KNIGHT], WHITE, attack_count);
    }
    BitBoard bn = board.getPieceBB(BLACK_KNIGHT);
    while (bn) {
        BitBoard attacks = info.piece_attacks[popLSB(bn)];
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
    BitBoard wp_attacks = info.pawn_attacks[WHITE];
    BitBoard bp_attacks = info.pawn_attacks[BLACK];
    BitBoard wb = board.getPieceBB(WHITE_BISHOP);
    while (wb) {
        BitBoard attacks = info.piece_attacks[popLSB(wb)];
        BitBoard valid_moves = attacks & ~board.getOcc(WHITE);
        BitBoard safe_moves = valid_moves & ~bp_attacks;

        const uint8_t blocking_pawns = bitCount(attacks & board.getPieceBB(WHITE_PAWN));
        score += blocking_pawns * BAD_BISHOP;
        TRACE_ADD(bad_bishop, WHITE, blocking_pawns);

        if (valid_moves && !safe_moves) {
            score += TRAPPED_BISHOP;
            TRACE_INC(trapped_bishop, WHITE);
        }

        attacks &= ~board.getOcc(WHITE);
        const uint8_t attack_count = bitCount(attacks);
        score += MOBILITY[BISHOP] * attack_count;
        TRACE_ADD(mobility[BISHOP], WHITE, attack_count);
    }
    BitBoard bb = board.getPieceBB(BLACK_BISHOP);
    while (bb) {
        BitBoard attacks = info.piece_attacks[popLSB(bb)];
        BitBoard valid_moves = attacks & ~board.getOcc(BLACK);
        BitBoard safe_moves = valid_moves & ~wp_attacks;

        const uint8_t blocking_pawns = bitCount(attacks & board.getPieceBB(BLACK_PAWN));
        score -= blocking_pawns * BAD_BISHOP;
        TRACE_ADD(bad_bishop, BLACK, blocking_pawns);

        if (valid_moves && !safe_moves) {
            score -= TRAPPED_BISHOP;
            TRACE_INC(trapped_bishop, BLACK);
        }

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

        BitBoard attacks = info.piece_attacks[sq];
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

        BitBoard attacks = info.piece_attacks[sq];
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

        BitBoard attacks = info.piece_attacks[sq];
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

        BitBoard attacks = info.piece_attacks[sq];
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
        const uint8_t count = bitCount(info.piece_attacks[popLSB(bb)] & king_zone);
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

    const BitBoard w_king_zone = king_zones[WHITE][0][w_king_sq];
    const BitBoard b_king_zone = king_zones[BLACK][0][b_king_sq];

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

    return score;
}

int eval(const Board& board) {
    if (isMaterialDraw(board)) {
        return 0;
    }

    EvalInfo info = precomputeEvalInfo(board);
    PieceCounts pc = getPieceCounts(board);
    Score score = applyMaterial(pc);
    score += applyAllPST(board);
    score += evaluateKnights(board, info);
    score += evaluateBishops(pc, board, info);
    score += evaluateRooks(board, info);
    score += evaluateQueens(board, info);
    score += evaluatePawnAdjustments(pc);
    score += evaluatePawns(board, info);
    score += kingSafety(pc, board, info);

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? TEMPO : -TEMPO;
    TRACE_INC(tempo, board.getSTM());

    const int r_score = T(score, board.phase());
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

        BitBoard w_ext_king_zone = shiftEast(w_king_zone) | shiftWest(w_king_zone);
        w_ext_king_zone |= shiftNorth(w_ext_king_zone) | shiftSouth(w_ext_king_zone);
        w_ext_king_zone &= ~w_king_zone;

        BitBoard b_king_zone = 1ULL << sq;
        b_king_zone |= shiftEast(b_king_zone) | shiftWest(b_king_zone);
        b_king_zone |= shiftNorth(b_king_zone) | shiftSouth(b_king_zone);
        b_king_zone |= shiftSouth(b_king_zone);

        BitBoard b_ext_king_zone = shiftEast(b_king_zone) | shiftWest(b_king_zone);
        b_ext_king_zone |= shiftNorth(b_ext_king_zone) | shiftSouth(b_ext_king_zone);
        b_ext_king_zone &= ~b_king_zone;

        BitBoard tight_zone = 1ULL << sq;
        tight_zone |= shiftEast(tight_zone) | shiftWest(tight_zone);
        tight_zone |= shiftNorth(tight_zone) | shiftSouth(tight_zone);

        king_zones[WHITE][0][sq] = w_king_zone;
        king_zones[WHITE][1][sq] = w_ext_king_zone;
        king_zones[BLACK][0][sq] = b_king_zone;
        king_zones[BLACK][1][sq] = b_ext_king_zone;
        king_zones[WHITE][2][sq] = tight_zone;
        king_zones[BLACK][2][sq] = tight_zone;
    }
}