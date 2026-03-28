#ifndef BOARD_H
#define BOARD_H

#include "core/bitboard.h"
#include "core/move.h"
#include <algorithm>
#include <vector>
#include <cstring>

using CastlingRights = uint8_t;

// Castling rights flags
constexpr uint8_t WHITE_KS = 0x8;
constexpr uint8_t WHITE_QS = 0x4;
constexpr uint8_t BLACK_KS = 0x2;
constexpr uint8_t BLACK_QS = 0x1;

constexpr BitBoard WHITE_KINGSIDE_CASTLE_MASK         = 0x6000000000000000;
constexpr BitBoard WHITE_QUEENSIDE_CASTLE_OCC_MASK    = 0x0E00000000000000;
constexpr BitBoard WHITE_QUEENSIDE_CASTLE_THREAT_MASK = 0x0C00000000000000;
constexpr BitBoard BLACK_KINGSIDE_CASTLE_MASK         = 0x0000000000000060;
constexpr BitBoard BLACK_QUEENSIDE_CASTLE_OCC_MASK    = 0x000000000000000E;
constexpr BitBoard BLACK_QUEENSIDE_CASTLE_THREAT_MASK = 0x000000000000000C;

// Engine constants
constexpr uint16_t MAX_PLY = 256;

constexpr inline Side getPieceSide(Piece piece) { return piece <= WHITE_KING ? WHITE : BLACK; }

class Board {
public:
    Board(); // Initializes board to default starting state
    Board(const std::string& fen);

    bool loadFEN(const std::string& fen);
    void clear();

    // Observers
    Piece pieceAt(uint8_t sq) const;
    BitBoard getOcc(Side side) const;
    BitBoard getDiscoveryAttacks(const Square sq, const Side side) const;
    std::string getCastlingString() const;
    void printBoard() const;

    CastlingRights getCastlingRights() const { return castling; }
    Square getEPSquare() const { return ep_square; }
    Square getKingSquare() const { return static_cast<Square>(getLSB(piece_bb[makePiece(KING, stm)])); }
    BitBoard getThreatenedBy(Side side) const { return threatened_by[side]; }
    BitBoard getThreatenedBySTM() const { return threatened_by[stm]; }
    BitBoard getThreatenedByXSTM() const { return threatened_by[xstm]; }
    BitBoard getLegalMask() const { return legal_mask; }
    uint32_t getNullMoveNumber() const { return null_move_number; }
    Side getSTM() const { return stm; }
    Side getXSTM() const { return xstm; }
    BitBoard getPieceBB(Piece p) const { return piece_bb[p]; }
    BitBoard getCheckersMask() const { return checkers; }
    BitBoard getPinMask() const { return pinned; }
    uint64_t hash() const { return zobrist_hash; }
    bool inCheck() const { return static_cast<bool>(checkers); }
    uint8_t getFMR() const { return fmr; }
    int phase() const { return std::clamp(phase_score, 0, 24); }
    uint64_t pawnHash() const { return pawn_hash; }

    // Make and undo move
    void makeMove(Move move);
    void undoMove(Move move);

    // Zobrist setup
    void refreshZobrist();
    struct BoardHistory {
        CastlingRights castling;
        Square ep_square;
        uint32_t null_move_number;
        uint8_t fmr;
        Piece captured_piece;
        BitBoard checkers;
        BitBoard legal_mask;
        BitBoard threatened_by[2];
        BitBoard pinned;
        uint64_t zobrist_hash;
        uint64_t pawn_hash;

        BoardHistory(
            CastlingRights castling, Square ep_square, uint32_t null_move_number, uint8_t fmr, Piece captured_piece, BitBoard checkers, BitBoard legal_mask,
            BitBoard white_threats, BitBoard black_threats, BitBoard pinned, uint64_t zobrist_hash, uint64_t pawn_hash
        ) :
            castling(castling), ep_square(ep_square), null_move_number(null_move_number), fmr(fmr), captured_piece(captured_piece), checkers(checkers),
            legal_mask(legal_mask), pinned(pinned), zobrist_hash(zobrist_hash), pawn_hash(pawn_hash) {
            threatened_by[WHITE] = white_threats;
            threatened_by[BLACK] = black_threats;
        }
    };
    std::vector<BoardHistory> getBoardHistory() const { return history; }
private:

    // Private member functions
    void setSpecials();
    void setThreatened();
    void setPieceBoard();
    void setOcc();
    void setPhase();

    // Note: 0 is white side, 64 is black side
    BitBoard piece_bb[12];
    BitBoard occ[3];
    Piece piece_board[64];
    int castling_rights[64];
    BitBoard checkers;
    BitBoard legal_mask;
    BitBoard threatened_by[2];
    BitBoard pinned;
    CastlingRights castling; // castling mask (i.e. 1111 = KQkq)
    Square ep_square;
    Side stm;  // Side to move
    Side xstm; // Not side to move
    int phase_score;

    // Move counting
    uint32_t move_number;
    uint32_t null_move_number; // For repetition checking
    uint8_t fmr;               // For fifty-move rule draw

    // History
    std::vector<BoardHistory> history;

    // Hashing
    uint64_t zobrist_hash;
    uint64_t pawn_hash;
};

#endif // BOARD_H