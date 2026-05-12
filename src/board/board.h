#ifndef BOARD_H
#define BOARD_H

#include "core/bitboard.h"
#include "core/move.h"
#include "search-eval/terms.h"
#include <algorithm>
#include <cstring>
#include <vector>

using CastlingRights = uint8_t;

// Castling rights flags
constexpr uint8_t WHITE_KS = 0x8;
constexpr uint8_t WHITE_QS = 0x4;
constexpr uint8_t BLACK_KS = 0x2;
constexpr uint8_t BLACK_QS = 0x1;

constexpr BitBoard WHITE_KINGSIDE_CASTLE_MASK = 0x6000000000000000;
constexpr BitBoard WHITE_QUEENSIDE_CASTLE_OCC_MASK = 0x0E00000000000000;
constexpr BitBoard WHITE_QUEENSIDE_CASTLE_THREAT_MASK = 0x0C00000000000000;
constexpr BitBoard BLACK_KINGSIDE_CASTLE_MASK = 0x0000000000000060;
constexpr BitBoard BLACK_QUEENSIDE_CASTLE_OCC_MASK = 0x000000000000000E;
constexpr BitBoard BLACK_QUEENSIDE_CASTLE_THREAT_MASK = 0x000000000000000C;

// Engine constants
constexpr uint16_t MAX_PLY = 256;

constexpr inline Side getPieceSide(Piece piece) { return piece <= WHITE_KING ? WHITE : BLACK; }

struct EvalInfo {
    BitBoard pawn_attacks[2];
    BitBoard piece_attacks[64];
};

class Board {
public:
    Board(); // Initializes board to default starting state
    Board(const std::string& fen);

    bool loadFEN(const std::string& fen);
    void clear();

    // Observers
    Piece pieceAt(uint8_t sq, bool reset = false) const;
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
    int getHistPly() const { return history_ply; }
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
    Score getMaterialPST() const { return material_pst_score; }
    const EvalInfo& getEvalInfo() const { return eval_info; }

    // Make and undo move
    void makeMove(Move move);
    void undoMove(Move move);

    void makeNullMove();
    void undoNullMove();

    // Zobrist setup
    void refreshZobrist();
    struct BoardHistory {
        CastlingRights castling;
        Square ep_square;
        uint8_t fmr;
        Piece captured_piece;
        BitBoard checkers;
        BitBoard legal_mask;
        BitBoard threatened_by[2];
        BitBoard pinned;
        uint64_t zobrist_hash;
        uint64_t pawn_hash;
        Score material_pst_score;
        EvalInfo eval_info;

        BoardHistory() :
            castling(0), ep_square(NO_SQUARE), fmr(0), captured_piece(NO_PIECE), checkers(0), legal_mask(0), pinned(0), zobrist_hash(0),
            pawn_hash(0), material_pst_score(0) {
            threatened_by[0] = 0;
            threatened_by[1] = 0;
            memset(&eval_info, 0, sizeof(EvalInfo));
        }

        BoardHistory(
            CastlingRights castling, Square ep_square, uint8_t fmr, Piece captured_piece, BitBoard checkers, BitBoard legal_mask,
            BitBoard white_threats, BitBoard black_threats, BitBoard pinned, uint64_t zobrist_hash, uint64_t pawn_hash, Score material_pst_score,
            const EvalInfo& eval_info
        ) :
            castling(castling), ep_square(ep_square), fmr(fmr), captured_piece(captured_piece), checkers(checkers),
            legal_mask(legal_mask), pinned(pinned), zobrist_hash(zobrist_hash), pawn_hash(pawn_hash), material_pst_score(material_pst_score) {
            threatened_by[WHITE] = white_threats;
            threatened_by[BLACK] = black_threats;
            memcpy(&this->eval_info, &eval_info, sizeof(EvalInfo));
        }
    };
    const BoardHistory* getBoardHistory() const { return history; }

    // Search
    Move killers[MAX_PLY][2];
    int static_evals[MAX_PLY]; // for improving
    int score_history[64][12]; // [stm][to][piece]
private:

    // Private member functions
    void setSpecials();
    void setThreatened();
    void setPieceBoard();
    void setOcc();
    void setPhase();
    void refreshMaterialPST();

    uint64_t zobrist_hash;
    uint64_t pawn_hash;
    BitBoard checkers;
    BitBoard legal_mask;
    BitBoard pinned;
    Side stm;  // Side to move
    Side xstm; // Not side to move
    Square ep_square;
    CastlingRights castling; // castling mask (i.e. 1111 = KQkq)
    uint8_t fmr;             // For fifty-move rule draw

    // Note: 0 is white side, 64 is black side
    BitBoard piece_bb[12];
    BitBoard occ[3];
    BitBoard threatened_by[2];
    Piece piece_board[64];
    uint8_t castling_rights[64];

    int phase_score;
    Score material_pst_score;
    EvalInfo eval_info;

    // History
    BoardHistory history[MAX_PLY + MAX_GAME_MOVES];
    int history_ply;

    // Move counting
    uint32_t move_number;
};

#endif // BOARD_H