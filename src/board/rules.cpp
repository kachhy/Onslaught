#include "rules.h"
#include "movegen/movegen.h"

bool isMaterialDraw(const Board& board) {
    if (board.getPieceBB(WHITE_PAWN) | board.getPieceBB(BLACK_PAWN) | board.getPieceBB(WHITE_ROOK) | board.getPieceBB(BLACK_ROOK) | board.getPieceBB(WHITE_QUEEN) |
        board.getPieceBB(BLACK_QUEEN)) { // Non-material draw
        return false;
    }

    const uint8_t white_knights = bitCount(board.getPieceBB(WHITE_KNIGHT));
    const uint8_t black_knights = bitCount(board.getPieceBB(BLACK_KNIGHT));
    const uint8_t white_bishops = bitCount(board.getPieceBB(WHITE_BISHOP));
    const uint8_t black_bishops = bitCount(board.getPieceBB(BLACK_BISHOP));
    const uint8_t total_minorpc = white_knights + black_knights + white_bishops + black_bishops;

    if (total_minorpc <= 1) { // KvK or KvK + minor piece
        return true;
    }
    if (total_minorpc == 2) {
        if (white_knights == 1 && black_knights == 1) { // KNvKN
            return true;
        }
        if (white_bishops == 1 && black_bishops == 1) { // KBvKB
            return true;
        }
        if (white_knights == 2 || black_knights == 2) { // KNNvK
            return true;
        }
        if ((white_knights == 1 && black_bishops == 1) || (white_bishops == 1 && black_knights == 1)) { // KNvKB
            return true;
        }
    }

    return false;
}

bool isFiftyMoveRuleDraw(const Board& board) {
    if (board.getFMR() > 99) {
        if (board.inCheck()) {
            return !getLegalMoves(board).empty();
        }
        return true;
    }
    return false;
}

bool isRepetitionDraw(const Board& board, uint32_t ply) {
    uint16_t distance = std::min(board.getNullMoveNumber(), static_cast<uint32_t>(board.getFMR()));
    uint16_t r = 0;
    const Board::BoardHistory* board_history = board.getBoardHistory();
    for (int32_t i = board.getHistPly() - 4; i >= 0 && i >= static_cast<int64_t>(board.getHistPly()) - distance; i -= 2) {
        if (board_history[i].zobrist_hash == board.hash()) {
            if (i > static_cast<int64_t>(board.getHistPly()) - ply) {
                return true;
            }
            if (++r == 2) {
                return true;
            }
        }
    }

    return false;
}

bool isDraw(const Board& board, uint32_t ply) { return isMaterialDraw(board) || isRepetitionDraw(board, ply) || isFiftyMoveRuleDraw(board); }