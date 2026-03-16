#include "eval.h"

using Score = int32_t;

constexpr Score S(const int16_t mg, const int16_t eg) { return (static_cast<Score>(mg) << 16) + static_cast<Score>(eg); }
constexpr int16_t MG(const Score score) { return static_cast<int16_t>(score >> 16); }
constexpr int16_t EG(const Score score) { return static_cast<int16_t>(score); }
constexpr Score T(const Score score, const int phase) { return (MG(score) * phase + EG(score) * (MAX_PHASE - phase)) / MAX_PHASE; } // taper

// Evaluation constants

Score material_values[6] = {
    S(82, 94), // PAWN
    S(337, 281), // KNIGHT
    S(365, 297), // BISHOP
    S(477, 512), // ROOK
    S(1025, 936), // QUEEN
    S(0, 0), // KING (shouldn't hit this case ever)
};

constexpr Score TEMPO = S(26, 0);
constexpr Score BISHOP_PAIR = S(29, 84);
const Score KNIGHT_PAWN_ADJ[9] = {
    S(-20, -20), // 0 pawns
    S(-16, -16),
    S(-12, -12),
    S( -4,  -4),
    S(  0,   0), // 4 pawns 
    S(  4,   4),
    S(  8,   8),
    S( 12,  12),
    S( 16,  16), // 8 pawns
};
const Score ROOK_PAWN_ADJ[9] = {
    S( 20,  30),
    S( 15,  22),
    S( 10,  15),
    S(  5,   8),
    S(  0,   0),
    S( -5,  -8),
    S(-10, -15),
    S(-15, -22),
    S(-20, -30),
};

constexpr int pst[7][64] = {
    // pawn
    {
        0,  0,  0,  0,  0,  0,  0,  0, // Rank 1
        5, 10, 10,-20,-20, 10, 10,  5, // Rank 2
        5, -5,-10,  0,  0,-10, -5,  5, // Rank 3
        0,  0,  0, 20, 20,  0,  0,  0, // Rank 4
        5,  5, 10, 25, 25, 10,  5,  5, // Rank 5
        10, 10, 20, 30, 30, 20, 10, 10, // Rank 6
        50, 50, 50, 50, 50, 50, 50, 50, // Rank 7
        0,  0,  0,  0,  0,  0,  0,  0  // Rank 8
    },
    // knight
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    },
    // bishop
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    },
    // rook
    {
        0,  0,  0,  5,  5,  0,  0,  0,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        5, 10, 10, 10, 10, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0
    },
    // queen
    {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
        0,  0,  5,  5,  5,  5,  0, -5,
        -5,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    },
    // king_mg
    {
        20, 30, 10,  0,  0, 10, 30, 20,
        20, 20,  0,  0,  0,  0, 20, 20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30
    },
    // king_eg
    {
        -50,-30,-30,-30,-30,-30,-30,-50,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50
    }
};

static int applyPST(const Board& board, const DefaultPiece piece) {
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

static int applyAllPST(const Board& board) {
    int score = applyPST(board, PAWN);
    score += applyPST(board, KNIGHT);
    score += applyPST(board, BISHOP);
    score += applyPST(board, ROOK);
    score += applyPST(board, QUEEN);
    score += applyPST(board, KING);
    return score;
}

static int applyMaterial(const Board& board) {
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

static int evaluateBishopPair(const Board& board) {
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

static int evaluatePawnAdjustments(const Board& board) {
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