#include "board.h"
#include <sstream>
#include <cctype>

Board::Board() {
    clear();
    history.reserve(MAX_PLY);

    // Pawns
    piece_bb[WHITE_PAWN] = 0x000000000000FF00ULL;
    piece_bb[BLACK_PAWN] = 0x00FF000000000000ULL;

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

    setOcc();
}

Board::Board(const std::string& fen) {
    history.reserve(MAX_PLY);
    loadFEN(fen);
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

void Board::printBoard() const {
    for (int8_t i = 63; i >= 0; i--) {
        switch (pieceAt(i)) {
        case WHITE_PAWN: std::cout << 'P'; break;
        case WHITE_KNIGHT: std::cout << 'N'; break;
        case WHITE_BISHOP: std::cout << 'B'; break;
        case WHITE_ROOK: std::cout << 'R'; break;
        case WHITE_QUEEN: std::cout << 'Q'; break;
        case WHITE_KING: std::cout << 'K'; break;
        case BLACK_PAWN: std::cout << 'p'; break;
        case BLACK_KNIGHT: std::cout << 'n'; break;
        case BLACK_BISHOP: std::cout << 'b'; break;
        case BLACK_ROOK: std::cout << 'r'; break;
        case BLACK_QUEEN: std::cout << 'q'; break;
        case BLACK_KING: std::cout << 'k'; break;
        case NO_PIECE: std::cout << '.'; break;
        }

        if (i % 8 == 0)
            std::cout << '\n';
    }
    std::cout << std::endl;
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
    int rank = 7;
    int file = 0;
    for (char c : placement) {
        if (c == '/') {
            --rank;
            file = 0;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            file += c - '0';
            continue;
        }

        if (file >= 8 || rank < 0) {
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
        int r = epStr[1] - '1';
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
    return true;
}