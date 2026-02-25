#ifndef BOARD_H
#define BOARD_H

#include <string.h> // for memset
#include <vector>
#include <string>
#include "bitboard.h"

typedef uint8_t CastlingRights;
typedef uint32_t Move;

#define GenerateMove(from, to, piece, flags) (from) | ((to) << 6) | ((piece) << 12) | ((flags) << 16)
#define From(move)                           ((static_cast<int>(move) & 0x0003f) >> 0)
#define To(move)                             ((static_cast<int>(move) & 0x00fc0) >> 6)
#define MovePiece(move)                      ((static_cast<int>(move) & 0x0f000) >> 12)
#define Flags(move)                          ((static_cast<int>(move) & 0xf0000) >> 16)

#define MAX_PLY    256

enum Square {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = 64 
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

// STM enum
enum Side {
    WHITE = 0,
    BLACK,
    BOTH // For occupancy only
};

struct BoardHistory {
    CastlingRights castling; 
    Square ep_square;
    uint8_t fmr;
};

class Board {
public:
    Board(); // Initializes board to default starting state

    Piece pieceAt(uint8_t sq) const;
    void setOcc();
    void printBoard() const;
    bool loadFEN(const std::string& fen);
    void clear();
private:
    // Note: 0 is white side, 64 is black side
    BitBoard piece_bb[12];
    BitBoard occ[3];
    CastlingRights castling; // castling mask (i.e. 1111 = KQkq)
    Square ep_square;
    Side stm; // Side to move
    Side xstm; // Not side to move

    // Move counting
    uint8_t fmr; // For fifty-move rule draw
    uint32_t move_number;

    // History
    std::vector<BoardHistory> history;
};

#endif // BOARD_H