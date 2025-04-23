#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <tuple>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ctime>

namespace py = pybind11;

class Connect4AI {
private:
    static constexpr int ROWS = 6;
    static constexpr int COLS = 7;
    static constexpr int WIN_LENGTH = 4;
    static constexpr int MAX_DEPTH = 6;
    
    // Pattern scores
    static constexpr int SCORE_WIN = 100000;
    static constexpr int SCORE_THREE = 150;
    static constexpr int SCORE_TWO = 15;
    static constexpr int SCORE_CENTER = 5;

public:
    // Window evaluation
    static int evaluate_window(const std::vector<int>& window, int player) {
        int opponent = 3 - player;
        int score = 0;

        int player_count = 0;
        int empty_count = 0;
        for (int val : window) {
            if (val == player) player_count++;
            else if (val == 0) empty_count++;
        }

        if (player_count == 4) {
            score += SCORE_WIN;
        } else if (player_count == 3 && empty_count == 1) {
            score += SCORE_THREE;
        } else if (player_count == 2 && empty_count == 2) {
            score += SCORE_TWO;
        }

        int opponent_count = 0;
        empty_count = 0;
        for (int val : window) {
            if (val == opponent) opponent_count++;
            else if (val == 0) empty_count++;
        }

        if (opponent_count == 3 && empty_count == 1) {
            score -= SCORE_THREE * 0.8;
        } else if (opponent_count == 2 && empty_count == 2) {
            score -= SCORE_TWO * 0.5;
        }

        return score;
    }

    // Get winning lines
    static std::vector<std::vector<std::pair<int, int>>> get_winning_lines() {
        std::vector<std::vector<std::pair<int, int>>> lines;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r, c + i});
                }
                lines.push_back(line);
            }
        }

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS - 3; r++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r + i, c});
                }
                lines.push_back(line);
            }
        }
        
        for (int r = 0; r < ROWS - 3; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r + i, c + i});
                }
                lines.push_back(line);
            }
        }
        
        for (int r = 3; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r - i, c + i});
                }
                lines.push_back(line);
            }
        }
        return lines;
    }

    // Count open threes
    static int count_open_threes(const std::vector<std::vector<int>>& board, int player) {
        int count = 0;
        auto winning_lines = get_winning_lines();
        for (const auto& line : winning_lines) {
            std::vector<int> pieces;
            for (const auto& pos : line) {
                int r = pos.first;
                int c = pos.second;
                pieces.push_back(board[r][c]);
            }
            int player_count = 0;
            int empty_count = 0;
            for (int piece : pieces) {
                if (piece == player) player_count++;
                else if (piece == 0) empty_count++;
            }
            if (player_count == 3 && empty_count == 1) {
                for (size_t idx = 0; idx < pieces.size(); idx++) {
                    if (pieces[idx] == 0) {
                        int r = line[idx].first;
                        int c = line[idx].second;
                        int r_empty = get_next_open_row(board, c);
                        if (r_empty == r) {
                            count++;
                            break;
                        }
                    }
                }
            }
        }
        return count;
    }

    // Position evaluation
    static int evaluate_position(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        auto lines = get_winning_lines();
        
        for (const auto& line : lines) {
            std::vector<int> window;
            for (const auto& pos : line) {
                window.push_back(board[pos.first][pos.second]);
            }
            score += evaluate_window(window, player);
        }

        // Center column preference
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                if (board[r][c] == player) {
                    if (c == 3) score += SCORE_CENTER;
                    else if (c == 2 || c == 4) score += SCORE_CENTER - 2;
                    else if (c == 1 || c == 5) score += SCORE_CENTER - 4;
                }
            }
        }

        // Open threes evaluation
        int player_threes = count_open_threes(board, player);
        int opponent = 3 - player;
        int opponent_threes = count_open_threes(board, opponent);

        if (player_threes >= 2) {
            score += 15000;
        } else if (opponent_threes >= 2) {
            score -= 15000;
        } else {
            score += player_threes * SCORE_THREE - opponent_threes * SCORE_THREE;
        }

        return score;
    }

    static bool is_terminal_node(const std::vector<std::vector<int>>& board) {
        return check_winner(board) != 0 || is_board_full(board);
    }

    static int check_winner(const std::vector<std::vector<int>>& board) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                if (board[r][c] != 0) {
                    bool win = true;
                    for (int i = 1; i < WIN_LENGTH; i++) {
                        if (board[r][c] != board[r][c + i]) {
                            win = false;
                            break;
                        }
                    }
                    if (win) return board[r][c];
                }
            }
        }

        for (int r = 0; r <= ROWS - WIN_LENGTH; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) {
                    bool win = true;
                    for (int i = 1; i < WIN_LENGTH; i++) {
                        if (board[r][c] != board[r + i][c]) {
                            win = false;
                            break;
                        }
                    }
                    if (win) return board[r][c];
                }
            }
        }

        for (int r = 0; r <= ROWS - WIN_LENGTH; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                if (board[r][c] != 0) {
                    bool win = true;
                    for (int i = 1; i < WIN_LENGTH; i++) {
                        if (board[r][c] != board[r + i][c + i]) {
                            win = false;
                            break;
                        }
                    }
                    if (win) return board[r][c];
                }
            }
        }

        for (int r = WIN_LENGTH - 1; r < ROWS; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                if (board[r][c] != 0) {
                    bool win = true;
                    for (int i = 1; i < WIN_LENGTH; i++) {
                        if (board[r][c] != board[r - i][c + i]) {
                            win = false;
                            break;
                        }
                    }
                    if (win) return board[r][c];
                }
            }
        }

        return 0;
    }

    static bool is_board_full(const std::vector<std::vector<int>>& board) {
        for (int c = 0; c < COLS; c++) {
            if (board[0][c] == 0) return false;
        }
        return true;
    }

    static std::vector<int> get_valid_moves(const std::vector<std::vector<int>>& board, int player = 0) {
        std::vector<std::pair<int, int>> scored_moves;
        
        for (int c = 0; c < COLS; c++) {
            if (board[0][c] == 0) {
                int score = SCORE_CENTER * (3 - std::min(std::abs(c - COLS/2), 3));
                
                for (int dc = -1; dc <= 1; dc++) {
                    int nc = c + dc;
                    if (nc >= 0 && nc < COLS) {
                        int r = get_next_open_row(board, nc);
                        if (r != -1 && r < ROWS-1 && board[r+1][nc] != 0) {
                            score += 2;
                        }
                    }
                }
                
                if (player != 0) {
                    auto [new_board, row] = make_move(board, c, player);
                    if (row != -1) {
                        if (check_winner(new_board) == player) {
                            score += 1000;
                        }
                        
                        auto [opp_board, _] = make_move(board, c, 3-player);
                        if (check_winner(opp_board) == 3-player) {
                            score += 900;
                        }
                    }
                }
                
                scored_moves.push_back({c, score});
            }
        }
        
        std::sort(scored_moves.begin(), scored_moves.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<int> valid_moves;
        for (const auto& [move, _] : scored_moves) {
            valid_moves.push_back(move);
        }
        
        return valid_moves;
    }

    static int get_next_open_row(const std::vector<std::vector<int>>& board, int col) {
        for (int r = ROWS - 1; r >= 0; r--) {
            if (board[r][col] == 0) {
                return r;
            }
        }
        return -1;
    }

    static std::pair<std::vector<std::vector<int>>, int> make_move(
            const std::vector<std::vector<int>>& board, int col, int player) {
        int row = get_next_open_row(board, col);
        if (row == -1) {
            return {board, -1};
        }

        std::vector<std::vector<int>> new_board = board;
        new_board[row][col] = player;
        return {new_board, row};
    }

    // Minimax with alpha-beta pruning
    static std::pair<float, int> minimax(
            const std::vector<std::vector<int>>& board, 
            int depth, 
            float alpha, 
            float beta, 
            bool maximizing_player, 
            int player) {
        
        bool is_terminal = is_terminal_node(board);
        if (depth == 0 || is_terminal) {
            if (is_terminal) {
                int winner = check_winner(board);
                if (winner == player) {
                    return {SCORE_WIN * 10, -1};
                } else if (winner == 3 - player) {
                    return {-SCORE_WIN * 10, -1};
                } else {
                    return {0, -1};
                }
            } else {
                return {static_cast<float>(evaluate_position(board, player)), -1};
            }
        }
        
        auto valid_moves = get_valid_moves(board, maximizing_player ? player : 3-player);
        
        if (maximizing_player) {
            float value = -std::numeric_limits<float>::infinity();
            int best_move = valid_moves.empty() ? -1 : valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, player);
                if (row == -1) continue;
                
                auto [new_score, _] = minimax(new_board, depth - 1, alpha, beta, false, player);

                if (new_score > value) {
                    value = new_score;
                    best_move = col;
                }

                alpha = std::max(alpha, value);
                if (alpha >= beta) {
                    break;
                }
            }
            
            return {value, best_move};
        } else {
            float value = std::numeric_limits<float>::infinity();
            int best_move = valid_moves.empty() ? -1 : valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, 3 - player);
                if (row == -1) continue;
                
                auto [new_score, _] = minimax(new_board, depth - 1, alpha, beta, true, player);

                if (new_score < value) {
                    value = new_score;
                    best_move = col;
                }

                beta = std::min(beta, value);
                if (alpha >= beta) {
                    break;
                }
            }
            
            return {value, best_move};
        }
    }

    static int determine_depth(int piece_count) {
        if (piece_count < 10) return 5;
        else if (piece_count < 25) return 6;
        else if (piece_count < 30) return 7;
        else return 10; // Tăng độ sâu ở giai đoạn cuối
    }

    static std::tuple<int, int, int, float> get_best_move(
        const std::vector<std::vector<int>>& board, int player, const std::vector<int>& valid_moves_) {
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Get current timestamp for log
        std::time_t now = std::time(nullptr);
        char timestamp[20];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        int piece_count = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) piece_count++;
            }
        }
        
        std::vector<int> valid_moves = get_valid_moves(board, player);
        if (valid_moves.empty()) valid_moves = valid_moves_;
        
        if (valid_moves.empty()) {
            std::cout << timestamp << " Connect4AI: Error - No valid moves available!" << std::endl;
            return {-1, 0, 0, 0.0f};
        }
        
        int depth = determine_depth(piece_count);
        
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1 && check_winner(new_board) == player) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Found immediate win at column " << col 
                          << ", Time: " << formatted_duration << "s" << std::endl;
                return {col, SCORE_WIN, 1, formatted_duration};
            }
        }

        int opponent = 3 - player;
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, opponent);
            if (row != -1 && check_winner(new_board) == opponent) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Blocking opponent win at column " << col 
                          << ", Time: " << formatted_duration << "s" << std::endl;
                return {col, -SCORE_WIN/2, 1, formatted_duration};
            }
        }
        
        for (int col : valid_moves) {
            auto [temp_board, row] = make_move(board, col, player);
            if (row == -1) continue;
            
            int winning_moves = 0;
            for (int next_col : valid_moves) {
                if (next_col == col) continue;
                
                auto [next_board, next_row] = make_move(temp_board, next_col, player);
                if (next_row != -1 && check_winner(next_board) == player) {
                    winning_moves++;
                }
            }
            
            if (winning_moves >= 2) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Creating dual threat at column " << col 
                          << ", Time: " << formatted_duration << "s" << std::endl;
                return {col, SCORE_WIN/4, 2, formatted_duration};
            }
        }

        auto [score, best_move] = minimax(board, depth, -std::numeric_limits<float>::infinity(),
                                        std::numeric_limits<float>::infinity(), true, player);

        if (best_move == -1 || std::find(valid_moves.begin(), valid_moves.end(), best_move) == valid_moves.end()) {
            best_move = valid_moves[0];
            std::cout << timestamp << " Connect4AI: Warning - Fallback to first valid move: " << best_move << std::endl;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float>(end_time - start_time).count();
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << duration;
        float formatted_duration = std::stof(ss.str());

        std::cout << timestamp << " Connect4AI: Stage: " << piece_count << "/" << (ROWS * COLS)
                  << ", Depth: " << depth 
                  << ", Move: " << best_move 
                  << ", Score: " << score
                  << ", Time: " << formatted_duration << "s" << std::endl;

        return {best_move, static_cast<int>(score), depth, formatted_duration};
    }
};

PYBIND11_MODULE(module_ai, m) {
    py::class_<Connect4AI>(m, "Connect4AI")
        .def_static("get_best_move", [](const std::vector<std::vector<int>>& board, int player,
                                      const std::vector<int>& valid_moves) {
            auto [move, score, depth, duration] = Connect4AI::get_best_move(board, player, valid_moves);
            return py::make_tuple(move, score, depth, duration);
        });
}