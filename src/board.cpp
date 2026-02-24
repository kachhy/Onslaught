#include "board.h"

Board::Board() {
    clear();

    history.reserve(MAX_PLY);

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

Piece Board::pieceAt(uint8_t sq) const {
    if (!getBit(occ[BOTH], sq))
        return NO_PIECE;

    for (uint8_t p = WHITE_PAWN; p <= BLACK_KING; p++) {
        if (getBit(piece_bb[p], sq)) {
            return p;
        }
    }

    return NO_PIECE; // Exceptional state
}

void Board::printBoard() const {
    for (int8_t i = 63; i >= 0; i--) {
        switch (pieceAt(i)) {
        case WHITE_PAWN:
            std::cout << 'P';
            break;
        case WHITE_KNIGHT:
            std::cout << 'N';
            break;
        case WHITE_BISHOP:
            std::cout << 'B';
            break;
        case WHITE_ROOK:
            std::cout << 'R';
            break;
        case WHITE_QUEEN:
            std::cout << 'Q';
            break;
        case WHITE_KING:
            std::cout << 'K';
            break;
        case BLACK_PAWN:
            std::cout << 'p';
            break;
        case BLACK_KNIGHT:
            std::cout << 'n';
            break;
        case BLACK_BISHOP:
            std::cout << 'b';
            break;
        case BLACK_ROOK:
            std::cout << 'r';
            break;
        case BLACK_QUEEN:
            std::cout << 'q';
            break;
        case BLACK_KING:
            std::cout << 'k';
            break;
        case NO_PIECE:
            std::cout << '.';
            break;
        }

        if (i % 8 == 0)
            std::cout << '\n';
    }
    std::cout << std::endl;
}

void Board::clear() {
    memset(piece_bb, 0, sizeof(piece_bb));
    memset(occ, 0, sizeof(occ));
    history.clear();

    // Turns
    stm  = WHITE;
    xstm = BLACK;

    // Counters
    move_number = 1;
    fmr         = 0;

    // EP squares and Castling rights
    ep_square = NO_SQUARE;
    castling  = 0xF;
}