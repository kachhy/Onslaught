#include "board.h"
#include "core/types.h"
#include "hash/zobrist.h"
#include "movegen/attacks.h"
#include <cassert>
#include <iostream>
#include <sstream>

Board::Board() { loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

Board::Board(const std::string& fen) { loadFEN(fen); }

void Board::setPieceBoard() {
    memset(piece_board, 0, sizeof(piece_board)); // TODO: unclear if we need this -> test

    for (uint8_t i = 0; i < 64; i++) {
        piece_board[i] = pieceAt(i, true);
    }
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

Piece Board::pieceAt(uint8_t sq, bool reset) const {
    if (reset) {
        if (!getBit(occ[BOTH], sq)) {
            return NO_PIECE;
        }
        for (uint8_t p = WHITE_PAWN; p <= BLACK_KING; p++) {
            if (getBit(piece_bb[p], sq)) {
                return static_cast<Piece>(p);
            }
        }

        return NO_PIECE; // Exceptional state
    }

    return piece_board[sq];
}

std::string Board::getCastlingString() const {
    std::string out = "";
    if (castling & 8) {
        out += 'K';
    } else {
        out += '-';
    }

    if (castling & 4) {
        out += 'Q';
    } else {
        out += '-';
    }

    if (castling & 2) {
        out += 'k';
    } else {
        out += '-';
    }

    if (castling & 1) {
        out += 'q';
    } else {
        out += '-';
    }

    return out;
}

std::string Board::toFEN() const {
    std::string out = "";

    // Piece placement (ranks 8 -> 1)
    for (int rank = 0; rank < 8; ++rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            uint8_t sq = static_cast<uint8_t>(rank * 8 + file);
            Piece p = piece_board[sq];

            if (p == NO_PIECE) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                out += static_cast<char>('0' + empty);
                empty = 0;
            }

            switch (p) {
            case WHITE_PAWN: out += 'P'; break;
            case WHITE_KNIGHT: out += 'N'; break;
            case WHITE_BISHOP: out += 'B'; break;
            case WHITE_ROOK: out += 'R'; break;
            case WHITE_QUEEN: out += 'Q'; break;
            case WHITE_KING: out += 'K'; break;
            case BLACK_PAWN: out += 'p'; break;
            case BLACK_KNIGHT: out += 'n'; break;
            case BLACK_BISHOP: out += 'b'; break;
            case BLACK_ROOK: out += 'r'; break;
            case BLACK_QUEEN: out += 'q'; break;
            case BLACK_KING: out += 'k'; break;
            default: break;
            }
        }

        if (empty > 0) {
            out += static_cast<char>('0' + empty);
        }
        if (rank < 7) {
            out += '/';
        }
    }

    // Active color
    out += ' ';
    out += (stm == WHITE ? 'w' : 'b');

    // Castling rights
    out += ' ';
    if (castling == 0) {
        out += '-';
    } else {
        if (castling & WHITE_KS) {
            out += 'K';
        }
        if (castling & WHITE_QS) {
            out += 'Q';
        }
        if (castling & BLACK_KS) {
            out += 'k';
        }
        if (castling & BLACK_QS) {
            out += 'q';
        }
    }

    // En-passant
    out += ' ';
    if (ep_square == NO_SQUARE) {
        out += '-';
    } else {
        out += board_coords[ep_square];
    }

    // Halfmove and fullmove counters
    out += ' ';
    out += std::to_string(static_cast<int>(fmr));
    out += ' ';
    out += std::to_string(move_number);

    return out;
}

void Board::printBoard() const {
    for (int8_t i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            std::cout << "\n" << (8 - (i >> 3)) << " ";
        }
        switch (pieceAt(i)) {
        case WHITE_PAWN: std::cout << "P "; break;
        case WHITE_KNIGHT: std::cout << "N "; break;
        case WHITE_BISHOP: std::cout << "B "; break;
        case WHITE_ROOK: std::cout << "R "; break;
        case WHITE_QUEEN: std::cout << "Q "; break;
        case WHITE_KING: std::cout << "K "; break;
        case BLACK_PAWN: std::cout << "p "; break;
        case BLACK_KNIGHT: std::cout << "n "; break;
        case BLACK_BISHOP: std::cout << "b "; break;
        case BLACK_ROOK: std::cout << "r "; break;
        case BLACK_QUEEN: std::cout << "q "; break;
        case BLACK_KING: std::cout << "k "; break;
        case NO_PIECE: std::cout << ". "; break;
        }
    }
    std::cout << "\n  a b c d e f g h\n"
              << "\nTurn: " << (stm == WHITE ? "White" : "Black") << "\nEP Square: " << board_coords[ep_square] << "\nCastling: " << getCastlingString()
              << "\nZobrist: " << zobrist_hash << "\nPawn hash: " << pawn_hash << "\nPhase: " << phase_score << "\n"
              << std::endl;
}

void Board::clear() {
    memset(piece_bb, 0, sizeof(piece_bb));
    memset(occ, 0, sizeof(occ));
    memset(score_history, 0, sizeof(score_history));
    history_ply = 0;

    threatened_by[WHITE] = 0ULL;
    threatened_by[BLACK] = 0ULL;
    material_pst_score = Score();

    // Turns
    stm = WHITE;
    xstm = BLACK;

    // Counters
    move_number = 1;
    fmr = 0;

    // EP squares and Castling rights
    ep_square = NO_SQUARE;
    castling = 0xF;

    for (uint8_t i = 0; i < 64; i++) {
        castling_rights[i] = 0xf;
    }

    // White king and rooks
    castling_rights[E1] = 0xf ^ (WHITE_KS | WHITE_QS);
    castling_rights[H1] = 0xf ^ WHITE_KS;
    castling_rights[A1] = 0xf ^ WHITE_QS;

    // Black king and rooks
    castling_rights[E8] = 0xf ^ (BLACK_KS | BLACK_QS);
    castling_rights[H8] = 0xf ^ BLACK_KS;
    castling_rights[A8] = 0xf ^ BLACK_QS;
}

bool Board::loadFEN(const std::string& fen) {
    clear();

    std::istringstream iss(fen);
    std::string placement, active, castling_str, ep_str, halfmove_str, fullmove_str;

    if (!(iss >> placement >> active >> castling_str >> ep_str)) {
        return false;
    }

    iss >> halfmove_str;
    iss >> fullmove_str;

    // Parse piece placement (ranks 8 -> 1)
    int rank = 0;
    int file = 0;
    for (char c : placement) {
        if (c == '/') {
            ++rank;
            file = 0;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            file += c - '0';
            continue;
        }

        if (file >= 8 || rank > 7) {
            return false; // malformed
        }

        uint8_t sq = static_cast<uint8_t>(rank * 8 + file);
        switch (c) {
        case 'P': setBit(piece_bb[WHITE_PAWN], sq); break;
        case 'N': setBit(piece_bb[WHITE_KNIGHT], sq); break;
        case 'B': setBit(piece_bb[WHITE_BISHOP], sq); break;
        case 'R': setBit(piece_bb[WHITE_ROOK], sq); break;
        case 'Q': setBit(piece_bb[WHITE_QUEEN], sq); break;
        case 'K': setBit(piece_bb[WHITE_KING], sq); break;
        case 'p': setBit(piece_bb[BLACK_PAWN], sq); break;
        case 'n': setBit(piece_bb[BLACK_KNIGHT], sq); break;
        case 'b': setBit(piece_bb[BLACK_BISHOP], sq); break;
        case 'r': setBit(piece_bb[BLACK_ROOK], sq); break;
        case 'q': setBit(piece_bb[BLACK_QUEEN], sq); break;
        case 'k': setBit(piece_bb[BLACK_KING], sq); break;
        default: return false;
        }

        ++file;
    }

    // Active color
    if (active == "w") {
        stm = WHITE;
        xstm = BLACK;
    } else if (active == "b") {
        stm = BLACK;
        xstm = WHITE;
    } else {
        return false;
    }

    // Castling rights: KQkq -> bits 1,2,4,8
    castling = 0;
    if (castling_str != "-") {
        for (char ch : castling_str) {
            switch (ch) {
            case 'K': castling |= WHITE_KS; break;
            case 'Q': castling |= WHITE_QS; break;
            case 'k': castling |= BLACK_KS; break;
            case 'q': castling |= BLACK_QS; break;
            default: break;
            }
        }
    }

    // En-passant
    if (ep_str == "-") {
        ep_square = NO_SQUARE;
    } else if (ep_str.size() == 2 && ep_str[0] >= 'a' && ep_str[0] <= 'h' && ep_str[1] >= '1' && ep_str[1] <= '8') {
        int f = ep_str[0] - 'a';
        int r = '8' - ep_str[1];
        ep_square = static_cast<Square>(r * 8 + f);
    } else {
        return false;
    }

    // Halfmove clock
    if (!halfmove_str.empty()) {
        try {
            fmr = static_cast<uint8_t>(std::stoi(halfmove_str));
        } catch (...) {
            fmr = 0;
        }
    }

    // Fullmove number
    if (!fullmove_str.empty()) {
        try {
            move_number = static_cast<uint32_t>(std::stoul(fullmove_str));
        } catch (...) {
            move_number = 1;
        }
    }

    setOcc();
    setPieceBoard();
    setSpecials();
    setPhase();
    refreshMaterialPST();
    refreshZobrist();
    accumulator.refresh(*this);

    return true;
}

BitBoard Board::getOcc(Side side) const { return occ[side]; }

BitBoard Board::getDiscoveryAttacks(const Square sq, const Side side) const {
    BitBoard bishop_attacks = getPieceAttacks(static_cast<Piece>(BISHOP), sq, occ[BOTH]);
    BitBoard rook_attacks = getPieceAttacks(static_cast<Piece>(ROOK), sq, occ[BOTH]);
    BitBoard bishops = piece_bb[makePiece(BISHOP, side == WHITE ? BLACK : WHITE)] & ~bishop_attacks;
    BitBoard rooks = piece_bb[makePiece(ROOK, side == WHITE ? BLACK : WHITE)] & ~rook_attacks;
    return (bishops & getPieceAttacks(static_cast<Piece>(BISHOP), sq, occ[BOTH] & ~bishop_attacks)) |
           (rooks & getPieceAttacks(static_cast<Piece>(ROOK), sq, occ[BOTH] & ~rook_attacks));
}

void Board::makeMove(Move move) {
    Square from = From(move);
    Square to = To(move);
    Piece piece = MovePiece(move);
    Piece captured = IsEP(move) ? makePiece(PAWN, xstm) : piece_board[to];

    history[history_ply++] = BoardHistory(
        castling, ep_square, fmr, captured, checkers, legal_mask, threatened_by[WHITE], threatened_by[BLACK], pinned, zobrist_hash, pawn_hash, material_pst_score,
        eval_info, accumulator
    );

    accumulator.onMove(move, *this);

    fmr++;

    flipBits(piece_bb[piece], from, to);
    flipBits(occ[stm], from, to);
    flipBits(occ[BOTH], from, to);

    piece_board[from] = NO_PIECE;
    piece_board[to] = piece;

    Score pst_delta = pst[piece][to] - pst[piece][from];
    if (stm == WHITE) {
        material_pst_score += pst_delta;
    } else {
        material_pst_score -= pst_delta;
    }

    zobrist_hash ^= piece_keys[piece][from];

    if (Castle(move)) {
        switch (to) {
        // K
        case G1:
            // move H rook
            popBit(piece_bb[WHITE_ROOK], H1);
            setBit(piece_bb[WHITE_ROOK], F1);
            flipBit(occ[stm], H1);
            flipBit(occ[BOTH], H1);
            flipBit(occ[stm], F1);
            flipBit(occ[BOTH], F1);
            piece_board[H1] = NO_PIECE;
            piece_board[F1] = WHITE_ROOK;
            zobrist_hash ^= piece_keys[WHITE_ROOK][H1];
            zobrist_hash ^= piece_keys[WHITE_ROOK][F1];
            material_pst_score += pst[WHITE_ROOK][F1] - pst[WHITE_ROOK][H1];
            break;
        // Q
        case C1:
            // move A rook
            popBit(piece_bb[WHITE_ROOK], A1);
            setBit(piece_bb[WHITE_ROOK], D1);
            flipBit(occ[stm], A1);
            flipBit(occ[BOTH], A1);
            flipBit(occ[stm], D1);
            flipBit(occ[BOTH], D1);
            piece_board[A1] = NO_PIECE;
            piece_board[D1] = WHITE_ROOK;
            zobrist_hash ^= piece_keys[WHITE_ROOK][A1];
            zobrist_hash ^= piece_keys[WHITE_ROOK][D1];
            material_pst_score += pst[WHITE_ROOK][D1] - pst[WHITE_ROOK][A1];
            break;
        // k
        case G8:
            popBit(piece_bb[BLACK_ROOK], H8);
            setBit(piece_bb[BLACK_ROOK], F8);
            flipBit(occ[stm], H8);
            flipBit(occ[BOTH], H8);
            flipBit(occ[stm], F8);
            flipBit(occ[BOTH], F8);
            piece_board[H8] = NO_PIECE;
            piece_board[F8] = BLACK_ROOK;
            zobrist_hash ^= piece_keys[BLACK_ROOK][H8];
            zobrist_hash ^= piece_keys[BLACK_ROOK][F8];
            material_pst_score -= pst[BLACK_ROOK][F8] - pst[BLACK_ROOK][H8];
            break;
        // q
        case C8:
            popBit(piece_bb[BLACK_ROOK], A8);
            setBit(piece_bb[BLACK_ROOK], D8);
            flipBit(occ[stm], A8);
            flipBit(occ[BOTH], A8);
            flipBit(occ[stm], D8);
            flipBit(occ[BOTH], D8);
            piece_board[A8] = NO_PIECE;
            piece_board[D8] = BLACK_ROOK;
            zobrist_hash ^= piece_keys[BLACK_ROOK][A8];
            zobrist_hash ^= piece_keys[BLACK_ROOK][D8];
            material_pst_score -= pst[BLACK_ROOK][D8] - pst[BLACK_ROOK][A8];
            break;
        default: // should never get here, removing warning
            break;
        }
    } else if (Capture(move)) {
        Square sq = IsEP(move) ? static_cast<Square>(static_cast<int>(to) - (stm == WHITE ? NORTH : SOUTH)) : to;

        if (IsEP(move)) {
            piece_board[sq] = NO_PIECE;
        }

        flipBit(piece_bb[captured], sq);
        flipBit(occ[xstm], sq);
        flipBit(occ[BOTH], sq);

        zobrist_hash ^= piece_keys[captured][sq];
        phase_score -= phase_weights[makeDefaultPiece(captured)];

        Score captured_val = material_values[makeDefaultPiece(captured)] + pst[captured][sq];
        if (stm == WHITE) {
            material_pst_score += captured_val; // removing black piece adds to score
        } else {
            material_pst_score -= captured_val; // removing white piece subtracts from score
        }

        if (captured == makePiece(PAWN, xstm)) {
            pawn_hash ^= piece_keys[makePiece(PAWN, xstm)][sq]; // Remove captured pawn
        }

        fmr = 0;
    }

    if (ep_square != NO_SQUARE) {
        zobrist_hash ^= ep_keys[ep_square];
        ep_square = NO_SQUARE;
    }

    if (castling) {
        zobrist_hash ^= castle_keys[castling];
        castling &= castling_rights[from];
        castling &= castling_rights[to];
        zobrist_hash ^= castle_keys[castling];
    }

    // Special pawn rules
    if (piece == makePiece(PAWN, stm)) {
        // Update STM pawn hash
        pawn_hash ^= piece_keys[piece][from] ^ piece_keys[piece][to];
        // Establish EP square if necessary
        if ((from ^ to) == 16) {
            Square new_ep_square = static_cast<Square>(to - (stm == WHITE ? NORTH : SOUTH));

            if (getPawnAttacks(new_ep_square, stm) & piece_bb[makePiece(PAWN, xstm)]) {
                ep_square = new_ep_square;
                zobrist_hash ^= ep_keys[new_ep_square];
            }
        } else if (Prom(move)) { // Promotion
            Piece prom_piece = makePiece(promPiece(move), stm);

            flipBit(piece_bb[piece], to);
            flipBit(piece_bb[prom_piece], to);

            piece_board[to] = static_cast<Piece>(prom_piece);
            zobrist_hash ^= piece_keys[prom_piece][to] ^ piece_keys[piece][to];
            phase_score += phase_weights[makeDefaultPiece(prom_piece)];

            Score prom_delta = (material_values[makeDefaultPiece(prom_piece)] + pst[prom_piece][to]) - (material_values[PAWN] + pst[piece][to]);
            if (stm == WHITE) {
                material_pst_score += prom_delta;
            } else {
                material_pst_score -= prom_delta;
            }

            pawn_hash ^= piece_keys[piece][to]; // Undo to hash- pawn no longer exists
        }

        fmr = 0;
    }

    zobrist_hash ^= piece_keys[piece][to];
    zobrist_hash ^= side_key;
    move_number += (stm == BLACK);
    std::swap(stm, xstm);

    setSpecials();

    assert(piece_bb[WHITE_KING] != 0ULL);
    assert(piece_bb[BLACK_KING] != 0ULL);
}

void Board::undoMove(Move move) {
    Square from = From(move);
    Square to = To(move);
    Piece piece = MovePiece(move);

    history_ply--;
    BoardHistory& hist_data = history[history_ply];
    castling = hist_data.castling;
    ep_square = hist_data.ep_square;
    fmr = hist_data.fmr;
    checkers = hist_data.checkers;
    legal_mask = hist_data.legal_mask;
    threatened_by[WHITE] = hist_data.threatened_by[WHITE];
    threatened_by[BLACK] = hist_data.threatened_by[BLACK];
    pinned = hist_data.pinned;
    zobrist_hash = hist_data.zobrist_hash;
    pawn_hash = hist_data.pawn_hash;
    material_pst_score = hist_data.material_pst_score;
    memcpy(&eval_info, &hist_data.eval_info, sizeof(EvalInfo));
    memcpy(&accumulator, &hist_data.accumulator, sizeof(Accumulator));

    std::swap(stm, xstm);
    move_number -= (stm == BLACK);

    if (Prom(move)) {
        Piece prom_piece = makePiece(promPiece(move), stm);

        flipBit(piece_bb[piece], to);
        flipBit(piece_bb[prom_piece], to);

        piece_board[to] = piece;
        phase_score -= phase_weights[makeDefaultPiece(prom_piece)];
    }

    flipBits(piece_bb[piece], to, from);
    flipBits(occ[stm], to, from);
    flipBits(occ[BOTH], to, from);

    piece_board[from] = piece;
    piece_board[to] = NO_PIECE;

    if (Castle(move)) {
        switch (to) {
        // White Kingside (K) - Rook was on F1, move back to H1
        case G1:
            popBit(piece_bb[WHITE_ROOK], F1);
            setBit(piece_bb[WHITE_ROOK], H1);
            flipBit(occ[stm], F1);
            flipBit(occ[BOTH], F1);
            flipBit(occ[stm], H1);
            flipBit(occ[BOTH], H1);
            piece_board[F1] = NO_PIECE;
            piece_board[H1] = WHITE_ROOK;
            break;

        // White Queenside (Q) - Rook was on D1, move back to A1
        case C1:
            popBit(piece_bb[WHITE_ROOK], D1);
            setBit(piece_bb[WHITE_ROOK], A1);
            flipBit(occ[stm], D1);
            flipBit(occ[BOTH], D1);
            flipBit(occ[stm], A1);
            flipBit(occ[BOTH], A1);
            piece_board[D1] = NO_PIECE;
            piece_board[A1] = WHITE_ROOK;
            break;

        // Black Kingside (k) - Rook was on F8, move back to H8
        case G8:
            popBit(piece_bb[BLACK_ROOK], F8);
            setBit(piece_bb[BLACK_ROOK], H8);
            flipBit(occ[stm], F8);
            flipBit(occ[BOTH], F8);
            flipBit(occ[stm], H8);
            flipBit(occ[BOTH], H8);
            piece_board[F8] = NO_PIECE;
            piece_board[H8] = BLACK_ROOK;
            break;

        // Black Queenside (q) - Rook was on D8, move back to A8
        case C8:
            popBit(piece_bb[BLACK_ROOK], D8);
            setBit(piece_bb[BLACK_ROOK], A8);
            flipBit(occ[stm], D8);
            flipBit(occ[BOTH], D8);
            flipBit(occ[stm], A8);
            flipBit(occ[BOTH], A8);
            piece_board[D8] = NO_PIECE;
            piece_board[A8] = BLACK_ROOK;
            break;
        default: // should never get here
            break;
        }
    } else if (Capture(move)) {
        Square sq = IsEP(move) ? static_cast<Square>(static_cast<int>(to) - (stm == WHITE ? NORTH : SOUTH)) : to;
        Piece captured = hist_data.captured_piece;

        flipBit(piece_bb[captured], sq);
        flipBit(occ[xstm], sq);
        flipBit(occ[BOTH], sq);

        piece_board[sq] = captured;
        phase_score += phase_weights[makeDefaultPiece(captured)];
    }
}

void Board::makeNullMove() {
    history[history_ply++] = BoardHistory(
        castling, ep_square, fmr, NO_PIECE, checkers, legal_mask, threatened_by[WHITE], threatened_by[BLACK], pinned, zobrist_hash, pawn_hash, material_pst_score,
        eval_info, accumulator
    );
    if (ep_square != NO_SQUARE) {
        zobrist_hash ^= ep_keys[ep_square];
        ep_square = NO_SQUARE;
    }
    std::swap(stm, xstm);
    zobrist_hash ^= side_key;
    setSpecials();
}

void Board::undoNullMove() {
    history_ply--;
    const BoardHistory& hist_data = history[history_ply];
    castling = hist_data.castling;
    ep_square = hist_data.ep_square;
    fmr = hist_data.fmr;
    checkers = hist_data.checkers;
    legal_mask = hist_data.legal_mask;
    threatened_by[WHITE] = hist_data.threatened_by[WHITE];
    threatened_by[BLACK] = hist_data.threatened_by[BLACK];
    pinned = hist_data.pinned;
    zobrist_hash = hist_data.zobrist_hash;
    pawn_hash = hist_data.pawn_hash;
    material_pst_score = hist_data.material_pst_score;
    memcpy(&eval_info, &hist_data.eval_info, sizeof(EvalInfo));
    memcpy(&accumulator, &hist_data.accumulator, sizeof(Accumulator));
    std::swap(stm, xstm);
}

void Board::refreshZobrist() {
    zobrist_hash = 0ULL;
    pawn_hash = 0ULL;
    BitBoard bb;

    for (uint8_t pc = static_cast<uint8_t>(WHITE_PAWN); pc <= static_cast<uint8_t>(BLACK_KING); pc++) {
        bb = piece_bb[pc];
        while (bb) // This can in theory be a for loop. Maybe it looks nicer, maybe it doesn't?
        {
            uint8_t sq = popLSB(bb);
            zobrist_hash ^= piece_keys[pc][sq];
            if (makeDefaultPiece(static_cast<Piece>(pc)) == PAWN) {
                pawn_hash ^= piece_keys[pc][sq];
            }
        }
    }

    if (ep_square != NO_SQUARE) {
        zobrist_hash ^= ep_keys[ep_square];
    }

    zobrist_hash ^= castle_keys[castling];

    if (stm == BLACK) {
        zobrist_hash ^= side_key;
    }
}

void Board::setSpecials() {
    Square king_square = static_cast<Square>(getLSB(piece_bb[makePiece(KING, stm)]));

    pinned = BitBoard(0);
    checkers = (getKnightAttacks(king_square) & piece_bb[makePiece(KNIGHT, xstm)]);
    checkers |= (getPawnAttacks(king_square, stm) & piece_bb[makePiece(PAWN, xstm)]);

    BitBoard sliders = (piece_bb[makePiece(BISHOP, xstm)] | piece_bb[makePiece(QUEEN, xstm)]) & getBishopAttacks(king_square, BitBoard(0));
    sliders |= (piece_bb[makePiece(ROOK, xstm)] | piece_bb[makePiece(QUEEN, xstm)]) & getRookAttacks(king_square, BitBoard(0));

    while (sliders) {
        Square sq = static_cast<Square>(popLSB(sliders));
        BitBoard blockers = between_squares[king_square][sq] & occ[BOTH];

        if (!blockers) {
            setBit(checkers, sq);
        } else if (!multipleActiveBits(blockers)) {
            pinned |= blockers & occ[stm];
        }
    }
    if (checkers == 0) {
        legal_mask = ~BitBoard(0);
    } else { // num checkers == 1 only can be changed later if needed
        Square checker_sq = static_cast<Square>(getLSB(checkers));
        legal_mask = checkers;
        Piece checker_piece = pieceAt(static_cast<uint8_t>(checker_sq));
        DefaultPiece checker_type = makeDefaultPiece(checker_piece);
        if (checker_type == BISHOP || checker_type == ROOK || checker_type == QUEEN) {
            legal_mask |= between_squares[king_square][checker_sq];
        }
    }
    setThreatened();
}

void Board::setPhase() {
    phase_score = 0;
    phase_score += phase_weights[PAWN] * bitCount(piece_bb[WHITE_PAWN] | piece_bb[BLACK_PAWN]);
    phase_score += phase_weights[KNIGHT] * bitCount(piece_bb[WHITE_KNIGHT] | piece_bb[BLACK_KNIGHT]);
    phase_score += phase_weights[BISHOP] * bitCount(piece_bb[WHITE_BISHOP] | piece_bb[BLACK_BISHOP]);
    phase_score += phase_weights[ROOK] * bitCount(piece_bb[WHITE_ROOK] | piece_bb[BLACK_ROOK]);
    phase_score += phase_weights[QUEEN] * bitCount(piece_bb[WHITE_QUEEN] | piece_bb[BLACK_QUEEN]);
}

void Board::refreshMaterialPST() {
    material_pst_score = Score();
    for (uint8_t pc = WHITE_PAWN; pc <= BLACK_KING; pc++) {
        BitBoard bb = piece_bb[pc];
        Score piece_mat = material_values[makeDefaultPiece(static_cast<Piece>(pc))];
        bool is_white = (pc <= WHITE_KING);
        while (bb) {
            uint8_t sq = popLSB(bb);
            Score val = piece_mat + pst[pc][sq];
            if (is_white) {
                material_pst_score += val;
            } else {
                material_pst_score -= val;
            }
        }
    }
}

void Board::setThreatened() {
    threatened_by[WHITE] = BitBoard(0);
    threatened_by[BLACK] = BitBoard(0);
    eval_info.pawn_attacks[WHITE] = shiftPawnAttacks(piece_bb[WHITE_PAWN], WHITE);
    eval_info.pawn_attacks[BLACK] = shiftPawnAttacks(piece_bb[BLACK_PAWN], BLACK);
    threatened_by[WHITE] |= eval_info.pawn_attacks[WHITE];
    threatened_by[BLACK] |= eval_info.pawn_attacks[BLACK];
    for (int white_index = WHITE_PAWN + 1, black_index = BLACK_PAWN + 1; white_index <= WHITE_KING; white_index++, black_index++) {
        BitBoard cur_piece_bb = piece_bb[white_index];
        while (cur_piece_bb) {
            Square cur_square = static_cast<Square>(popLSB(cur_piece_bb));
            const BitBoard attacks = getPieceAttacks(static_cast<Piece>(white_index), cur_square, occ[BOTH]);
            threatened_by[WHITE] |= attacks;
            eval_info.piece_attacks[cur_square] = attacks;
        }
        cur_piece_bb = piece_bb[black_index];
        while (cur_piece_bb) {
            Square cur_square = static_cast<Square>(popLSB(cur_piece_bb));
            const BitBoard attacks = getPieceAttacks(static_cast<Piece>(black_index), cur_square, occ[BOTH]);
            threatened_by[BLACK] |= attacks;
            eval_info.piece_attacks[cur_square] = attacks;
        }
    }
}
