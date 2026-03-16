#include "eval.h"
#include "terms.h"

constexpr size_t TABLE_SIZE_MB = 4;
constexpr size_t TARGET_BYTES = TABLE_SIZE_MB * MEGABYTE;

struct PawnEntry {
    uint64_t hash = 0;
    Score eval;
};

constexpr size_t PAWN_TABLE_SIZE = TARGET_BYTES / sizeof(PawnEntry); // 4 MB
constexpr size_t PAWN_TABLE_MASK = PAWN_TABLE_SIZE - 1; // 262144 - 1 for 4 MB
alignas(64) PawnEntry pawn_evals[PAWN_TABLE_SIZE]; // Pawn hash table: 262144 entries, which is 2 ^ 18

struct PieceCounts {
    int wp, bp, wn, bn, wb, bb, wr, br, wq, bq;
};

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

static inline Score evaluatePawns(const Board& board) {
    Score score{};
    if (probePawns(board, score)) {
        return score;
    }
    storePawnEval(board, score);
    return score;
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

static inline Score applyAllPST(const Board& board) {
    Score score = applyPST(board, PAWN);
    score += applyPST(board, KNIGHT);
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

static inline Score evaluateBishopPair(const PieceCounts& pc) {
    Score score{};
    if (pc.wb >= 2) {
        score += BISHOP_PAIR;
    }
    if (pc.bb >= 2) {
        score -= BISHOP_PAIR;
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

int eval(const Board& board) {
    PieceCounts pc = getPieceCounts(board);
    Score score = applyMaterial(pc);
    score += applyAllPST(board);
    score += evaluateBishopPair(pc);
    score += evaluatePawnAdjustments(pc);
    score += evaluatePawns(board);

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? TEMPO : -TEMPO;

    int r_score = T(score, board.phase());

    return board.getSTM() == WHITE ? r_score : -r_score;
}