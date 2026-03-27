#include "tuning.h"
#include <iostream>

Tuner::Tuner(const size_t dataset_size) {
    dataset.reserve(dataset_size);
    traces.reserve(dataset_size);
    std::cout << "[Tune] Initializing parameters" << std::endl;
    initParams();
}

void Tuner::loadDataset(const std::string& filename, const uint32_t max) {
    std::ifstream in(filename);
    std::string line;
    std::cout << "[Tune] Loading positions..." << std::endl;
    while (std::getline(in, line)) {
        const size_t index = line.find("[");
        const std::string res = line.substr(index + 1, 3);
        Side winner;
        if (res == "0.5") {
            winner = BOTH;
        } else if (res == "1.0") {
            winner = WHITE;
        } else {
            winner = BLACK;
        }
        dataset.emplace_back(line.substr(0, index), winner);
        if (!(dataset.size() % 100000)) {
            std::cout << "\tLoaded " << dataset.size() << " positions." << std::endl;

            if (dataset.size() >= max) {
                break;
            }
        }
    }
    std::cout << "[Tune] Preprocessing evaluation traces..." << std::endl;
    for (const Position& pos : dataset) {
        trace = {};
        eval(pos.board);
        trace.phase = pos.board.phase();
        trace.result = pos.result;
        traces.emplace_back(trace);
    }
}

void Tuner::initParams() {
    params.clear();

    // Material [6 pieces x 2 (mg/eg)]
    for (int p = 0; p < 6; p++) {
        params.push_back({ (double)MG(material_values[p]) });
        params.push_back({ (double)EG(material_values[p]) });
    }

    // Tempo
    // Index is + 12
    params.push_back({ (double)MG(TEMPO) });
    params.push_back({ (double)EG(TEMPO) });

    // Mobility [5 buckets x 2]
    // Index is + 14
    for (int i = 0; i < 5; i++) {
        params.push_back({ (double)MG(MOBILITY[i]) });
        params.push_back({ (double)EG(MOBILITY[i]) });
    }

    // Pawn structure
    // Index is + 24
    params.push_back({ (double)MG(PAWN_PHALANX) });
    params.push_back({ (double)EG(PAWN_PHALANX) });

    // Index is + 26
    params.push_back({ (double)MG(DOUBLED_PAWNS) });
    params.push_back({ (double)EG(DOUBLED_PAWNS) });

    // Index is + 28
    params.push_back({ (double)MG(BACKWARDS_PAWN) });
    params.push_back({ (double)EG(BACKWARDS_PAWN) });

    // Pawn protection [6 pieces x 2]
    // Index is + 30
    for (int p = 0; p < 6; p++) {
        params.push_back({ (double)MG(PAWN_PROTECTION[p]) });
        params.push_back({ (double)EG(PAWN_PROTECTION[p]) });
    }

    // Index is + 42
    // Passed pawns [8 ranks x 2]
    for (int r = 0; r < 8; r++) {
        params.push_back({ (double)MG(PASSED_PAWNS[r]) });
        params.push_back({ (double)EG(PASSED_PAWNS[r]) });
    }

    // Index is + 58
    // Knight
    params.push_back({ (double)MG(KNIGHT_OUTPOST) });
    params.push_back({ (double)EG(KNIGHT_OUTPOST) });

    // Index is + 60
    params.push_back({ (double)MG(KNIGHT_BEHIND_PAWN) });
    params.push_back({ (double)EG(KNIGHT_BEHIND_PAWN) });

    // Knight pawn adj [9 x 2]
    // Index is + 62
    for (int i = 0; i < 9; i++) {
        params.push_back({ (double)MG(KNIGHT_PAWN_ADJ[i]) });
        params.push_back({ (double)EG(KNIGHT_PAWN_ADJ[i]) });
    }

    // Bishop
    // Index is + 80
    params.push_back({ (double)MG(BISHOP_PAIR) });
    params.push_back({ (double)EG(BISHOP_PAIR) });

    // Index is + 82
    params.push_back({ (double)MG(BISHOP_CONTROL_PENALTY) });
    params.push_back({ (double)EG(BISHOP_CONTROL_PENALTY) });

    // Index is + 84
    params.push_back({ (double)MG(BAD_BISHOP) });
    params.push_back({ (double)EG(BAD_BISHOP) });

    // Index is + 86
    params.push_back({ (double)MG(BISHOP_BLOCKING_PAWN) });
    params.push_back({ (double)EG(BISHOP_BLOCKING_PAWN) });

    // Index is + 88
    params.push_back({ (double)MG(TRAPPED_BISHOP) });
    params.push_back({ (double)EG(TRAPPED_BISHOP) });

    // Rook
    // Index is + 90
    params.push_back({ (double)MG(ROOK_ON_SEVENTH_RANK) });
    params.push_back({ (double)EG(ROOK_ON_SEVENTH_RANK) });

    // Index is + 92
    params.push_back({ (double)MG(ROOK_ON_OPEN_FILE) });
    params.push_back({ (double)EG(ROOK_ON_OPEN_FILE) });

    // Index is + 94
    params.push_back({ (double)MG(ROOK_ON_SEMI_OPEN_FILE) });
    params.push_back({ (double)EG(ROOK_ON_SEMI_OPEN_FILE) });

    // Rook pawn adj [9 x 2]
    // Index is + 96
    for (int i = 0; i < 9; i++) {
        params.push_back({ (double)MG(ROOK_PAWN_ADJ[i]) });
        params.push_back({ (double)EG(ROOK_PAWN_ADJ[i]) });
    }

    // Queen
    // Index is + 114
    params.push_back({ (double)MG(QUEEN_REL_PIN) });
    params.push_back({ (double)EG(QUEEN_REL_PIN) });

    // Index is + 116
    params.push_back({ (double)MG(NO_OPPONENT_QUEENS) });
    params.push_back({ (double)EG(NO_OPPONENT_QUEENS) });

    // King safety - file exposure
    // Index is + 118
    params.push_back({ (double)MG(KING_ON_OPEN_FILE) });
    params.push_back({ (double)EG(KING_ON_OPEN_FILE) });

    // Index is + 120
    params.push_back({ (double)MG(KING_ON_SEMI_OPEN_FILE) });
    params.push_back({ (double)EG(KING_ON_SEMI_OPEN_FILE) });

    // Pawn shield [4 buckets x 2]
    // Index is + 122
    for (int i = 0; i < 4; i++) {
        params.push_back({ (double)MG(PAWN_SHIELD[i]) });
        params.push_back({ (double)EG(PAWN_SHIELD[i]) });
    }

    // Pawn storm [3 buckets x 2]
    // Index is + 130
    for (int i = 0; i < 3; i++) {
        params.push_back({ (double)MG(PAWN_STORM[i]) });
        params.push_back({ (double)EG(PAWN_STORM[i]) });
    }

    // King zone attacks [4 piece types x 2]
    // Index is + 136
    for (int i = 0; i < 4; i++) {
        params.push_back({ (double)MG(KING_ZONE_ATTACK[i]) });
        params.push_back({ (double)EG(KING_ZONE_ATTACK[i]) });
    }

    // Index is + 144
    params.push_back({ (double)MG(KING_ZONE_WEAK_SQUARE) });
    params.push_back({ (double)EG(KING_ZONE_WEAK_SQUARE) });

    // Index is + 146
    params.push_back({ (double)MG(KING_ZONE_WEAK_SQUARE_EXTENDED) });
    params.push_back({ (double)EG(KING_ZONE_WEAK_SQUARE_EXTENDED) });

    // King castling state [2 buckets x 2]
    // Index is + 148
    for (int i = 0; i < 2; i++) {
        params.push_back({ (double)MG(KING_CASTLED[i]) });
        params.push_back({ (double)EG(KING_CASTLED[i]) });
    }

    // Index is + 152
    params.push_back({ (double)MG(KING_LOST_ONE_CASTLING_RIGHT) });
    params.push_back({ (double)EG(KING_LOST_ONE_CASTLING_RIGHT) });

    // Index is + 154
    params.push_back({ (double)MG(KING_UNCASTLED_RIGHTS_REMAIN) });
    params.push_back({ (double)EG(KING_UNCASTLED_RIGHTS_REMAIN) });

    // PST [12 piece-colors x 64 squares x 2]
    // Index is + 156
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) {
            params.push_back({ (double)MG(pst[p][sq]) });
            params.push_back({ (double)EG(pst[p][sq]) });
        }
    }
}