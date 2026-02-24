#include "board.h"

Board::Board() {
    history.reserve(MAX_PLY);

    // Clear all bitboards
    for (uint8_t p = WHITE_PAWN; p <= BLACK_KING; p++)
        piece_bb[p] = 0ULL;

    // Pawns
    piece_bb[WHITE_PAWN] = 0x000000000000FF00ULL;
    piece_bb[BLACK_PAWN] = 0x00FF000000000000ULL;

    // White Knights
    setBit(piece_bb[WHITE_KNIGHT], B1);
    setBit(piece_bb[WHITE_KNIGHT], G1);

    // White Bishops
    setBit(piece_bb[WHITE_BISHOP], C1);
    setBit(piece_bb[WHITE_BISHOP], F1);

    // White Rooks
    setBit(piece_bb[WHITE_ROOK], A1);
    setBit(piece_bb[WHITE_ROOK], H1);

    // White Royals
    setBit(piece_bb[WHITE_QUEEN], D1);
    setBit(piece_bb[WHITE_KING],  E1);

    // Black Knights
    setBit(piece_bb[BLACK_KNIGHT], B8);
    setBit(piece_bb[BLACK_KNIGHT], G8);

    // Black Bishops
    setBit(piece_bb[BLACK_BISHOP], C8);
    setBit(piece_bb[BLACK_BISHOP], F8);

    // Black Rooks
    setBit(piece_bb[BLACK_ROOK], A8);
    setBit(piece_bb[BLACK_ROOK], H8);

    // Black Royals
    setBit(piece_bb[BLACK_QUEEN], D8);
    setBit(piece_bb[BLACK_KING],  E8);

    // EP squares and Castling rights
    ep_square = NO_SQUARE;
    castling  = 0xF;

    // Counters
    move_number = 1;
    fmr         = 0;

    setOcc();
}

void Board::setOcc() {
    // Set white occupancy
    for (uint8_t p = WHITE_PAWN; p <= WHITE_KING; p++) {
        occ[WHITE] |= piece_bb[p];
        occ[BOTH] |= piece_bb[p];
    }

    // Set black occupancy
    for (uint8_t p = BLACK_PAWN; p <= BLACK_KING; p++) {
        occ[BLACK] |= piece_bb[p];
        occ[BOTH] |= piece_bb[p];
    }
}