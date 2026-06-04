#include "datagen.h"
#include "board/board.h"
#include "board/rules.h"
#include "hash/transposition.h"
#include "movegen/movegen.h"
#include "search-eval/eval.h"
#include "search-eval/search.h"
#include "uci/uci.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

static std::atomic<size_t> positions_generated = 0;
static std::atomic<size_t> last_progress = 0;
static std::mutex progress_mutex;
static std::chrono::high_resolution_clock::time_point datagen_start;

constexpr static int16_t DG_DEPTH = 128;
constexpr static int32_t DG_NODES = 5000;
constexpr static int16_t DG_RANDOM_MOVES = 8;
constexpr static size_t DG_POSITION_GOAL = 40000000;
constexpr static int DG_THREADS = 24;

// Filtering thresholds
constexpr static int DG_SCORE_FILTER = 1500;  // skip recording when |score| >= this
constexpr static int DG_OPENING_FILTER = 800; // discard whole game if first scored ply is this lopsided

// Adjudication
constexpr static int DG_ADJ_WIN_SCORE = 1500;
constexpr static int DG_ADJ_WIN_PLIES = 4;
constexpr static int DG_ADJ_DRAW_SCORE = 10;
constexpr static int DG_ADJ_DRAW_PLIES = 8;
constexpr static int DG_ADJ_DRAW_MIN_PLY = 40;

// Progress reporting
constexpr static size_t DG_PROGRESS_INTERVAL = 1000;

static bool isQuiet(const Board& board, Move best_move) {
    if (board.inCheck()) {
        return false;
    }

    if (Capture(best_move) || IsEP(best_move) || Prom(best_move)) {
        return false;
    }

    return true;
}

static bool isMateScore(int score) { return std::abs(score) >= SCORE_MAX - MAX_GAME_MOVES; }

struct FENScore {
    std::string fen;
    int score;

    FENScore(const std::string& fen, int score) : fen(fen), score(score) { }
};

static void datagenWorker(int thread_id) {
    const std::string filename = "data_" + std::to_string(thread_id) + ".txt";
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "datagen[" << thread_id << "]: failed to open " << filename << std::endl;
        return;
    }

    std::mt19937 rng(static_cast<uint32_t>(std::time(nullptr)) ^ (static_cast<uint32_t>(thread_id) * 0x9E3779B9u));

    stdin_enabled = false;

    GoParams params;
    params.nodes = DG_NODES;
    params.silent = true;
    params.depth = DG_DEPTH;

    std::vector<FENScore> fen_scores;
    fen_scores.reserve(512);

    while (positions_generated < DG_POSITION_GOAL) {
        Board board;
        std::string result;
        bool valid_opening = true;

        // Random opening moves
        for (int i = 0; i < DG_RANDOM_MOVES; i++) {
            MoveList moves;
            getLegalMoves(board, moves);
            if (moves.empty()) {
                valid_opening = false;
                break;
            }
            std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
            board.makeMove(moves[dist(rng)]);
        }

        if (!valid_opening) {
            fen_scores.clear();
            continue;
        }

        // Adjudication counters
        int win_streak_white = 0;
        int win_streak_black = 0;
        int draw_streak = 0;
        int game_ply = 0;
        bool first_scored = true;
        bool discard_game = false;

        tt.clear();

        while (1) { // Run game
            if (isDraw(board, 0)) {
                result = "0.5";
                break;
            }

            MoveList moves;
            getLegalMoves(board, moves);

            if (moves.empty()) { // Either checkmate or stalemate
                if (board.inCheck()) {
                    result = board.getXSTM() == WHITE ? "1.0" : "0.0";
                } else {
                    result = "0.5";
                }
                break;
            }

            int score;
            searching = true;
            Move best_move = search(board, DG_DEPTH, score, params);
            if (best_move == NO_MOVE) {
                result = "0.5";
                break;
            }

            const bool mate = isMateScore(score);
            const int white_score = board.getSTM() == WHITE ? score : -score;

            // WDL filter: opening already lopsided -> discard game
            if (first_scored) {
                first_scored = false;
                if (!mate && std::abs(white_score) > DG_OPENING_FILTER) {
                    discard_game = true;
                    break;
                }
            }

            // Adjudication streaks
            if (mate) {
                if (white_score > 0) {
                    win_streak_white++;
                    win_streak_black = 0;
                } else {
                    win_streak_black++;
                    win_streak_white = 0;
                }
                draw_streak = 0;
            } else if (white_score >= DG_ADJ_WIN_SCORE) {
                win_streak_white++;
                win_streak_black = 0;
                draw_streak = 0;
            } else if (white_score <= -DG_ADJ_WIN_SCORE) {
                win_streak_black++;
                win_streak_white = 0;
                draw_streak = 0;
            } else if (std::abs(white_score) <= DG_ADJ_DRAW_SCORE) {
                draw_streak++;
                win_streak_white = 0;
                win_streak_black = 0;
            } else {
                win_streak_white = 0;
                win_streak_black = 0;
                draw_streak = 0;
            }

            if (win_streak_white >= DG_ADJ_WIN_PLIES) {
                result = "1.0";
                break;
            }
            if (win_streak_black >= DG_ADJ_WIN_PLIES) {
                result = "0.0";
                break;
            }
            if (game_ply >= DG_ADJ_DRAW_MIN_PLY && draw_streak >= DG_ADJ_DRAW_PLIES) {
                result = "0.5";
                break;
            }

            // History buffer guard
            if (board.getHistPly() >= MAX_PLY + MAX_GAME_MOVES - 8) {
                result = "0.5";
                break;
            }

            if (!mate && std::abs(score) < DG_SCORE_FILTER && isQuiet(board, best_move)) {
                fen_scores.emplace_back(board.toFEN(), white_score);
            }
            board.makeMove(best_move);
            game_ply++;
        }

        if (discard_game) {
            fen_scores.clear();
            continue;
        }

        for (const FENScore& fs : fen_scores) {
            out << fs.fen << " | " << fs.score << " | " << result << "\n";
        }
        out.flush();

        const size_t new_total = positions_generated.fetch_add(fen_scores.size()) + fen_scores.size();

        // Progress indicator (thread 0 only)
        if (thread_id == 0) {
            size_t prev = last_progress.load();
            if (new_total - prev >= DG_PROGRESS_INTERVAL && last_progress.compare_exchange_strong(prev, new_total)) {
                const auto now = std::chrono::high_resolution_clock::now();
                const double elapsed_sec = std::chrono::duration<double>(now - datagen_start).count();
                const double pps = elapsed_sec > 0.0 ? static_cast<double>(new_total) / elapsed_sec : 0.0;
                const size_t remaining = new_total < DG_POSITION_GOAL ? DG_POSITION_GOAL - new_total : 0;
                const long long eta_sec = pps > 0.0 ? static_cast<long long>(static_cast<double>(remaining) / pps) : -1;
                const long long eta_h = eta_sec >= 0 ? eta_sec / 3600 : 0;
                const long long eta_m = eta_sec >= 0 ? (eta_sec % 3600) / 60 : 0;
                const long long eta_s = eta_sec >= 0 ? eta_sec % 60 : 0;
                std::lock_guard<std::mutex> lock(progress_mutex);
                std::cerr << "[datagen] " << new_total << " / " << DG_POSITION_GOAL << " positions  (" << static_cast<long long>(pps) << " pos/s, "
                          << static_cast<long long>(elapsed_sec) << "s elapsed, ETA ";

                if (eta_sec < 0) {
                    std::cerr << "--";
                } else {
                    std::cerr << eta_h << "h " << eta_m << "m " << eta_s << "s";
                }

                std::cerr << ")" << std::endl;
            }
        }

        fen_scores.clear();
    }
}

void runDatagen() {
    std::vector<std::thread> workers;
    workers.reserve(DG_THREADS);
    datagen_start = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < DG_THREADS; t++) {
        workers.emplace_back(datagenWorker, t);
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // RNG Stagger
    }

    for (std::thread& w : workers) {
        w.join();
    }

    const auto now = std::chrono::high_resolution_clock::now();
    const double elapsed_sec = std::chrono::duration<double>(now - datagen_start).count();
    const size_t total = positions_generated.load();
    const double pps = elapsed_sec > 0.0 ? static_cast<double>(total) / elapsed_sec : 0.0;
    std::cerr << "[datagen] done: " << total << " positions across " << DG_THREADS << " threads in " << static_cast<long long>(elapsed_sec) << "s ("
              << static_cast<long long>(pps) << " pos/s)" << std::endl;
}
