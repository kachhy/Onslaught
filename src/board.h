#ifndef BOARD_H
#define BOARD_H

#include <string.h> // for memset
#include <vector>
#include <string>
#include "attacks.h"
#include "bitboard.h"
#include "types.h"

typedef uint8_t CastlingRights;
typedef uint32_t Move;

#define GenerateMove(from, to, piece, flags) (from) | ((to) << 6) | ((piece) << 12) | ((flags) << 16)
#define From(move)                           ((static_cast<int>(move) & 0x0003f) >> 0)
#define To(move)                             ((static_cast<int>(move) & 0x00fc0) >> 6)
#define MovePiece(move)                      ((static_cast<int>(move) & 0x0f000) >> 12)
#define Flags(move)                          ((static_cast<int>(move) & 0xf0000) >> 16)

#define MAX_PLY    256

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

struct BoardHistory {
    CastlingRights castling; 
    Square ep_square;
    uint8_t fmr;
};

class Board {
public:
    Board(); // Initializes board to default starting state
    Board(const std::string& fen);

    Piece pieceAt(uint8_t sq) const;
    void setOcc();
    void printBoard() const;
    bool loadFEN(const std::string& fen);
    void clear();
    BitBoard getOcc(Side side) const;
private:
    // Note: 0 is white side, 64 is black side
    BitBoard piece_bb[12];
    BitBoard occ[3];
    CastlingRights castling; // castling mask (i.e. 1111 = KQkq)
    Square ep_square;
    Side stm; // Side to move
    Side xstm; // Not side to move

    // Move counting
    uint32_t move_number;
    uint8_t fmr; // For fifty-move rule draw

    // History
    std::vector<BoardHistory> history;
};

#endif // BOARD_H