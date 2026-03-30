#include "tuning.h"
#include <chrono>
#include <iomanip>
#include <iostream>

static float sideToResult(const Side side) {
    switch (side) {
        case WHITE: return 1.0f;
        case BLACK: return 0.0f;
        default:    return 0.5f;  // draw / BOTH
    }
}

Tuner::Tuner(const size_t dataset_size) {
    dataset.reserve(dataset_size);
    traces.reserve(dataset_size);
}

void Tuner::loadDataset(const std::string& filename, const uint32_t max) {
    std::ifstream in(filename);
    std::string line;
    std::cout << "[Tune] Loading positions..." << std::endl;
    int w = 0, d = 0, b = 0;
    while (std::getline(in, line)) {
        const size_t index = line.find("[");
        const std::string res = line.substr(index + 1, 3);
        Side winner;
        if (res == "0.5") {
            winner = BOTH;
            d++;
        } else if (res == "1.0") {
            winner = WHITE;
            w++;
        } else {
            winner = BLACK;
            b++;
        }
        dataset.emplace_back(line.substr(0, index), winner);
        if (!(dataset.size() % 100000)) {
            std::cout << "\tLoaded " << dataset.size() << " positions." << std::endl;

            if (dataset.size() >= max) {
                break;
            }
        }
    }
    std::cout << "[Tune] White wins: " << w << ", Black wins: " << b << ", Draws: " << d << std::endl;
    std::cout << "[Tune] Preprocessing evaluation traces..." << std::endl;
    for (const Position& pos : dataset) {
        trace = {};
        eval(pos.board);
        trace.phase = pos.board.phase();
        trace.result = sideToResult(pos.result);
        traces.emplace_back(trace);
    }
}

void Tuner::run(const uint32_t epochs) {
    std::cout << "[Tune] Initializing parameters" << std::endl;
    initParams();
    std::cout << "[Tune] Finding optimal K value" << std::endl;
    findOptimalK();
    std::cout << "\tFound optimal K at " << K << std::endl;
    std::cout << "[Tune] Beginning tuning" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t epoch = 1; epoch <= epochs; epoch++) {
        computeGradients();
        updateAdam(epoch);

        if (epoch % 50 == 0) {
            const double error = computeError();
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            int etr = ((epochs - epoch) * duration.count() / 1000) / 50;
            std::cout << "\tEpoch " << std::setw(6) << epoch << " | Error: " << std::setw(7) << error << " | Time elapsed (this epoch): " << std::setw(5)
                      << duration.count() / 1000 << "s | ETR: " << std::setw(7) << etr << "s\n";
            start = stop;
        }
    }
}

double Tuner::computeError() const {
    double err = 0.0;
    for (const Trace& tr : traces) {
        double score = reconstructScore(tr);
        double sig = sigmoid(score, K);
        double diff = sig - tr.result;
        err += diff * diff;
    }
    return err / traces.size();
}

double Tuner::computeError(double k) const {
    double err = 0.0;
    for (const Trace& tr : traces) {
        double score = reconstructScore(tr);
        double sig = sigmoid(score, k);
        double diff = sig - tr.result;
        err += diff * diff;
    }
    return err / traces.size();
}

void Tuner::computeGradients() {
    for (TunerParam& p : params) {
        p.grad = 0.0;
    }

    for (const Trace& tr : traces) {
        double score = reconstructScore(tr);
        double sig = sigmoid(score, K);
        double base = 2.0 * (sig - tr.result) * sig * (1.0 - sig) * K / 400.0;
        double phase = tr.phase / 24.0;
        updateGradients(tr, base, phase);
    }

    // Normalize gradients
    for (TunerParam& p : params) {
        p.grad /= traces.size();
    }
}

void Tuner::findOptimalK() {
    double low = 0.0;
    double high = 5.0;
    double phi = (sqrt(5.0) - 1.0) / 2.0; // golden ratio
    double c = high - phi * (high - low);
    double d = low + phi * (high - low);
    while (fabs(high - low) > 1e-6) {
        if (computeError(c) < computeError(d)) {
            high = d;
        } else {
            low = c;
        }
        c = high - phi * (high - low);
        d = low + phi * (high - low);
    }
    K = (low + high) / 2.0;
}

void Tuner::updateAdam(const uint32_t epoch) { // TODO: Verify this is correct
    for (TunerParam& param : params) {
        param.m = BETA1 * param.m + (1.0 - BETA1) * param.grad;
        param.v = BETA2 * param.v + (1.0 - BETA2) * param.grad * param.grad;
        double m_h = param.m / (1.0 - pow(BETA1, epoch));
        double v_h = param.v / (1.0 - pow(BETA2, epoch));
        param.value -= LEARNING_RATE * m_h / (sqrt(v_h) + EPSILON);
    }
}

void Tuner::dumpParams(std::ofstream& out) const {
    std::cout << "[Tune] Dumping parameters" << std::endl;
    // Material
    out << "constexpr Score material_values[6] = {\n";
    for (int p = 0; p < 6; p++) {
        out << "\tS(" << static_cast<int>(std::round(params[MATERIAL_OFFSET + 2 * p].value)) << ", " << static_cast<int>(std::round(params[MATERIAL_OFFSET + 2 * p + 1].value)) << "),\n";
    }
    out << "};\n";

    // Tempo
    out << "constexpr Score TEMPO = S(" << static_cast<int>(std::round(params[TEMPO_OFFSET].value)) << ", " << static_cast<int>(std::round(params[TEMPO_OFFSET + 1].value)) << ");\n";

    // Mobility
    out << "constexpr Score MOBILITY[5] = {\n";
    for (int i = 0; i < 5; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[MOBILITY_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[MOBILITY_OFFSET + 2 * i + 1].value)) << "),\n";
    }
    out << "};\n";

    // Pawn structure
    // Pawn phalanx
    out << "\nconstexpr Score PAWN_PHALANX = S(" << static_cast<int>(std::round(params[PAWN_PHALANX_OFFSET].value)) << ", " << static_cast<int>(std::round(params[PAWN_PHALANX_OFFSET + 1].value))
        << ");\n";

    // Doubled pawns
    out << "constexpr Score DOUBLED_PAWNS = S(" << static_cast<int>(std::round(params[DOUBLED_PAWNS_OFFSET].value)) << ", "
        << static_cast<int>(params[DOUBLED_PAWNS_OFFSET + 1].value) << ");\n";

    // Backwards pawn
    out << "constexpr Score BACKWARDS_PAWN = S(" << static_cast<int>(std::round(params[BACKWARDS_PAWN_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[BACKWARDS_PAWN_OFFSET + 1].value)) << ");\n";

    // Pawn protection
    out << "constexpr Score PAWN_PROTECTION[6] = {\n";
    for (int p = 0; p < 6; p++) {
        out << "\tS(" << static_cast<int>(std::round(params[PAWN_PROTECTION_OFFSET + 2 * p].value)) << ", " << static_cast<int>(std::round(params[PAWN_PROTECTION_OFFSET + 2 * p + 1].value))
            << "),\n";
    }
    out << "};\n";

    // Passed pawns
    out << "constexpr Score PASSED_PAWNS[8] = {\n";
    for (int r = 0; r < 8; r++) {
        out << "\tS(" << static_cast<int>(std::round(params[PASSED_PAWNS_OFFSET + 2 * r].value)) << ", " << static_cast<int>(std::round(params[PASSED_PAWNS_OFFSET + 2 * r + 1].value)) << "),\n";
    }
    out << "};\n";

    // Knights
    // Knight outpost
    out << "\nconstexpr Score KNIGHT_OUTPOST = S(" << static_cast<int>(std::round(params[KNIGHT_OUTPOST_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[KNIGHT_OUTPOST_OFFSET + 1].value)) << ");\n";

    // Knight behind pawn
    out << "constexpr Score KNIGHT_BEHIND_PAWN = S(" << static_cast<int>(params[KNIGHT_BEHIND_PAWN_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[KNIGHT_BEHIND_PAWN_OFFSET + 1].value)) << ");\n";

    // Knight pawn adjustments
    out << "constexpr Score KNIGHT_PAWN_ADJ[9] = {\n";
    for (int i = 0; i < 9; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i + 1].value))
            << "),\n";
    }
    out << "};\n";

    // Bishops
    // Bishop pair
    out << "\nconstexpr Score BISHOP_PAIR = S(" << static_cast<int>(std::round(params[BISHOP_PAIR_OFFSET].value)) << ", " << static_cast<int>(std::round(params[BISHOP_PAIR_OFFSET + 1].value))
        << ");\n";

    // Bishop control penalty
    out << "constexpr Score BISHOP_CONTROL_PENALTY = S(" << static_cast<int>(std::round(params[BISHOP_CTRL_PENALTY_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[BISHOP_CTRL_PENALTY_OFFSET + 1].value)) << ");\n";

    // Bad bishop
    out << "constexpr Score BAD_BISHOP = S(" << static_cast<int>(std::round(params[BAD_BISHOP_OFFSET].value)) << ", " << static_cast<int>(std::round(params[BAD_BISHOP_OFFSET + 1].value))
        << ");\n";

    // Bishop blocking pawn
    out << "constexpr Score BISHOP_BLOCKING_PAWN = S(" << static_cast<int>(std::round(params[BISHOP_BLOCKING_PAWN_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[BISHOP_BLOCKING_PAWN_OFFSET + 1].value)) << ");\n";

    // Trapped bishop
    out << "constexpr Score TRAPPED_BISHOP = S(" << static_cast<int>(std::round(params[TRAPPED_BISHOP_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[TRAPPED_BISHOP_OFFSET + 1].value)) << ");\n";

    // Rooks
    // Rook on seventh
    out << "\nconstexpr Score ROOK_ON_SEVENTH_RANK = S(" << static_cast<int>(params[ROOK_SEVENTH_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[ROOK_SEVENTH_OFFSET + 1].value)) << ");\n";

    // Rook on open file
    out << "constexpr Score ROOK_ON_OPEN_FILE = S(" << static_cast<int>(params[ROOK_OPEN_FILE_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[ROOK_OPEN_FILE_OFFSET + 1].value)) << ");\n";

    // Rook on semi open file
    out << "constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(" << static_cast<int>(params[ROOK_SEMI_OPEN_FILE_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[ROOK_SEMI_OPEN_FILE_OFFSET + 1].value)) << ");\n";

    // Rook pawn adjustments
    out << "constexpr Score ROOK_PAWN_ADJ[9] = {\n";
    for (int i = 0; i < 9; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[ROOK_PAWN_ADJ_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[ROOK_PAWN_ADJ_OFFSET + 2 * i + 1].value))
            << "),\n";
    }
    out << "};\n";

    // Queen
    // Queen rel pin
    out << "\nconstexpr Score QUEEN_REL_PIN = S(" << static_cast<int>(std::round(params[QUEEN_REL_PIN_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[QUEEN_REL_PIN_OFFSET + 1].value)) << ");\n";

    // No opponent queens
    out << "constexpr Score NO_OPPONENT_QUEENS = S(" << static_cast<int>(params[NO_OPPONENT_QUEENS_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[NO_OPPONENT_QUEENS_OFFSET + 1].value)) << ");\n";

    // King
    // King open file
    out << "\nconstexpr Score KING_ON_OPEN_FILE = S(" << static_cast<int>(std::round(params[KING_OPEN_FILE_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[KING_OPEN_FILE_OFFSET + 1].value)) << ");\n";

    // King semi open file
    out << "constexpr Score KING_ON_SEMI_OPEN_FILE = S(" << static_cast<int>(params[KING_SEMI_OPEN_FILE_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[KING_SEMI_OPEN_FILE_OFFSET + 1].value)) << ");\n";

    // King pawn shield
    out << "constexpr Score PAWN_SHIELD[4] = {\n";
    for (int i = 0; i < 4; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[PAWN_SHIELD_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[PAWN_SHIELD_OFFSET + 2 * i + 1].value)) << "),\n";
    }
    out << "};\n";

    // Pawn storm
    out << "constexpr Score PAWN_STORM[3] = {\n";
    for (int i = 0; i < 3; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[PAWN_STORM_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[PAWN_STORM_OFFSET + 2 * i + 1].value)) << "),\n";
    }
    out << "};\n";

    // King zone attack
    out << "constexpr Score KING_ZONE_ATTACK[4] = {\n";
    for (int i = 0; i < 4; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[KING_ZONE_ATTACK_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[KING_ZONE_ATTACK_OFFSET + 2 * i + 1].value))
            << "),\n";
    }
    out << "};\n";

    // King zone weak square
    out << "constexpr Score KING_ZONE_WEAK_SQUARE = S(" << static_cast<int>(std::round(params[KING_ZONE_WEAK_SQ_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[KING_ZONE_WEAK_SQ_OFFSET + 1].value)) << ");\n";

    // King zone weak square extended
    out << "constexpr Score KING_ZONE_WEAK_SQUARE_EXTENDED = S(" << static_cast<int>(std::round(params[KING_ZONE_WEAK_EXT_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[KING_ZONE_WEAK_EXT_OFFSET + 1].value)) << ");\n";

    // King castled
    out << "constexpr Score KING_CASTLED[2] = {\n";
    for (int i = 0; i < 2; i++) {
        out << "\tS(" << static_cast<int>(std::round(params[KING_CASTLED_OFFSET + 2 * i].value)) << ", " << static_cast<int>(std::round(params[KING_CASTLED_OFFSET + 2 * i + 1].value)) << "),\n";
    }
    out << "};\n";

    // King lost one CR
    out << "constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(" << static_cast<int>(std::round(params[KING_LOST_CASTLE_OFFSET].value)) << ", "
        << static_cast<int>(std::round(params[KING_LOST_CASTLE_OFFSET + 1].value)) << ");\n";

    // King uncastled rights remain
    out << "constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(" << static_cast<int>(params[KING_UNCASTLED_OFFSET].value) << ", "
        << static_cast<int>(std::round(params[KING_UNCASTLED_OFFSET + 1].value)) << ");\n";

    // PST
    out << "const Score pst[12][64] = {\n";
    for (int p = 0; p < 12; p++) {
        out << "\t{\n\t\t";
        for (int sq = 0; sq < 64; sq++) {
            out << "S(" << static_cast<int>(std::round(params[PST_OFFSET + (p * 128) + (sq * 2)].value)) << ", "
                << static_cast<int>(std::round(params[PST_OFFSET + (p * 128) + (sq * 2) + 1].value)) << "), ";
            if ((sq + 1) % 8 == 0) {
                out << "\n\t\t";
            }
        }
        out << "\n},\n";
    }
    out << "};\n";
}

double Tuner::reconstructScore(const Trace& tr) const {
    double mg = 0.0, eg = 0.0;
    double phase = tr.phase / 24.0;

    // Material
    for (int p = 0; p < 6; p++) {
        int coeff = tr.material[p][WHITE] - tr.material[p][BLACK];
        mg += coeff * params[MATERIAL_OFFSET + p * 2].value;
        eg += coeff * params[MATERIAL_OFFSET + p * 2 + 1].value;
    }

    // Tempo
    int tempo = tr.tempo[WHITE] - tr.tempo[BLACK];
    mg += tempo * params[TEMPO_OFFSET].value;
    eg += tempo * params[TEMPO_OFFSET + 1].value;

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

void Tuner::updateGradients(const Trace& tr, double base, double phase) {
    // Material
    for (int p = 0; p < 6; p++) {
        int coeff = tr.material[p][WHITE] - tr.material[p][BLACK];
        params[MATERIAL_OFFSET + p * 2].grad += base * coeff * phase;
        params[MATERIAL_OFFSET + p * 2 + 1].grad += base * coeff * (1.0 - phase);
    }

    // Tempo
    int tempo = tr.tempo[WHITE] - tr.tempo[BLACK];
    params[TEMPO_OFFSET].grad += base * tempo * phase;
    params[TEMPO_OFFSET + 1].grad += base * tempo * (1.0 - phase);

    // Mobility
    for (int i = 0; i < 5; i++) {
        int coeff = tr.mobility[i][WHITE] - tr.mobility[i][BLACK];
        params[MOBILITY_OFFSET + i * 2].grad += base * coeff * phase;
        params[MOBILITY_OFFSET + i * 2 + 1].grad += base * coeff * (1.0 - phase);
    }

    // Pawn structure
    // Pawn phalanx
    int phal = tr.pawn_phalanx[WHITE] - tr.pawn_phalanx[BLACK];
    params[PAWN_PHALANX_OFFSET].grad += base * phal * phase;
    params[PAWN_PHALANX_OFFSET + 1].grad += base * phal * (1.0 - phase);

    // Doubled pawns
    int dp = tr.doubled_pawns[WHITE] - tr.doubled_pawns[BLACK];
    params[DOUBLED_PAWNS_OFFSET].grad += base * dp * phase;
    params[DOUBLED_PAWNS_OFFSET + 1].grad += base * dp * (1.0 - phase);

    // Backwards pawn
    int backwards = tr.backwards_pawn[WHITE] - tr.backwards_pawn[BLACK];
    params[BACKWARDS_PAWN_OFFSET].grad += base * backwards * phase;
    params[BACKWARDS_PAWN_OFFSET + 1].grad += base * backwards * (1.0 - phase);

    // Pawn protection
    for (int p = 0; p < 6; p++) {
        int coeff = tr.pawn_protection[p][WHITE] - tr.pawn_protection[p][BLACK];
        params[PAWN_PROTECTION_OFFSET + 2 * p].grad += base * coeff * phase;
        params[PAWN_PROTECTION_OFFSET + 2 * p + 1].grad += base * coeff * (1.0 - phase);
    }

    // Passed pawns
    for (int r = 0; r < 8; r++) {
        int coeff = tr.passed_pawns[r][WHITE] - tr.passed_pawns[r][BLACK];
        params[PASSED_PAWNS_OFFSET + 2 * r].grad += base * coeff * phase;
        params[PASSED_PAWNS_OFFSET + 2 * r + 1].grad += base * coeff * (1.0 - phase);
    }

    // Knights
    // Knight outpost
    int outpost = tr.knight_outpost[WHITE] - tr.knight_outpost[BLACK];
    params[KNIGHT_OUTPOST_OFFSET].grad += base * outpost * phase;
    params[KNIGHT_OUTPOST_OFFSET + 1].grad += base * outpost * (1.0 - phase);

    // Knight behind pawn
    int behind_pawn = tr.knight_behind_pawn[WHITE] - tr.knight_behind_pawn[BLACK];
    params[KNIGHT_BEHIND_PAWN_OFFSET].grad += base * behind_pawn * phase;
    params[KNIGHT_BEHIND_PAWN_OFFSET + 1].grad += base * behind_pawn * (1.0 - phase);

    // Knight pawn adjustments
    for (int i = 0; i < 9; i++) {
        int coeff = tr.knight_pawn_adj[i][WHITE] - tr.knight_pawn_adj[i][BLACK];
        params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i].grad += base * coeff * phase;
        params[KNIGHT_PAWN_ADJ_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // Bishops
    // Bishop pair
    int bishop_pair = tr.bishop_pair[WHITE] - tr.bishop_pair[BLACK];
    params[BISHOP_PAIR_OFFSET].grad += base * bishop_pair * phase;
    params[BISHOP_PAIR_OFFSET + 1].grad += base * bishop_pair * (1.0 - phase);

    // Bishop control penalty
    int bcp = tr.bishop_control_penalty[WHITE] - tr.bishop_control_penalty[BLACK];
    params[BISHOP_CTRL_PENALTY_OFFSET].grad += base * bcp * phase;
    params[BISHOP_CTRL_PENALTY_OFFSET + 1].grad += base * bcp * (1.0 - phase);

    // Bad bishop
    int bad_bishop = tr.bad_bishop[WHITE] - tr.bad_bishop[BLACK];
    params[BAD_BISHOP_OFFSET].grad += base * bad_bishop * phase;
    params[BAD_BISHOP_OFFSET + 1].grad += base * bad_bishop * (1.0 - phase);

    // Bishop blocking pawn
    int bbp = tr.bishop_blocking_pawn[WHITE] - tr.bishop_blocking_pawn[BLACK];
    params[BISHOP_BLOCKING_PAWN_OFFSET].grad += base * bbp * phase;
    params[BISHOP_BLOCKING_PAWN_OFFSET + 1].grad += base * bbp * (1.0 - phase);

    // Trapped bishop
    int trapped_bishop = tr.trapped_bishop[WHITE] - tr.trapped_bishop[BLACK];
    params[TRAPPED_BISHOP_OFFSET].grad += base * trapped_bishop * phase;
    params[TRAPPED_BISHOP_OFFSET + 1].grad += base * trapped_bishop * (1.0 - phase);

    // Rooks
    // Rook on seventh
    int rook_seventh = tr.rook_on_seventh_rank[WHITE] - tr.rook_on_seventh_rank[BLACK];
    params[ROOK_SEVENTH_OFFSET].grad += base * rook_seventh * phase;
    params[ROOK_SEVENTH_OFFSET + 1].grad += base * rook_seventh * (1.0 - phase);

    // Rook on open file
    int rook_open = tr.rook_on_open_file[WHITE] - tr.rook_on_open_file[BLACK];
    params[ROOK_OPEN_FILE_OFFSET].grad += base * rook_open * phase;
    params[ROOK_OPEN_FILE_OFFSET + 1].grad += base * rook_open * (1.0 - phase);

    // Rook on semi open file
    int rook_semi = tr.rook_on_semi_open_file[WHITE] - tr.rook_on_semi_open_file[BLACK];
    params[ROOK_SEMI_OPEN_FILE_OFFSET].grad += base * rook_semi * phase;
    params[ROOK_SEMI_OPEN_FILE_OFFSET + 1].grad += base * rook_semi * (1.0 - phase);

    // Rook pawn adjustments
    for (int i = 0; i < 9; i++) {
        int coeff = tr.rook_pawn_adj[i][WHITE] - tr.rook_pawn_adj[i][BLACK];
        params[ROOK_PAWN_ADJ_OFFSET + 2 * i].grad += base * coeff * phase;
        params[ROOK_PAWN_ADJ_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // Queen
    // Queen rel pin
    int qrp = tr.queen_rel_pin[WHITE] - tr.queen_rel_pin[BLACK];
    params[QUEEN_REL_PIN_OFFSET].grad += base * qrp * phase;
    params[QUEEN_REL_PIN_OFFSET + 1].grad += base * qrp * (1.0 - phase);

    // No opponent queens
    int noq = tr.no_opponent_queens[WHITE] - tr.no_opponent_queens[BLACK];
    params[NO_OPPONENT_QUEENS_OFFSET].grad += base * noq * phase;
    params[NO_OPPONENT_QUEENS_OFFSET + 1].grad += base * noq * (1.0 - phase);

    // King
    // King open file
    int k_open = tr.king_on_open_file[WHITE] - tr.king_on_open_file[BLACK];
    params[KING_OPEN_FILE_OFFSET].grad += base * k_open * phase;
    params[KING_OPEN_FILE_OFFSET + 1].grad += base * k_open * (1.0 - phase);

    // King semi open file
    int k_semi = tr.king_on_semi_open_file[WHITE] - tr.king_on_semi_open_file[BLACK];
    params[KING_SEMI_OPEN_FILE_OFFSET].grad += base * k_semi * phase;
    params[KING_SEMI_OPEN_FILE_OFFSET + 1].grad += base * k_semi * (1.0 - phase);

    // King pawn shield
    for (int i = 0; i < 4; i++) {
        int coeff = tr.pawn_shield[i][WHITE] - tr.pawn_shield[i][BLACK];
        params[PAWN_SHIELD_OFFSET + 2 * i].grad += base * coeff * phase;
        params[PAWN_SHIELD_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // Pawn storm
    for (int i = 0; i < 3; i++) {
        int coeff = tr.pawn_storm[i][WHITE] - tr.pawn_storm[i][BLACK];
        params[PAWN_STORM_OFFSET + 2 * i].grad += base * coeff * phase;
        params[PAWN_STORM_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // King zone attack
    for (int i = 0; i < 4; i++) {
        int coeff = tr.king_zone_attack[i][WHITE] - tr.king_zone_attack[i][BLACK];
        params[KING_ZONE_ATTACK_OFFSET + 2 * i].grad += base * coeff * phase;
        params[KING_ZONE_ATTACK_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // King zone weak square
    int kzws = tr.king_zone_weak_square[WHITE] - tr.king_zone_weak_square[BLACK];
    params[KING_ZONE_WEAK_SQ_OFFSET].grad += base * kzws * phase;
    params[KING_ZONE_WEAK_SQ_OFFSET + 1].grad += base * kzws * (1.0 - phase);

    // King zone weak square extended
    int kzwse = tr.king_zone_weak_square_extended[WHITE] - tr.king_zone_weak_square_extended[BLACK];
    params[KING_ZONE_WEAK_EXT_OFFSET].grad += base * kzwse * phase;
    params[KING_ZONE_WEAK_EXT_OFFSET + 1].grad += base * kzwse * (1.0 - phase);

    // King castled
    for (int i = 0; i < 2; i++) {
        int coeff = tr.king_castled[i][WHITE] - tr.king_castled[i][BLACK];
        params[KING_CASTLED_OFFSET + 2 * i].grad += base * coeff * phase;
        params[KING_CASTLED_OFFSET + 2 * i + 1].grad += base * coeff * (1.0 - phase);
    }

    // King lost one CR
    int kl1cr = tr.king_lost_one_castling_right[WHITE] - tr.king_lost_one_castling_right[BLACK];
    params[KING_LOST_CASTLE_OFFSET].grad += base * kl1cr * phase;
    params[KING_LOST_CASTLE_OFFSET + 1].grad += base * kl1cr * (1.0 - phase);

    // King uncastled rights remain
    int kucr = tr.king_uncastled_rights_remain[WHITE] - tr.king_uncastled_rights_remain[BLACK];
    params[KING_UNCASTLED_OFFSET].grad += base * kucr * phase;
    params[KING_UNCASTLED_OFFSET + 1].grad += base * kucr * (1.0 - phase);

    // PST
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) {
            // white pieces positive, black pieces negative
            int coeff = (p < 6) ? tr.pst[p][sq] : -tr.pst[p][sq];
            params[PST_OFFSET + (p * 128) + (sq * 2)].grad += base * coeff * phase;
            params[PST_OFFSET + (p * 128) + (sq * 2) + 1].grad += base * coeff * (1.0 - phase);
        }
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