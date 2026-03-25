#include "eval.h"
#include "movegen/attacks.h"
#include "terms.h"
#include "board/rules.h"
#include <iostream>

constexpr size_t TABLE_SIZE_MB = 4;
constexpr size_t TARGET_BYTES = TABLE_SIZE_MB * MEGABYTE;

struct PawnEntry {
    uint64_t hash = 0;
    Score eval;
};

constexpr size_t PAWN_TABLE_SIZE = TARGET_BYTES / sizeof(PawnEntry); // 4 MB
constexpr size_t PAWN_TABLE_MASK = PAWN_TABLE_SIZE - 1; // 262144 - 1 for 4 MB
alignas(64) static PawnEntry pawn_evals[PAWN_TABLE_SIZE]; // Pawn hash table: 262144 entries, which is 2 ^ 18

struct PieceCounts {
    int wp, bp, wn, bn, wb, bb, wr, br, wq, bq;
};

static BitBoard knight_outpost_table[2][64];
static BitBoard king_critical_files[8];

static inline PieceCounts getPieceCounts(const Board& board) {
    return {
        bitCount(board.getPieceBB(WHITE_PAWN)),
        bitCount(board.getPieceBB(BLACK_PAWN)),
        bitCount(board.getPieceBB(WHITE_KNIGHT)),
        bitCount(board.getPieceBB(BLACK_KNIGHT)),
        bitCount(board.getPieceBB(WHITE_BISHOP)),
        bitCount(board.getPieceBB(BLACK_BISHOP)),
        bitCount(board.getPieceBB(WHITE_ROOK)),
        bitCount(board.getPieceBB(BLACK_ROOK)),
        bitCount(board.getPieceBB(WHITE_QUEEN)),
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

static inline Score applyPST(const Board& board, const DefaultPiece piece) {
    Score score{};
    BitBoard white_piece_bb = board.getPieceBB(makePiece(piece, WHITE));
    BitBoard black_piece_bb = board.getPieceBB(makePiece(piece, BLACK));
    while (white_piece_bb) {
        score += pst[piece][popLSB(white_piece_bb)];
    }
    while (black_piece_bb) {
        score -= pst[piece][flipRank(popLSB(black_piece_bb))];
    }
    return score;
}

static inline Score evaluatePawns(const Board& board) {
    Score score{};
    if (probePawns(board, score)) {
        return score;
    }
    
    BitBoard wp = board.getPieceBB(WHITE_PAWN);
    BitBoard bp = board.getPieceBB(BLACK_PAWN);

    score += PAWN_PHALANX * bitCount(shiftWest(wp) & wp);
    score -= PAWN_PHALANX * bitCount(shiftWest(bp) & bp);

    const int dp = std::max(bitCount(A_FILE & wp) - 1, 0)
                 + std::max(bitCount(B_FILE & wp) - 1, 0)
                 + std::max(bitCount(C_FILE & wp) - 1, 0)
                 + std::max(bitCount(D_FILE & wp) - 1, 0)
                 + std::max(bitCount(E_FILE & wp) - 1, 0)
                 + std::max(bitCount(F_FILE & wp) - 1, 0)
                 + std::max(bitCount(G_FILE & wp) - 1, 0)
                 + std::max(bitCount(H_FILE & wp) - 1, 0)
                 - std::max(bitCount(A_FILE & bp) - 1, 0)
                 - std::max(bitCount(B_FILE & bp) - 1, 0)
                 - std::max(bitCount(C_FILE & bp) - 1, 0)
                 - std::max(bitCount(D_FILE & bp) - 1, 0)
                 - std::max(bitCount(E_FILE & bp) - 1, 0)
                 - std::max(bitCount(F_FILE & bp) - 1, 0)
                 - std::max(bitCount(G_FILE & bp) - 1, 0)
                 - std::max(bitCount(H_FILE & bp) - 1, 0);

    score += dp * DOUBLED_PAWNS;

    BitBoard wp_protected = shiftPawnAttacks(wp, WHITE);
    BitBoard bp_protected = shiftPawnAttacks(bp, BLACK);

    for (uint8_t piece = PAWN; piece <= KING; piece++) {
        score += PAWN_PROTECTION[piece] * bitCount(board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), WHITE)) & wp_protected);
        score -= PAWN_PROTECTION[piece] * bitCount(board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), BLACK)) & bp_protected);
    }

    // Passed pawns
    BitBoard temp_wp = wp; 
    while (temp_wp) {
        const Square sq = static_cast<Square>(popLSB(temp_wp));
        const int rank = getRank(sq);
        if (rank > 2) { 
            const BitBoard forward_ray = H_FILE >> (63 - sq);
            if (!(forward_ray & (bp | bp_protected))) {
                score += PASSED_PAWNS[rank - 3];
            }
        }
    }
    BitBoard temp_bp = bp;
    while (temp_bp) {
        const Square sq = static_cast<Square>(popLSB(temp_bp));
        const int rank = getRank(sq);
        if (rank < 5) {
            const BitBoard forward_ray = A_FILE << sq;
            if (!(forward_ray & (wp | wp_protected))) {
                score -= PASSED_PAWNS[4 - rank]; 
            }
        }
    }

    // Pawn PST
    score += applyPST(board, PAWN);

    storePawnEval(board, score);
    return score;
}

static inline Score applyMobility(const Board& board) {
    Score score{};
    const BitBoard occ = board.getOcc(BOTH);
    const BitBoard white_pieces = board.getOcc(WHITE);
    const BitBoard black_pieces = board.getOcc(BLACK);
    for (uint8_t piece = KNIGHT; piece <= QUEEN; piece++) {
        const Piece wpc = makePiece(static_cast<DefaultPiece>(piece), WHITE);
        const Piece bpc = makePiece(static_cast<DefaultPiece>(piece), BLACK);
        BitBoard temp = board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), WHITE));
        while (temp) {
            Square sq = static_cast<Square>(popLSB(temp));
            BitBoard attacks = getPieceAttacks(wpc, sq, occ);
            attacks &= ~white_pieces; 
            score += MOBILITY[piece] * bitCount(attacks); 
        }
        temp = board.getPieceBB(makePiece(static_cast<DefaultPiece>(piece), BLACK));
        while (temp) {
            Square sq = static_cast<Square>(popLSB(temp));
            BitBoard attacks = getPieceAttacks(bpc, sq, occ);
            attacks &= ~black_pieces;
            score -= MOBILITY[piece] * bitCount(attacks);
        }
    }
    return score;
}

static inline Score applyAllPST(const Board& board) {
    Score score = applyPST(board, KNIGHT);
    score += applyPST(board, BISHOP);
    score += applyPST(board, ROOK);
    score += applyPST(board, QUEEN);
    score += applyPST(board, KING);
    return score;
}

static inline Score applyMaterial(const PieceCounts& pc) {
    Score score{};
    score += material_values[PAWN] * (pc.wp - pc.bp);
    score += material_values[KNIGHT] * (pc.wn - pc.bn);
    score += material_values[BISHOP] * (pc.wb - pc.bb);
    score += material_values[ROOK] * (pc.wr - pc.br);
    score += material_values[QUEEN] * (pc.wq - pc.bq);
    return score;
}

static inline Score evaluateKnights(const Board& board) {
    Score score{};
    BitBoard wn_candidates = board.getPieceBB(WHITE_KNIGHT) & WHITE_OUTPOST_RANKS;
    while (wn_candidates) {
        BitBoard potential_attackers = knight_outpost_table[WHITE][popLSB(wn_candidates)] & board.getPieceBB(BLACK_PAWN);
        score += (!potential_attackers) * KNIGHT_OUTPOST;
    }
    BitBoard bn_candidates = board.getPieceBB(BLACK_KNIGHT) & BLACK_OUTPOST_RANKS;
    while (bn_candidates) {
        BitBoard potential_attackers = knight_outpost_table[BLACK][popLSB(bn_candidates)] & board.getPieceBB(WHITE_PAWN);
        score -= (!potential_attackers) * KNIGHT_OUTPOST;
    }
    score += bitCount(shiftNorth(board.getPieceBB(WHITE_KNIGHT)) & board.getPieceBB(WHITE_PAWN)) * KNIGHT_BEHIND_PAWN;
    score -= bitCount(shiftSouth(board.getPieceBB(BLACK_KNIGHT)) & board.getPieceBB(BLACK_PAWN)) * KNIGHT_BEHIND_PAWN;
    return score;
}

static inline Score evaluateBishops(const PieceCounts& pc, const Board& board) {
    Score score{};
    if (pc.wb >= 2) {
        score += BISHOP_PAIR;
    }
    if (pc.bb >= 2) {
        score -= BISHOP_PAIR;
    }
    if ((board.getPieceBB(WHITE_BISHOP) & LIGHT_SQUARES) && !(board.getPieceBB(WHITE_BISHOP) & DARK_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(WHITE_PAWN) & LIGHT_SQUARES);
        score += pawns_same_color * BISHOP_CONTROL_PENALTY;
    }
    else if ((board.getPieceBB(WHITE_BISHOP) & DARK_SQUARES) && !(board.getPieceBB(WHITE_BISHOP) & LIGHT_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(WHITE_PAWN) & DARK_SQUARES);
        score += pawns_same_color * BISHOP_CONTROL_PENALTY;
    }
    if ((board.getPieceBB(BLACK_BISHOP) & LIGHT_SQUARES) && !(board.getPieceBB(BLACK_BISHOP) & DARK_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(BLACK_PAWN) & LIGHT_SQUARES);
        score -= pawns_same_color * BISHOP_CONTROL_PENALTY;
    }
    else if ((board.getPieceBB(BLACK_BISHOP) & DARK_SQUARES) && !(board.getPieceBB(BLACK_BISHOP) & LIGHT_SQUARES)) {
        const int pawns_same_color = bitCount(board.getPieceBB(BLACK_PAWN) & DARK_SQUARES);
        score -= pawns_same_color * BISHOP_CONTROL_PENALTY;
    }
    BitBoard bp_attacks = shiftPawnAttacks(board.getPieceBB(BLACK_PAWN), BLACK); 
    BitBoard wp_attacks = shiftPawnAttacks(board.getPieceBB(WHITE_PAWN), WHITE);
    BitBoard wb = board.getPieceBB(WHITE_BISHOP);
    while (wb) {
        const Square sq = static_cast<Square>(popLSB(wb));
        BitBoard attacks = getPieceAttacks(WHITE_BISHOP, sq, board.getOcc(BOTH));
        BitBoard valid_moves = attacks & ~board.getOcc(WHITE);
        BitBoard safe_moves = valid_moves & ~bp_attacks;

        if (!safe_moves) {
            score += TRAPPED_BISHOP; 
        } else {
            int blocking_pawns = bitCount(attacks & board.getPieceBB(WHITE_PAWN));
            score += blocking_pawns * BAD_BISHOP;
        }
    }
    BitBoard bb = board.getPieceBB(BLACK_BISHOP);
    while (bb) {
        const Square sq = static_cast<Square>(popLSB(bb));
        BitBoard attacks = getPieceAttacks(BLACK_BISHOP, sq, board.getOcc(BOTH));
        BitBoard valid_moves = attacks & ~board.getOcc(BLACK);
        BitBoard safe_moves = valid_moves & ~wp_attacks;
        if (!safe_moves) {
            score -= TRAPPED_BISHOP;
        } else {
            int blocking_pawns = bitCount(attacks & board.getPieceBB(BLACK_PAWN));
            score -= blocking_pawns * BAD_BISHOP;
        }
    }
    score += bitCount(shiftSouth(board.getPieceBB(WHITE_BISHOP)) & board.getPieceBB(WHITE_PAWN)) * BISHOP_BLOCKING_PAWN;
    score -= bitCount(shiftNorth(board.getPieceBB(BLACK_BISHOP)) & board.getPieceBB(BLACK_PAWN)) * BISHOP_BLOCKING_PAWN;
    return score;
}

static inline Score evaluateRooks(const Board& board) {
    Score score{};
    const BitBoard wp = board.getPieceBB(WHITE_PAWN);
    const BitBoard bp = board.getPieceBB(BLACK_PAWN);
    score += bitCount(board.getPieceBB(WHITE_ROOK) & RANK_7) * ROOK_ON_SEVENTH_RANK;
    score -= bitCount(board.getPieceBB(BLACK_ROOK) & RANK_2) * ROOK_ON_SEVENTH_RANK;
    BitBoard wr = board.getPieceBB(WHITE_ROOK);
    BitBoard br = board.getPieceBB(BLACK_ROOK);
    while (wr) {
        const BitBoard file_mask = A_FILE << getFile(popLSB(wr));
        if (!(wp & file_mask)) {
            score += !(bp & file_mask) ? ROOK_ON_OPEN_FILE : ROOK_ON_SEMI_OPEN_FILE;
        }
    }
    while (br) {
        const BitBoard file_mask = A_FILE << getFile(popLSB(br));
        if (!(bp & file_mask)) {
            score -= !(wp & file_mask) ? ROOK_ON_OPEN_FILE : ROOK_ON_SEMI_OPEN_FILE;
        }
    }
    return score;
}

static inline Score evaluateQueens(const Board& board) {
    Score score{};
    BitBoard wq = board.getPieceBB(WHITE_QUEEN);
    BitBoard bq = board.getPieceBB(BLACK_QUEEN);
    while (wq) {
        if (board.getDiscoveryAttacks(static_cast<Square>(popLSB(wq)), WHITE)) {
            score += QUEEN_REL_PIN;
        }
    }
    while (bq) {
        if (board.getDiscoveryAttacks(static_cast<Square>(popLSB(bq)), BLACK)) {
            score -= QUEEN_REL_PIN;
        }
    }
    return score;
}

static inline Score evaluatePawnAdjustments(const PieceCounts& pc) {
    Score score{};
    score += KNIGHT_PAWN_ADJ[pc.wp] * pc.wn;
    score -= KNIGHT_PAWN_ADJ[pc.bp] * pc.bn;
    score += ROOK_PAWN_ADJ[pc.wp] * pc.wr;
    score -= ROOK_PAWN_ADJ[pc.bp] * pc.br;
    return score;
}

static inline int zoneWeakSquares(const Board& board, const BitBoard w_king_zone, const BitBoard b_king_zone) {
    int count = 0;
    BitBoard w_defended = 0ULL;
    BitBoard b_defended = 0ULL;
    for (uint8_t pc = PAWN; pc < KING; pc++) {
        const Piece wpc = makePiece(static_cast<DefaultPiece>(pc), WHITE);
        const Piece bpc = makePiece(static_cast<DefaultPiece>(pc), BLACK);
        BitBoard wbb = board.getPieceBB(wpc);
        BitBoard bbb = board.getPieceBB(bpc);
        while (wbb) {
            w_defended |= getPieceAttacks(wpc, static_cast<Square>(popLSB(wbb)), board.getOcc(BOTH));
        }
        while (bbb) {
            b_defended |= getPieceAttacks(bpc, static_cast<Square>(popLSB(bbb)), board.getOcc(BOTH));
        }
    }
    count += bitCount(w_king_zone & ~w_defended);
    count -= bitCount(b_king_zone & ~b_defended);
    return count;
}

static inline Score getPieceKingZoneAttacks(const Board& board, const BitBoard king_zone, const Piece piece) {
    Score score{};
    BitBoard bb = board.getPieceBB(piece);
    while (bb) {
        score += bitCount(getPieceAttacks(piece, static_cast<Square>(popLSB(bb)), board.getOcc(BOTH)) & king_zone) * KING_ZONE_ATTACK[makeDefaultPiece(piece)];
    }
    return score;
}

static inline Score kingSafety(const PieceCounts& pc, const Board& board) {
    Score score{};
    const int w_king_sq = getLSB(board.getPieceBB(WHITE_KING));
    const int b_king_sq = getLSB(board.getPieceBB(BLACK_KING));
    const int w_king_rank = getRank(w_king_sq);
    const int b_king_rank = getRank(b_king_sq);
    const BitBoard wp = board.getPieceBB(WHITE_PAWN);
    const BitBoard bp = board.getPieceBB(BLACK_PAWN);
    score += NO_OPPONENT_QUEENS * !(pc.bq);
    score -= NO_OPPONENT_QUEENS * !(pc.wq);
    if (getBit(G1_H1, w_king_sq)) {
        for (const uint8_t sq : {F2, G2, H2}) {
            if (getBit(wp, sq)) {
                score += PAWN_SHIELD[1];
            }
            else if (getBit(wp, sq - 8)) {
                score += PAWN_SHIELD[2];
            }
            else if (getBit(wp, sq - 16)) {
                score += PAWN_SHIELD[3];
            }
            else {
                score += PAWN_SHIELD[0];
            }
        }
    }
    else if (getBit(A1_B1_C1, w_king_sq)) {
        for (const uint8_t sq : {A2, B2, C2, D2}) {
            if (getBit(wp, sq)) {
                score += PAWN_SHIELD[1];
            }
            else if (getBit(wp, sq - 8)) {
                score += PAWN_SHIELD[2];
            }
            else if (getBit(wp, sq - 16)) {
                score += PAWN_SHIELD[3];
            }
            else {
                score += PAWN_SHIELD[0];
            }
        }
    }
    if (getBit(G8_H8, b_king_sq)) {
        for (const uint8_t sq : {F7, G7, H7}) {
            if (getBit(bp, sq)) {
                score -= PAWN_SHIELD[1];
            }
            else if (getBit(bp, sq + 8)) {
                score -= PAWN_SHIELD[2];
            }
            else if (getBit(bp, sq + 16)) {
                score -= PAWN_SHIELD[3];
            }
            else {
                score -= PAWN_SHIELD[0];
            }
        }
    }
    else if (getBit(A8_B8_C8, b_king_sq)) {
        for (const uint8_t sq : {A7, B7, C7, D7}) {
            if (getBit(bp, sq)) {
                score -= PAWN_SHIELD[1];
            }
            else if (getBit(bp, sq + 8)) {
                score -= PAWN_SHIELD[2];
            }
            else if (getBit(bp, sq + 16)) {
                score -= PAWN_SHIELD[3];
            }
            else {
                score -= PAWN_SHIELD[0];
            }
        }
    }

    BitBoard bp_storm = bp & king_critical_files[getFile(w_king_sq)];
    while (bp_storm) {
        const int dist = getRank(popLSB(bp_storm)) - w_king_rank;
        if (dist < 5) {
            score += PAWN_STORM[0];
        }
        else if (dist < 6) {
            score += PAWN_STORM[1];
        }
        else {
            score += PAWN_STORM[2];
        }
    }
    BitBoard wp_storm = wp & king_critical_files[getFile(b_king_sq)];
    while (wp_storm) {
        const int dist = b_king_rank - getRank(popLSB(wp_storm));
        if (dist < 5) {
            score -= PAWN_STORM[0];
        }
        else if (dist < 6) {
            score -= PAWN_STORM[1];
        }
        else {
            score -= PAWN_STORM[2];
        }
    }
    const BitBoard w_file_mask = A_FILE << getFile(w_king_sq);
    if (!(wp & w_file_mask)) {
        score += !(bp & w_file_mask) ? KING_ON_OPEN_FILE : KING_ON_SEMI_OPEN_FILE;
    }
    const BitBoard b_file_mask = A_FILE << getFile(b_king_sq);
    if (!(bp & b_file_mask)) {
        score -= !(wp & b_file_mask) ? KING_ON_OPEN_FILE : KING_ON_SEMI_OPEN_FILE;
    }

    BitBoard w_king_zone = 1ULL << w_king_sq;
    w_king_zone |= shiftEast(w_king_zone) | shiftWest(w_king_zone);
    w_king_zone |= shiftNorth(w_king_zone) | shiftSouth(w_king_zone);
    w_king_zone |= shiftNorth(w_king_zone);
    
    BitBoard w_ext_king_zone = shiftEast(w_king_zone) | shiftWest(w_king_zone);
    w_ext_king_zone |= shiftNorth(w_ext_king_zone) | shiftSouth(w_ext_king_zone);
    w_ext_king_zone &= ~w_king_zone;

    BitBoard b_king_zone = 1ULL << b_king_sq;
    b_king_zone |= shiftEast(b_king_zone) | shiftWest(b_king_zone);
    b_king_zone |= shiftNorth(b_king_zone) | shiftSouth(b_king_zone);
    b_king_zone |= shiftSouth(b_king_zone);

    BitBoard b_ext_king_zone = shiftEast(b_king_zone) | shiftWest(b_king_zone);
    b_ext_king_zone |= shiftNorth(b_ext_king_zone) | shiftSouth(b_ext_king_zone);
    b_ext_king_zone &= ~b_king_zone;

    if (board.getThreatenedBy(BLACK) & w_king_zone) {
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_KNIGHT);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_BISHOP);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_ROOK);
        score += getPieceKingZoneAttacks(board, w_king_zone, BLACK_QUEEN);
    }
    if (board.getThreatenedBy(WHITE) & b_king_zone) {
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_KNIGHT);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_BISHOP);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_ROOK);
        score -= getPieceKingZoneAttacks(board, b_king_zone, WHITE_QUEEN);
    }

    score += zoneWeakSquares(board, w_king_zone, b_king_zone) * KING_ZONE_WEAK_SQUARE;
    score += zoneWeakSquares(board, w_ext_king_zone, b_ext_king_zone) * KING_ZONE_WEAK_SQUARE_EXTENDED;
    
    CastlingRights cr = board.getCastlingRights();
    const bool w_lost_both_cr = !(cr & (WHITE_KS | WHITE_QS));
    const bool w_lost_one_cr = !w_lost_both_cr && (cr & (WHITE_KS | WHITE_QS)) != (WHITE_KS | WHITE_QS);
    const bool w_king_non_castled_sq = !((1ULL << w_king_sq) & (G1 | H1 | A1_B1_C1));

    if (w_lost_both_cr) {
        score += KING_CASTLED[w_king_non_castled_sq];
    } else if (w_lost_one_cr) {
        score += KING_LOST_ONE_CASTLING_RIGHT;
    } else if (w_king_non_castled_sq) {
        score += KING_UNCASTLED_RIGHTS_REMAIN;
    }

    const bool b_lost_both_cr = !(cr & (BLACK_KS | BLACK_QS));
    const bool b_lost_one_cr = !b_lost_both_cr && (cr & (BLACK_KS | BLACK_QS)) != (BLACK_KS | BLACK_QS);
    const bool b_king_non_castled_sq = !((1ULL << b_king_sq) & (G8 | H8 | A8_B8_C8));

    if (b_lost_both_cr) {
        score -= KING_CASTLED[b_king_non_castled_sq];
    } else if (b_lost_one_cr) {
        score -= KING_LOST_ONE_CASTLING_RIGHT;
    } else if (b_king_non_castled_sq) {
        score -= KING_UNCASTLED_RIGHTS_REMAIN;
    }

    return score;
}

int eval(const Board& board) {
    // if (isMaterialDraw(board))
    //     return 0;

    PieceCounts pc = getPieceCounts(board);
    Score score = applyMaterial(pc);
    score += applyAllPST(board);
    score += applyMobility(board);
    score += evaluateKnights(board);
    score += evaluateBishops(pc, board);
    score += evaluateRooks(board);
    score += evaluateQueens(board);
    score += evaluatePawnAdjustments(pc);
    score += evaluatePawns(board);
    score += kingSafety(pc, board);

    // std::cout << "Subscores:\nMaterial:" << T(applyMaterial(pc), 24)
    //           << "\nPST: " << T(applyAllPST(board), 24)
    //           << "\nMob: " << T(applyMobility(board), 24)
    //           << "\nKnights: " << T(evaluateKnights(board), 24)
    //           << "\nBishops: " << T(evaluateBishops(pc, board), 24)
    //           << "\nRooks: " << T(evaluateRooks(board), 24)
    //           << "\nQueens: " << T(evaluateQueens(board), 24)
    //           << "\nPawn Adj: " << T(evaluatePawnAdjustments(pc), 24)
    //           << "\nPawns: " << T(evaluatePawns(board), 24)
    //           << "\nKing safety: " << T(kingSafety(pc, board), 24) << std::endl;

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? TEMPO : -TEMPO;

    int r_score = T(score, board.phase());

    return board.getSTM() == WHITE ? r_score : -r_score;
}

int eval_perspective(const Board& board) {
    int base_score = eval(board);
    return board.getSTM() == BLACK ? -base_score : base_score;
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
    }
}