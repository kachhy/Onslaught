#include "eval.h"
#include "terms.h"

constexpr int16_t EVAL_UNKNOWN = 32001;
constexpr size_t TABLE_SIZE_MB = 4;
constexpr size_t TARGET_BYTES = TABLE_SIZE_MB * MEGABYTE;

struct PawnEntry {
    uint64_t hash = 0;
    int16_t eval;
};

constexpr size_t PAWN_TABLE_SIZE = TARGET_BYTES / sizeof(PawnEntry); // 4 MB
constexpr size_t PAWN_TABLE_MASK = PAWN_TABLE_SIZE - 1; // 262144 - 1 for 4 MB
PawnEntry pawn_evals[PAWN_TABLE_SIZE]; // Pawn hash table: 262144 entries, which is 2 ^ 18

static inline int probePawns(const Board& board) {
    const uint64_t index = board.pawnHash() & PAWN_TABLE_MASK;
    if (pawn_evals[index].hash == board.pawnHash()) {
        return pawn_evals[index].eval;
    }
    return EVAL_UNKNOWN;
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

static inline int applyMaterial(const Board& board) {
    const int p = board.phase();
    int score = T(material_values[PAWN], p) * bitCount(board.getPieceBB(WHITE_PAWN));
    score += T(material_values[KNIGHT], p) * bitCount(board.getPieceBB(WHITE_KNIGHT));
    score += T(material_values[BISHOP], p) * bitCount(board.getPieceBB(WHITE_BISHOP));
    score += T(material_values[ROOK], p) * bitCount(board.getPieceBB(WHITE_ROOK));
    score += T(material_values[QUEEN], p) * bitCount(board.getPieceBB(WHITE_QUEEN));

    score += T(material_values[PAWN], p) * -1 * bitCount(board.getPieceBB(BLACK_PAWN));
    score += T(material_values[KNIGHT], p) * -1 * bitCount(board.getPieceBB(BLACK_KNIGHT));
    score += T(material_values[BISHOP], p) * -1 * bitCount(board.getPieceBB(BLACK_BISHOP));
    score += T(material_values[ROOK], p) * -1 * bitCount(board.getPieceBB(BLACK_ROOK));
    score += T(material_values[QUEEN], p) * -1 * bitCount(board.getPieceBB(BLACK_QUEEN));
    
    return score;
}

static inline int evaluateBishopPair(const Board& board) {
    const int p = board.phase();
    int score = 0;
    if (bitCount(board.getPieceBB(WHITE_BISHOP)) >= 2) {
        score += T(BISHOP_PAIR, p);
    }
    if (bitCount(board.getPieceBB(BLACK_BISHOP)) >= 2) {
        score -= T(BISHOP_PAIR, p);
    }
    return score;
}

static inline int evaluatePawnAdjustments(const Board& board) {
    const int p = board.phase();
    int score = 0;
    const int wp = bitCount(board.getPieceBB(WHITE_PAWN));
    const int bp = bitCount(board.getPieceBB(BLACK_PAWN));
    const int wn = bitCount(board.getPieceBB(WHITE_KNIGHT));
    const int bn = bitCount(board.getPieceBB(BLACK_KNIGHT));
    const int wr = bitCount(board.getPieceBB(WHITE_ROOK));
    const int br = bitCount(board.getPieceBB(BLACK_ROOK));
    score += T(KNIGHT_PAWN_ADJ[wp], p) * wn;
    score -= T(KNIGHT_PAWN_ADJ[bp], p) * bn;
    score -= T(ROOK_PAWN_ADJ[wp], p) * wr;
    score -= T(ROOK_PAWN_ADJ[bp], p) * br;
    return score;
}

int eval(const Board& board) {
    int score = applyMaterial(board);
    score += applyAllPST(board);
    score += evaluateBishopPair(board);
    score += evaluatePawnAdjustments(board);

    int p = board.phase();

    // Tempo bonus
    score += (board.getSTM() == WHITE) ? T(TEMPO, p) : -T(TEMPO, p);

    return board.getSTM() == WHITE ? score : -score;
}