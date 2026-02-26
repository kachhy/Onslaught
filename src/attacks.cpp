#include "attacks.h"

BitBoard rook_masks[64];
BitBoard bishop_masks[64];

BitBoard king_attacks[64];
BitBoard knight_attacks[64];
BitBoard bishop_attacks[64][4096];
BitBoard rook_attacks[64][4096];

const int bishop_relevant_bits[64] = {6, 5, 5, 5, 5, 5, 5, 6,
                                      5, 5, 5, 5, 5, 5, 5, 5,
                                      5, 5, 7, 7, 7, 7, 5, 5,
                                      5, 5, 7, 9, 9, 7, 5, 5,
                                      5, 5, 7, 9, 9, 7, 5, 5,
                                      5, 5, 7, 7, 7, 7, 5, 5,
                                      5, 5, 5, 5, 5, 5, 5, 5,
                                      6, 5, 5, 5, 5, 5, 5, 6};

const int rook_relevant_bits[64] = {12, 11, 11, 11, 11, 11, 11, 12,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    11, 10, 10, 10, 10, 10, 10, 11,
                                    12, 11, 11, 11, 11, 11, 11, 12};

BitBoard generateKingAttacks(Square sq) {
    BitBoard attacks = BitBoard(0);
    BitBoard initial = BitBoard(0);
    setBit(initial, sq);

    attacks |= shiftNorth(initial);
    attacks |= shiftNorthEast(initial);
    attacks |= shiftEast(initial);
    attacks |= shiftSouthEast(initial);
    attacks |= shiftSouth(initial);
    attacks |= shiftSouthWest(initial);
    attacks |= shiftWest(initial);
    attacks |= shiftNorthWest(initial);

    return attacks;
}

BitBoard generateKnightAttacks(Square sq) {
    BitBoard attacks = BitBoard(0);
    BitBoard initial = BitBoard(0);
    setBit(initial, sq);

    if ((initial >> 17) & ~H_FILE) {
        attacks |= (initial >> 17);
    }
    if ((initial >> 15) & ~A_FILE) {
        attacks |= (initial >> 15);
    }
    if ((initial >> 10) & ~(G_FILE | H_FILE)) {
        attacks |= (initial >> 10);
    }
    if ((initial >> 6) & ~(A_FILE | B_FILE)) {
        attacks |= (initial >> 6);
    }

    if ((initial << 17) & ~A_FILE) {
        attacks |= (initial << 17);
    }
    if ((initial << 15) & ~H_FILE) {
        attacks |= (initial << 15);
    }
    if ((initial << 10) & ~(A_FILE | B_FILE)) {
        attacks |= (initial << 10);
    }
    if ((initial << 6) & ~(G_FILE | H_FILE)) {
        attacks |= (initial << 6);
    }

    return attacks;
}

BitBoard getRookMask(Square sq) {
    BitBoard attacks = BitBoard(0);

    int8_t initial_rank = getRank(sq);
    int8_t initial_file = getFile(sq);

    for (int8_t rank = initial_rank + 1; rank < 7; rank++) {
        attacks |= (1ULL << (rank * 8 + initial_file));
    }
    for (int8_t rank = initial_rank - 1; rank > 0; rank--) {
        attacks |= (1ULL << (rank * 8 + initial_file));
    }
    for (int8_t file = initial_file + 1; file < 7; file++) {
        attacks |= (1ULL << (initial_rank * 8 + file));
    }
    for (int8_t file = initial_file - 1; file > 0; file--) {
        attacks |= (1ULL << (initial_rank * 8 + file));
    }

    return attacks;
}

BitBoard getBishopMask(Square sq) {
    BitBoard attacks = BitBoard(0);

    int8_t initial_rank = getRank(sq);
    int8_t initial_file = getFile(sq);

    for (int8_t rank = initial_rank + 1, file = initial_file + 1; rank < 7 && file < 7; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (int8_t rank = initial_rank - 1, file = initial_file + 1; rank > 0 && file < 7; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (int8_t rank = initial_rank + 1, file = initial_file - 1; rank < 7 && file > 0; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }
    for (int8_t rank = initial_rank - 1, file = initial_file - 1; rank > 0 && file > 0; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;
}

void populateRookMasks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        rook_masks[sq] = getRookMask(static_cast<Square>(sq));
    }
}

void populateBishopMasks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        bishop_masks[sq] = getBishopMask(static_cast<Square>(sq));
    }
}

void populateKingAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        king_attacks[sq] = generateKingAttacks(static_cast<Square>(sq));
    }
}

void populateKnightAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = generateKnightAttacks(static_cast<Square>(sq));
    }
}

void populateBishopAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
    }
}

void populateRookAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
    }
}