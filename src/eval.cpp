#include "eval.h"
#include "terms.h"

constexpr int16_t EVAL_UNKNOWN = 32001;
constexpr size_t TABLE_SIZE_MB = 4;
constexpr size_t TARGET_BYTES = TABLE_SIZE_MB * MEGABYTE;

struct PawnEntry {
    uint64_t hash = 0;
    Score eval;
};

constexpr size_t PAWN_TABLE_SIZE = TARGET_BYTES / sizeof(PawnEntry); // 4 MB
constexpr size_t PAWN_TABLE_MASK = PAWN_TABLE_SIZE - 1; // 262144 - 1 for 4 MB
alignas(64) PawnEntry pawn_evals[PAWN_TABLE_SIZE]; // Pawn hash table: 262144 entries, which is 2 ^ 18

static inline Score probePawns(const Board& board) {
    const uint64_t index = board.pawnHash() & PAWN_TABLE_MASK;
    if (pawn_evals[index].hash == board.pawnHash()) {
        return pawn_evals[index].eval;
    }
    return S(EVAL_UNKNOWN, EVAL_UNKNOWN);
}

static inline void storePawnEval(const Board& board, const Score score) {
    const uint64_t index = board.pawnHash() & PAWN_TABLE_MASK;
    pawn_evals[index].hash = board.pawnHash();
    pawn_evals[index].eval = score;
} 

static inline Score evaluatePawns(const Board& board) {
    Score score = S(0, 0);
    if ((score = probePawns(board)) != S(EVAL_UNKNOWN, EVAL_UNKNOWN)) {
        return score;
    }
    else {
        score = S(0, 0);
    }
    storePawnEval(board, score);
    return score;
}

static inline int applyPST(const Board& board, const DefaultPiece piece) {
    int score = 0;
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

static inline int applyAllPST(const Board& board) {
    int score = applyPST(board, PAWN);
    score += applyPST(board, KNIGHT);
    score += applyPST(board, BISHOP);
    score += applyPST(board, ROOK);
    score += applyPST(board, QUEEN);
    score += applyPST(board, KING);
    return score;
}

static inline Score applyMaterial(const Board& board) {
    Score score = material_values[PAWN] * bitCount(board.getPieceBB(WHITE_PAWN));
    score += material_values[KNIGHT] * bitCount(board.getPieceBB(WHITE_KNIGHT));
    score += material_values[BISHOP] * bitCount(board.getPieceBB(WHITE_BISHOP));
    score += material_values[ROOK] * bitCount(board.getPieceBB(WHITE_ROOK));
    score += material_values[QUEEN] * bitCount(board.getPieceBB(WHITE_QUEEN));

    score += material_values[PAWN] * -1 * bitCount(board.getPieceBB(BLACK_PAWN));
    score += material_values[KNIGHT] * -1 * bitCount(board.getPieceBB(BLACK_KNIGHT));
    score += material_values[BISHOP] * -1 * bitCount(board.getPieceBB(BLACK_BISHOP));
    score += material_values[ROOK] * -1 * bitCount(board.getPieceBB(BLACK_ROOK));
    score += material_values[QUEEN] * -1 * bitCount(board.getPieceBB(BLACK_QUEEN));
    
    return score;
}

static inline Score evaluateBishopPair(const Board& board) {
    Score score = S(0, 0);
    if (bitCount(board.getPieceBB(WHITE_BISHOP)) >= 2) {
        score += BISHOP_PAIR;
    }
    if (bitCount(board.getPieceBB(BLACK_BISHOP)) >= 2) {
        score -= BISHOP_PAIR;
    }
    return score;
}

static inline Score evaluatePawnAdjustments(const Board& board) {
    Score score = S(0, 0);
    const int wp = bitCount(board.getPieceBB(WHITE_PAWN));
    const int bp = bitCount(board.getPieceBB(BLACK_PAWN));
    const int wn = bitCount(board.getPieceBB(WHITE_KNIGHT));
    const int bn = bitCount(board.getPieceBB(BLACK_KNIGHT));
    const int wr = bitCount(board.getPieceBB(WHITE_ROOK));
    const int br = bitCount(board.getPieceBB(BLACK_ROOK));
    score += KNIGHT_PAWN_ADJ[wp] * wn;
    score -= KNIGHT_PAWN_ADJ[bp] * bn;
    score += ROOK_PAWN_ADJ[wp] * wr;
    score -= ROOK_PAWN_ADJ[bp] * br;
    return score;
}

int eval(const Board& board) {
    Score score = applyMaterial(board);
    // score += applyAllPST(board);
    score += evaluateBishopPair(board);
    score += evaluatePawnAdjustments(board);
    score += evaluatePawns(board);

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? TEMPO : -TEMPO;

    int r_score = T(score, board.phase());

    return board.getSTM() == WHITE ? r_score : -r_score;
}