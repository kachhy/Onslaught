#include "attacks.h"

BitBoard bishop_masks[64];
BitBoard rook_masks[64];

BitBoard pawn_attacks[2][64];
BitBoard knight_attacks[64];
BitBoard bishop_attacks[64][4096];
BitBoard rook_attacks[64][4096];
BitBoard king_attacks[64];

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

BitBoard generatePawnAttacks(Square sq, Side side) {
    BitBoard attacks = BitBoard(0);
    BitBoard initial = BitBoard(0);
    setBit(initial, sq);

    if (side == WHITE) {
        attacks |= shiftNorthEast(initial);
        attacks |= shiftNorthWest(initial);
    }
    else {
        attacks |= shiftSouthEast(initial);
        attacks |= shiftSouthWest(initial);
    }

    return attacks;
}

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

BitBoard getPawnAttacks(Square sq, Side side) {
    return pawn_attacks[side][sq];
}

BitBoard getKnightAttacks(Square sq) {
    return knight_attacks[sq];
}

BitBoard getBishopAttacks(Square sq, BitBoard occ) {
    occ &= bishop_masks[sq];
    occ *= bishop_magics[sq];
    occ >>= 64 - (bishop_relevant_bits[sq]);
    return bishop_attacks[sq][occ];
}

BitBoard getRookAttacks(Square sq, BitBoard occ) {
    occ &= rook_masks[sq];
    occ *= rook_magics[sq];
    occ >>= 64 - (rook_relevant_bits[sq]);
    return rook_attacks[sq][occ];
}

BitBoard getQueenAttacks(Square sq, BitBoard occ) {
    return getBishopAttacks(sq, occ) | getRookAttacks(sq, occ);
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

BitBoard computeBishopAttacks(Square sq, BitBoard blockers) {
    BitBoard attacks = BitBoard(0);

    int8_t initial_row = getRank(sq);
    int8_t initial_file = getFile(sq);

    for (int8_t rank = initial_row + 1, file = initial_file + 1; rank < 8 && file < 8; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if (getBit(blockers, rank * 8 + file)) {
            break;
        }
    }

    for (int8_t rank = initial_row - 1, file = initial_file + 1; rank >= 0 && file < 8; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if (getBit(blockers, rank * 8 + file)) {
            break;
        }
    }

    for (int8_t rank = initial_row + 1, file = initial_file - 1; rank < 8 && file >= 0; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if (getBit(blockers, rank * 8 + file)) {
            break;
        }
    }

    for (int8_t rank = initial_row - 1, file = initial_file - 1; rank >= 0 && file >= 0; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if (getBit(blockers, rank * 8 + file)) {
            break;
        }
    }

    return attacks;
}

BitBoard computeRookAttacks(Square sq, BitBoard blockers) {
    BitBoard attacks = BitBoard(0);

    int8_t initial_rank = getRank(sq);
    int8_t initial_file = getFile(sq);

    for (int8_t rank = initial_rank + 1; rank < 8; rank++) {
        attacks |= (1ULL << (rank * 8 + initial_file));
        if (getBit(blockers, rank * 8 + initial_file)) {
            break;
        }
    }

    for (int8_t rank = initial_rank - 1; rank >= 0; rank--) {
        attacks |= (1ULL << (rank * 8 + initial_file));
        if (getBit(blockers, rank * 8 + initial_file)) {
            break;
        }
    }

    for (int8_t file = initial_file + 1; file < 8; file++) {
        attacks |= (1ULL << (initial_rank * 8 + file));
        if (getBit(blockers, initial_rank * 8 + file)) {
            break;
        }
    }

    for (int8_t file = initial_file - 1; file >= 0; file--) {
        attacks |= (1ULL << (initial_rank * 8 + file));
        if (getBit(blockers, initial_rank * 8 + file)) {
            break;
        }
    }

    return attacks;
}

BitBoard setPieceLayoutOcc(uint16_t idx, uint8_t bits, BitBoard attacks) {
    BitBoard occ = BitBoard(0);

    for (uint8_t i = 0; i < bits; i++) {
        Square sq = static_cast<Square>(popLSB(attacks));

        // TODO: change to getBit for simplicity
        if (idx & (1 << i)) {
            setBit(occ, sq);
        }
    }

    return occ;
}

void populateBishopMasks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        bishop_masks[sq] = getBishopMask(static_cast<Square>(sq));
    }
}

void populateRookMasks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        rook_masks[sq] = getRookMask(static_cast<Square>(sq));
    }
}

void populatePawnAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        pawn_attacks[WHITE][sq] = generatePawnAttacks(static_cast<Square>(sq), WHITE);
        pawn_attacks[BLACK][sq] = generatePawnAttacks(static_cast<Square>(sq), BLACK);
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
        BitBoard mask  = bishop_masks[sq];
        uint8_t r_bits = bishop_relevant_bits[sq];
        uint16_t n     = (1 << r_bits);

        for (uint16_t i = 0; i < n; i++) {
            BitBoard occ   = setPieceLayoutOcc(i, r_bits, mask);
            uint16_t index = (occ * bishop_magics[sq]) >> (64 - r_bits);
            bishop_attacks[sq][index] = computeBishopAttacks(static_cast<Square>(sq), occ);
        }
    }
}

void populateRookAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        BitBoard mask  = rook_masks[sq];
        uint8_t r_bits = rook_relevant_bits[sq];
        uint16_t n     = (1 << r_bits);

        for (uint16_t i = 0; i < n; i++) {
            BitBoard occ   = setPieceLayoutOcc(i, r_bits, mask);
            uint16_t index = (occ * rook_magics[sq]) >> (64 - r_bits);
            rook_attacks[sq][index] = computeRookAttacks(static_cast<Square>(sq), occ);
        }
    }
}