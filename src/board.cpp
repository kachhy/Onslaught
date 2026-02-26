#include "board.h"
#include <sstream>
#include <cctype>

DefaultPiece promPiece(Move move) {
    if (Flags(move) & QUEEN_PROMO_FLAG) {
        return KNIGHT;
    }
    if (Flags(move) & KNIGHT_PROMO_FLAG) {
        return KNIGHT;
    }
    if (Flags(move) & ROOK_PROMO_FLAG) {
        return ROOK;
    }
    return BISHOP;
}

Board::Board() {
    clear();
    history.reserve(MAX_PLY);

    // Pawns
    piece_bb[WHITE_PAWN] = 0x00FF000000000000ULL;
    piece_bb[BLACK_PAWN] = 0x000000000000FF00ULL;

    // White Knights
    setBit(piece_bb[WHITE_KNIGHT], B1);
    setBit(piece_bb[WHITE_KNIGHT], G1);

    // White Bishops
    setBit(piece_bb[WHITE_BISHOP], C1);
    setBit(piece_bb[WHITE_BISHOP], F1);

    // White Rooks
    setBit(piece_bb[WHITE_ROOK], A1);
    setBit(piece_bb[WHITE_ROOK], H1);

    // White Royals
    setBit(piece_bb[WHITE_QUEEN], D1);
    setBit(piece_bb[WHITE_KING],  E1);

    // Black Knights
    setBit(piece_bb[BLACK_KNIGHT], B8);
    setBit(piece_bb[BLACK_KNIGHT], G8);

    // Black Bishops
    setBit(piece_bb[BLACK_BISHOP], C8);
    setBit(piece_bb[BLACK_BISHOP], F8);

    // Black Rooks
    setBit(piece_bb[BLACK_ROOK], A8);
    setBit(piece_bb[BLACK_ROOK], H8);

    // Black Royals
    setBit(piece_bb[BLACK_QUEEN], D8);
    setBit(piece_bb[BLACK_KING],  E8);

    // EP squares and Castling rights
    ep_square = NO_SQUARE;
    castling  = 0xF;

    // Counters
    move_number = 1;
    fmr         = 0;

    setOcc();
    setPieceBoard();
}

Board::Board(const std::string& fen) {
    history.reserve(MAX_PLY);
    loadFEN(fen);
}

void Board::setPieceBoard() {
    memset(piece_board, 0, sizeof(piece_board)); // TODO: unclear if we need this -> test

    for (uint8_t i = 0; i < 64; i++) {
        piece_board[i] = pieceAt(i);
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

Piece Board::pieceAt(uint8_t sq) const {
    if (!getBit(occ[BOTH], sq))
        return NO_PIECE;

    for (uint8_t p = WHITE_PAWN; p <= BLACK_KING; p++) {
        if (getBit(piece_bb[p], sq)) {
            return static_cast<Piece>(p);
        }
    }

    return NO_PIECE; // Exceptional state
}

std::string Board::getCastlingString() const {
    std::string out = "";
    if (castling & 8) {
        out += 'K';
    }
    else {
        out += '-';
    }

    if (castling & 4) {
        out += 'Q';
    }
    else {
        out += '-';
    }

    if (castling & 2) {
        out += 'k';
    }
    else {
        out += '-';
    }

    if (castling & 1) {
        out += 'q';
    }
    else {
        out += '-';
    }

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
              << "\nEP Square: " << board_coords[ep_square] 
              << "\nCastling: " << getCastlingString()
              << "\n" << std::endl;
}

void Board::clear() {
    memset(piece_bb, 0, sizeof(piece_bb));
    memset(occ, 0, sizeof(occ));
    history.clear();

    // Turns
    stm  = WHITE;
    xstm = BLACK;

    // Counters
    move_number = 1;
    fmr         = 0;

    // EP squares and Castling rights
    ep_square = NO_SQUARE;
    castling  = 0xF;

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

bool Board::loadFEN(const std::string &fen) {
    clear();

    std::istringstream iss(fen);
    std::string placement, active, castlingStr, epStr, halfmoveStr, fullmoveStr;

    if (!(iss >> placement >> active >> castlingStr >> epStr)) {
        return false;
    }

    iss >> halfmoveStr;
    iss >> fullmoveStr;

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
        default:
            return false;
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
    if (castlingStr != "-") {
        for (char ch : castlingStr) {
            switch (ch) {
            case 'K': castling |= 1; break;
            case 'Q': castling |= 2; break;
            case 'k': castling |= 4; break;
            case 'q': castling |= 8; break;
            default: break;
            }
        }
    }

    // En-passant
    if (epStr == "-") {
        ep_square = NO_SQUARE;
    } else if (epStr.size() == 2 && epStr[0] >= 'a' && epStr[0] <= 'h' && epStr[1] >= '1' && epStr[1] <= '8') {
        int f = epStr[0] - 'a';
        int r = '8' - epStr[1];
        ep_square = static_cast<Square>(r * 8 + f);
    } else {
        return false;
    }

    // Halfmove clock
    if (!halfmoveStr.empty()) {
        try {
            fmr = static_cast<uint8_t>(std::stoi(halfmoveStr));
        } catch (...) {
            fmr = 0;
        }
    }

    // Fullmove number
    if (!fullmoveStr.empty()) {
        try {
            move_number = static_cast<uint32_t>(std::stoul(fullmoveStr));
        } catch (...) {
            move_number = 1;
        }
    }

    setOcc();
    setPieceBoard();
    return true;
}

BitBoard Board::getOcc(Side side) const {
    return occ[side];
}

void Board::makeMove(Move move) {
    Square from      = From(move);
    Square to        = To(move);
    Piece piece      = MovePiece(move);
    int captured     = IsEP(move) ? makePiece(PAWN, xstm) : piece_board[to];

    history.emplace_back(castling, ep_square, fmr);

    fmr++;
    flipBits(piece_bb[piece], from, to);
    flipBits(occ[stm], from, to);
    flipBits(occ[BOTH], from, to);

    piece_board[from] = NO_PIECE;
    piece_board[to]   = piece;

    if (Castle(move)) {
        std::cout << "castle\n";
        switch (to)
        {
            // K
            case G1:
                // move H rook
                popBit(piece_bb[WHITE_ROOK], H1);
                setBit(piece_bb[WHITE_ROOK], F1);
                flipBit(occ[stm], H1);
                flipBit(occ[BOTH], H1);
                flipBit(occ[stm], F1);
                flipBit(occ[BOTH], F1);
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
                break;
            // k
            case G8:
                popBit(piece_bb[BLACK_ROOK], H8);
                setBit(piece_bb[BLACK_ROOK], F8);
                flipBit(occ[stm], H8);
                flipBit(occ[BOTH], H8);
                flipBit(occ[stm], F8);
                flipBit(occ[BOTH], F8);
                break;
            // q
            case C8:
                popBit(piece_bb[BLACK_ROOK], A8);
                setBit(piece_bb[BLACK_ROOK], D8);
                flipBit(occ[stm], A8);
                flipBit(occ[BOTH], A8);
                flipBit(occ[stm], D8);
                flipBit(occ[BOTH], D8);
                break;
        }
    }
    else if (Capture(move)) {
        std::cout << "capture\n";
        Square sq = IsEP(move) ? static_cast<Square>(static_cast<int>(to) - (stm == WHITE ? NORTH : SOUTH)) : to;

        if (IsEP(move)) {
            piece_board[sq] = NO_PIECE;
        }

        flipBit(piece_bb[captured], sq);
        flipBit(occ[xstm], sq);
        flipBit(occ[BOTH], sq);

        fmr = 0;
    }

    ep_square = NO_SQUARE;

    if (castling) {
        castling &= castling_rights[from];
        castling &= castling_rights[to];
    }

    // Special pawn rules
    if (piece == makePiece(PAWN, stm)) {
        // Establish EP square if necessary
        if ((from ^ to) == 16) {
            Square new_ep_square = static_cast<Square>(to - (stm == WHITE ? NORTH : SOUTH));

            if (getPawnAttacks(new_ep_square, stm) & piece_bb[makePiece(PAWN, xstm)]) {
                ep_square = new_ep_square;
            }
        }
        else if (Prom(move)) { // Promotion
            int prom_piece = makePiece(promPiece(move), stm);
            
            flipBit(piece_bb[piece], to);
            flipBit(piece_bb[prom_piece], to);

            piece_board[to] = static_cast<Piece>(prom_piece);
        }

        fmr = 0;
    }

    move_number++;
    xstm = stm;
    stm = xstm == WHITE ? BLACK : WHITE; // optimizable
}

void Board::undoMove(Move move) {
}