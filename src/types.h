#ifndef TYPES_H
#define TYPES_H

enum Square {
    A8 = 0, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
    NO_SQUARE = 64
};

// STM enum
enum Side {
    WHITE = 0,
    BLACK,
    BOTH // For occupancy only
};

// Pieces

enum DefaultPiece {
    PAWN = 0,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

enum Piece {
    WHITE_PAWN = 0,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,
    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,
    NO_PIECE
};

enum TTBound {
    EXACTBOUND = 0,
    LOWERBOUND,
    UPPERBOUND
};

extern const char* board_coords[];
extern int phase_scores[5];

// Type helper functions
Piece makePiece(DefaultPiece piece, Side color);
DefaultPiece makeDefaultPiece(Piece piece);

#endif // TYPES_H