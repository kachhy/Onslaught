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

// Move flags
#define QUIET_FLAG        0b0000
#define CASTLE_FLAG       0b0001
#define CAPTURE_FLAG      0b0100
#define EP_FLAG           0b0110
#define PROMO_FLAG        0b1000
#define KNIGHT_PROMO_FLAG 0b1000
#define BISHOP_PROMO_FLAG 0b1001
#define ROOK_PROMO_FLAG   0b1010
#define QUEEN_PROMO_FLAG  0b1011

// Castling rights flags
#define WHITE_KS 0x8
#define WHITE_QS 0x4
#define BLACK_KS 0x2
#define BLACK_QS 0x1

#define GenerateMove(from, to, piece, flags) (from) | ((to) << 6) | ((piece) << 12) | ((flags) << 16)
#define From(move)                           static_cast<Square>((static_cast<int>(move) & 0x0003f) >> 0)
#define To(move)                             static_cast<Square>((static_cast<int>(move) & 0x00fc0) >> 6)
#define MovePiece(move)                      static_cast<Piece>((static_cast<int>(move) & 0x0f000) >> 12)
#define Flags(move)                          ((static_cast<int>(move) & 0xf0000) >> 16)
#define Capture(move)                        (Flags(move) & CAPTURE_FLAG)
#define IsEP(move)                           (Flags(move) == EP_FLAG)
#define Castle(move)                         (Flags(move) == CASTLE_FLAG)
#define Prom(move)                           (Flags(move) & PROMO_FLAG)

DefaultPiece promPiece(Move move);

#define MAX_PLY    256

struct BoardHistory {
    CastlingRights castling; 
    Square ep_square;
    uint8_t fmr;
    Piece captured_piece;

    BoardHistory(CastlingRights castling, Square ep_square, uint8_t fmr, Piece captured_piece) : castling(castling), ep_square(ep_square), fmr(fmr), captured_piece(captured_piece) {}
};

class Board {
public:
    Board(); // Initializes board to default starting state
    Board(const std::string& fen);

    Piece pieceAt(uint8_t sq) const;
    void setPieceBoard();
    void setOcc();
    std::string getCastlingString() const;
    void printBoard() const;
    bool loadFEN(const std::string& fen);
    void clear();
    BitBoard getOcc(Side side) const;

    // Make and undo move
    void makeMove(Move move);
    void undoMove(Move move);
private:
    // Note: 0 is white side, 64 is black side
    BitBoard piece_bb[12];
    BitBoard occ[3];
    Piece piece_board[64];
    int castling_rights[64];
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