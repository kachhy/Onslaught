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

double Tuner::reconstructScore(const Trace& tr) const {
    double mg = 0.0, eg = 0.0;
    double phase = tr.phase / 24.0;

    // Material
    for (int p = 0; p < 6; p++) {
        int coeff = tr.material[p][WHITE] - tr.material[p][BLACK];
        mg += coeff * params[MATERIAL_OFFSET].value;
        eg += coeff * params[MATERIAL_OFFSET + 1].value;
    }

    // Tempo
    int bp = tr.tempo[WHITE] - tr.tempo[BLACK];
    mg += bp * params[TEMPO_OFFSET].value;
    eg += bp * params[TEMPO_OFFSET + 1].value;

    // Mobility
    for (int i = 0; i < 5; i++) {
        int coeff = tr.mobility[i][WHITE] - tr.mobility[i][BLACK];
        mg += coeff * params[MOBILITY_OFFSET + i * 2].value;
        eg += coeff * params[MOBILITY_OFFSET + i * 2 + 1].value;
    }

    // Pawn structure
    // Pawn phalanx
    int phal = tr.pawn_phalanx[WHITE] - tr.pawn_phalanx[BLACK];
    mg += phal * params[PAWN_PHALANX_OFFSET].value;
    eg += phal * params[PAWN_PHALANX_OFFSET + 1].value;

    // Doubled pawns
    int dp = tr.doubled_pawns[WHITE] - tr.doubled_pawns[BLACK];
    mg += dp * params[DOUBLED_PAWNS_OFFSET].value;
    eg += dp * params[DOUBLED_PAWNS_OFFSET + 1].value;

    // Backwards pawn
    int backwards = tr.backwards_pawn[WHITE] - tr.backwards_pawn[BLACK];
    mg += backwards * params[BACKWARDS_PAWN_OFFSET].value;
    eg += backwards * params[BACKWARDS_PAWN_OFFSET + 1].value;

    // Pawn protection
    for (int p = 0; p < 6; p++) {
        int coeff = tr.pawn_protection[p][WHITE] - tr.pawn_protection[p][BLACK];
        mg += coeff * params[PAWN_PROTECTION_OFFSET + 2 * p].value;
        eg += coeff * params[PAWN_PROTECTION_OFFSET + 2 * p + 1].value;
    }

    // Passed pawns
    for (int r = 0; r < 8; r++) {
        int coeff = tr.passed_pawns[r][WHITE] - tr.passed_pawns[r][BLACK];
        mg += coeff * params[PASSED_PAWNS_OFFSET + 2 * r].value;
        eg += coeff * params[PASSED_PAWNS_OFFSET + 2 * r + 1].value;
    }

    // Knights
    // Knight outpost
    int outpost = tr.knight_outpost[WHITE] - tr.knight_outpost[BLACK];
    mg += outpost * params[KNIGHT_OUTPOST_OFFSET].value;
    eg += outpost * params[KNIGHT_OUTPOST_OFFSET + 1].value;

    // Knight behind pawn
    int behind_pawn = tr.knight_behind_pawn[WHITE] - tr.knight_behind_pawn[BLACK];
    mg += behind_pawn * params[KNIGHT_BEHIND_PAWN_OFFSET].value;
    eg += behind_pawn * params[KNIGHT_BEHIND_PAWN_OFFSET + 1].value;

    // Knight pawn adjustments
    for (int i = 0; i < 9; i++) {
        int coeff = tr.knight_pawn_adj[i][WHITE] - tr.knight_pawn_adj[i][BLACK];
        mg += coeff * params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i].value;
        eg += coeff * params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i + 1].value;
    }

    // Bishops
    // Bishop pair
    int bishop_pair = tr.bishop_pair[WHITE] - tr.bishop_pair[BLACK];
    mg += bishop_pair * params[BISHOP_PAIR_OFFSET].value;
    eg += bishop_pair * params[BISHOP_PAIR_OFFSET + 1].value;

    // Bishop control penalty
    int bcp = tr.bishop_control_penalty[WHITE] - tr.bishop_control_penalty[BLACK];
    mg += bcp * params[BISHOP_CTRL_PENALTY_OFFSET].value;
    eg += bcp * params[BISHOP_CTRL_PENALTY_OFFSET + 1].value;

    // Bad bishop
    int bad_bishop = tr.bad_bishop[WHITE] - tr.bad_bishop[BLACK];
    mg += bad_bishop * params[BAD_BISHOP_OFFSET].value;
    eg += bad_bishop * params[BAD_BISHOP_OFFSET + 1].value;

    // Bishop blocking pawn
    int bbp = tr.bishop_blocking_pawn[WHITE] - tr.bishop_blocking_pawn[BLACK];
    mg += bbp * params[BISHOP_BLOCKING_PAWN_OFFSET].value;
    eg += bbp * params[BISHOP_BLOCKING_PAWN_OFFSET + 1].value;

    // Trapped bishop
    int trapped_bishop = tr.trapped_bishop[WHITE] - tr.trapped_bishop[BLACK];
    mg += trapped_bishop * params[TRAPPED_BISHOP_OFFSET].value;
    eg += trapped_bishop * params[TRAPPED_BISHOP_OFFSET + 1].value;

    // Rooks
    // Rook on seventh
    int rook_seventh = tr.rook_on_seventh_rank[WHITE] - tr.rook_on_seventh_rank[BLACK];
    mg += rook_seventh * params[ROOK_SEVENTH_OFFSET].value;
    eg += rook_seventh * params[ROOK_SEVENTH_OFFSET + 1].value;

    // Rook on open file
    int rook_open = tr.rook_on_open_file[WHITE] - tr.rook_on_open_file[BLACK];
    mg += rook_open * params[ROOK_OPEN_FILE_OFFSET].value;
    eg += rook_open * params[ROOK_OPEN_FILE_OFFSET + 1].value;

    // Rook on semi open file
    int rook_semi = tr.rook_on_semi_open_file[WHITE] - tr.rook_on_semi_open_file[BLACK];
    mg += rook_semi * params[ROOK_SEMI_OPEN_FILE_OFFSET].value;
    eg += rook_semi * params[ROOK_SEMI_OPEN_FILE_OFFSET + 1].value;

    // Rook pawn adjustments
    for (int i = 0; i < 9; i++) {
        int coeff = tr.rook_pawn_adj[i][WHITE] - tr.rook_pawn_adj[i][BLACK];
        mg += coeff * params[ROOK_PAWN_ADJ_OFFSET + 2 * i].value;
        eg += coeff * params[ROOK_PAWN_ADJ_OFFSET + 2 * i + 1].value;
    }

    // Queen
    // Queen rel pin
    int qrp = tr.queen_rel_pin[WHITE] - tr.queen_rel_pin[BLACK];
    mg += qrp * params[QUEEN_REL_PIN_OFFSET].value;
    eg += qrp * params[QUEEN_REL_PIN_OFFSET + 1].value;

    // No opponent queens
    int noq = tr.no_opponent_queens[WHITE] - tr.no_opponent_queens[BLACK];
    mg += noq * params[NO_OPPONENT_QUEENS_OFFSET].value;
    eg += noq * params[NO_OPPONENT_QUEENS_OFFSET + 1].value;

    // King
    // King open file
    int k_open = tr.king_on_open_file[WHITE] - tr.king_on_open_file[BLACK];
    mg += k_open * params[KING_OPEN_FILE_OFFSET].value;
    eg += k_open * params[KING_OPEN_FILE_OFFSET + 1].value;

    // King semi open file
    int k_semi = tr.king_on_semi_open_file[WHITE] - tr.king_on_semi_open_file[BLACK];
    mg += k_semi * params[KING_SEMI_OPEN_FILE_OFFSET].value;
    eg += k_semi * params[KING_SEMI_OPEN_FILE_OFFSET + 1].value;

    // King pawn shield
    for (int i = 0; i < 4; i++) {
        int coeff = tr.pawn_shield[i][WHITE] - tr.pawn_shield[i][BLACK];
        mg += coeff * params[PAWN_SHIELD_OFFSET + 2 * i].value;
        eg += coeff * params[PAWN_SHIELD_OFFSET + 2 * i + 1].value;
    }

    // Pawn storm
    for (int i = 0; i < 3; i++) {
        int coeff = tr.pawn_storm[i][WHITE] - tr.pawn_storm[i][BLACK];
        mg += coeff * params[PAWN_STORM_OFFSET + 2 * i].value;
        eg += coeff * params[PAWN_STORM_OFFSET + 2 * i + 1].value;
    }

    // King zone attack
    for (int i = 0; i < 4; i++) {
        int coeff = tr.king_zone_attack[i][WHITE] - tr.king_zone_attack[i][BLACK];
        mg += coeff * params[KING_ZONE_ATTACK_OFFSET + 2 * i].value;
        eg += coeff * params[KING_ZONE_ATTACK_OFFSET + 2 * i + 1].value;
    }

    // King zone weak square
    int kzws = tr.king_zone_weak_square[WHITE] - tr.king_zone_weak_square[BLACK];
    mg += kzws * params[KING_ZONE_WEAK_SQ_OFFSET].value;
    eg += kzws * params[KING_ZONE_WEAK_SQ_OFFSET + 1].value;

    // King zone weak square extended
    int kzwse = tr.king_zone_weak_square_extended[WHITE] - tr.king_zone_weak_square_extended[BLACK];
    mg += kzwse * params[KING_ZONE_WEAK_EXT_OFFSET].value;
    eg += kzwse * params[KING_ZONE_WEAK_EXT_OFFSET + 1].value;

    // King castled
    for (int i = 0; i < 2; i++) {
        int coeff = tr.king_castled[i][WHITE] - tr.king_castled[i][BLACK];
        mg += coeff * params[KING_CASTLED_OFFSET + 2 * i].value;
        eg += coeff * params[KING_CASTLED_OFFSET + 2 * i + 1].value;
    }

    // King lost one CR
    int kl1cr = tr.king_lost_one_castling_right[WHITE] - tr.king_lost_one_castling_right[BLACK];
    mg += kl1cr * params[KING_LOST_CASTLE_OFFSET].value;
    eg += kl1cr * params[KING_LOST_CASTLE_OFFSET + 1].value;

    // King uncastled rights remain
    int kucr = tr.king_uncastled_rights_remain[WHITE] - tr.king_uncastled_rights_remain[BLACK];
    mg += kucr * params[KING_UNCASTLED_OFFSET].value;
    eg += kucr * params[KING_UNCASTLED_OFFSET + 1].value;

    // PST
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) {
            // white pieces positive, black pieces negative
            int coeff = (p < 6) ? tr.pst[p][sq] : -tr.pst[p][sq];
            mg += coeff * params[PST_OFFSET + (p * 128) + (sq * 2)].value;
            eg += coeff * params[PST_OFFSET + (p * 128) + (sq * 2) + 1].value;
        }
    }

    return phase * mg + (1.0 - phase) * eg;
}

double Tuner::computeError() const {
    double err = 0.0;
    for (const Trace& tr : traces) {
        double score = reconstructScore(tr);
        double sig = sigmoid(score);
        double diff = sig - tr.result;
        err += diff * diff;
    }
    return err / traces.size();
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