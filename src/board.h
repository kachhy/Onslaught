#ifndef BOARD_H
#define BOARD_H

#include <string.h> // for memset
#include <string>
#include <vector>
#include <string>
#include "attacks.h"
#include "bitboard.h"
#include "types.h"
#include "zobrist.h"

using CastlingRights = uint8_t;
using Move = uint32_t;

// Move flags
constexpr uint8_t QUIET_FLAG        = 0b0000;
constexpr uint8_t CASTLE_FLAG       = 0b0001;
constexpr uint8_t CAPTURE_FLAG      = 0b0100;
constexpr uint8_t EP_FLAG           = 0b0110;
constexpr uint8_t PROMO_FLAG        = 0b1000;
constexpr uint8_t KNIGHT_PROMO_FLAG = 0b1000;
constexpr uint8_t BISHOP_PROMO_FLAG = 0b1001;
constexpr uint8_t ROOK_PROMO_FLAG   = 0b1010;
constexpr uint8_t QUEEN_PROMO_FLAG  = 0b1011;

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

// Move encoding/decoding
constexpr Move NO_MOVE = 0;
constexpr inline Move GenerateMove(Square from, Square to, Piece piece, uint32_t flags) {
    return (static_cast<Move>(from)) | (static_cast<Move>(to) << 6) | (static_cast<Move>(piece) << 12) | (static_cast<Move>(flags) << 16);
}
constexpr inline Square From(Move move) { return static_cast<Square>((static_cast<int>(move) & 0x0003f) >> 0); }
constexpr inline Square To(Move move) { return static_cast<Square>((static_cast<int>(move) & 0x00fc0) >> 6); }
constexpr inline Piece MovePiece(Move move) { return static_cast<Piece>((static_cast<int>(move) & 0x0f000) >> 12); }
constexpr inline uint32_t Flags(Move move) { return ((static_cast<uint32_t>(move) & 0x000f0000u) >> 16); }
constexpr inline bool Capture(Move move) { return (Flags(move) & CAPTURE_FLAG) != 0; }
constexpr inline bool IsEP(Move move) { return Flags(move) == EP_FLAG; }
constexpr inline bool Castle(Move move) { return Flags(move) == CASTLE_FLAG; }
constexpr inline bool Prom(Move move) { return (Flags(move) & PROMO_FLAG) != 0; }
std::string moveToStr(Move move);

constexpr inline Side getPieceSide(Piece piece) { return piece <= WHITE_KING ? WHITE : BLACK; }

DefaultPiece promPiece(Move move);

class Board {
public:
    Board(); // Initializes board to default starting state
    Board(const std::string& fen);

    bool loadFEN(const std::string& fen);
    void clear();
    
    // Observers
    Piece pieceAt(uint8_t sq) const;
    BitBoard getOcc(Side side) const;
    std::string getCastlingString() const;
    bool isDraw(uint32_t ply) const;
    void printBoard() const;

    CastlingRights getCastlingRights() const { return castling; }
    Square getEPSquare() const { return ep_square; }
    Square getKingSquare() const { return static_cast<Square>(getLSB(piece_bb[makePiece(KING, stm)])); }
    BitBoard getThreatenedBy(Side side) const { return threatened_by[side]; }
    BitBoard getThreatenedBySTM() const { return threatened_by[stm]; }
    BitBoard getThreatenedByXSTM() const { return threatened_by[xstm]; }
    BitBoard getLegalMask() const { return legal_mask; }
    Side getSTM() const { return stm; }
    Side getXSTM() const { return xstm; }
    BitBoard getPieceBB(Piece p) const { return piece_bb[p]; }
    BitBoard getCheckersMask() const { return checkers; }
    BitBoard getPinMask() const { return pinned; }
    uint64_t hash() const { return zobrist_hash; }
    bool inCheck() const { return static_cast<bool>(checkers); }
    uint8_t getFMR() const { return fmr; }
    int phase() const { return phase_score; }

    // Make and undo move
    void makeMove(Move move);
    void undoMove(Move move);

    // Zobrist setup
    void refreshZobrist();
private:
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

        BoardHistory(CastlingRights castling, Square ep_square, uint32_t null_move_number, uint8_t fmr, Piece captured_piece, BitBoard checkers, BitBoard legal_mask, BitBoard white_threats, BitBoard black_threats, BitBoard pinned, uint64_t zobrist_hash)
                    : castling(castling), ep_square(ep_square), null_move_number(null_move_number), fmr(fmr), 
                      captured_piece(captured_piece), checkers(checkers), legal_mask(legal_mask), pinned(pinned), zobrist_hash(zobrist_hash) {
                        threatened_by[WHITE] = white_threats;
                        threatened_by[BLACK] = black_threats;
                    }
    };

    // Private member functions
    void setSpecials();
    void setThreatened();
    void setPieceBoard();
    void setOcc();
    void setPhase();

    // Draw detection functions
    bool isMaterialDraw() const;
    bool isRepetitionDraw(uint32_t ply) const;

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
    Side stm; // Side to move
    Side xstm; // Not side to move
    int phase_score;

    // Move counting
    uint32_t move_number;
    uint32_t null_move_number; // For repetition checking
    uint8_t fmr; // For fifty-move rule draw

    // History
    std::vector<BoardHistory> history;

    // Hashing
    uint64_t zobrist_hash;
};

#include "movegen.h"
bool isFiftyMoveRuleDraw(Board& board);

#endif // BOARD_H