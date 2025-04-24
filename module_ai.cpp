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

namespace py = pybind11;

class Connect4AI {
private:
    static constexpr int ROWS = 6;
    static constexpr int COLS = 7;
    static constexpr int WIN_LENGTH = 4;
    static constexpr int MAX_DEPTH = 7;

public:
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
            score += 100000;
        } else if (player_count == 3 && empty_count == 1) {
            score += 150;
        } else if (player_count == 2 && empty_count == 2) {
            score += 15;
        }

        int opponent_count = 0;
        empty_count = 0;
        for (int val : window) {
            if (val == opponent) opponent_count++;
            else if (val == 0) empty_count++;
        }

        if (opponent_count == 3 && empty_count == 1) {
            score -= 120; // Tăng từ -80 lên -120
        }

        return score;
    }

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

    static int evaluate_position(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                if (board[r][c] == player) {
                    if (c == 3) {
                        score += 5; // Tăng từ 3 lên 5
                    } else if (c == 2 || c == 4) {
                        score += 3; // Tăng từ 2 lên 3
                    } else if (c == 1 || c == 5) {
                        score += 1; // Thêm điểm cho cột xa trung tâm
                    }
                }
            }
        }

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<int> window;
                for (int i = 0; i < 4; i++) {
                    window.push_back(board[r][c + i]);
                }
                score += evaluate_window(window, player);
            }
        }

        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS - 3; r++) {
                std::vector<int> window;
                for (int i = 0; i < 4; i++) {
                    window.push_back(board[r + i][c]);
                }
                score += evaluate_window(window, player);
            }
        }

        for (int r = 0; r < ROWS - 3; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<int> window;
                for (int i = 0; i < 4; i++) {
                    window.push_back(board[r + i][c + i]);
                }
                score += evaluate_window(window, player);
            }
        }

        for (int r = 3; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<int> window;
                for (int i = 0; i < 4; i++) {
                    window.push_back(board[r - i][c + i]);
                }
                score += evaluate_window(window, player);
            }
        }

        int player_threes = count_open_threes(board, player);
        int opponent = 3 - player;
        int opponent_threes = count_open_threes(board, opponent);

        if (player_threes >= 2) {
            score += 15000; // Tăng từ 10000 lên 15000
        } else if (opponent_threes >= 2) {
            score -= 15000; // Tăng từ -10000 lên -15000
        } else {
            score += player_threes * 150 - opponent_threes * 150; // Tăng từ 100 lên 150
        }

        return score;
    }

    static bool is_terminal_node(const std::vector<std::vector<int>>& board) {
        return check_winner(board) != 0 || is_board_full(board);
    }

    static int check_winner(const std::vector<std::vector<int>>& board) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - WIN_LENGTH + 1; c++) {
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

        for (int r = 0; r < ROWS - WIN_LENGTH + 1; r++) {
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

        for (int r = 0; r < ROWS - WIN_LENGTH + 1; r++) {
            for (int c = 0; c < COLS - WIN_LENGTH + 1; c++) {
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
            for (int c = 0; c < COLS - WIN_LENGTH + 1; c++) {
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

    static std::vector<int> get_valid_moves(const std::vector<std::vector<int>>& board) {
        std::vector<int> valid_moves;
        for (int c = 0; c < COLS; c++) {
            if (board[0][c] == 0) {
                valid_moves.push_back(c);
            }
        }
        // Sắp xếp ưu tiên cột trung tâm
        std::sort(valid_moves.begin(), valid_moves.end(),
                  [](int a, int b) { return std::abs(a - COLS/2) < std::abs(b - COLS/2); });
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

    static std::pair<std::vector<std::vector<int>>, int> make_move(const std::vector<std::vector<int>>& board, int col, int player) {
        int row = get_next_open_row(board, col);
        if (row == -1) {
            return {board, -1};
        }

        std::vector<std::vector<int>> new_board = board;
        new_board[row][col] = player;
        return {new_board, row};
    }

    static std::pair<float, int> minimax(const std::vector<std::vector<int>>& board, int depth, float alpha, float beta, bool maximizing_player, int player) {
        auto valid_moves = get_valid_moves(board);
        bool is_terminal = is_terminal_node(board);

        if (depth == 0 || is_terminal) {
            if (is_terminal) {
                int winner = check_winner(board);
                if (winner == player) {
                    return {100000000000000, -1};
                } else if (winner == 3 - player) {
                    return {-100000000000000, -1};
                } else {
                    return {0, -1};
                }
            } else {
                return {static_cast<float>(evaluate_position(board, player)), -1};
            }
        }

        if (maximizing_player) {
            float value = -std::numeric_limits<float>::infinity();
            int best_move = valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, _] = make_move(board, col, player);
                auto [new_score, _unused] = minimax(new_board, depth - 1, alpha, beta, false, player);

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
            int best_move = valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, _] = make_move(board, col, 3 - player);
                auto [new_score, _unused] = minimax(new_board, depth - 1, alpha, beta, true, player);

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

    static std::tuple<int, int, int, float> get_best_move(
        const std::vector<std::vector<int>>& board, int player, const std::vector<int>& valid_moves) {
        
        auto start_time = std::chrono::high_resolution_clock::now();
    
        // Đếm số quân cờ
        int piece_count = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) piece_count++;
            }
        }
    
        // Kiểm tra nước đi thắng ngay lập tức
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1 && check_winner(new_board) == player) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                return {col, 100000, 0, std::stof(ss.str())};
            }
        }
    
        // Kiểm tra nước đi chặn đối thủ
        int opponent = 3 - player;
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, opponent);
            if (row != -1 && check_winner(new_board) == opponent) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                return {col, -100000, 0, std::stof(ss.str())};
            }
        }
    
        // Xác định độ sâu dựa trên giai đoạn và chuỗi 3
        int player_threes = count_open_threes(board, player);
        int opponent_threes = count_open_threes(board, opponent);
        int depth = MAX_DEPTH;
        if (player_threes >= 1 || opponent_threes >= 1) {
            depth += 2; // Tăng độ sâu khi có cơ hội hoặc nguy cơ
        } else if (valid_moves.size() <= 3) {
            depth += 2; // Tăng độ sâu khi ít nước đi
        } else if (piece_count >= 30) {
            depth = 10; // Giai đoạn cuối
        } else if (piece_count < 15) {
            depth = 5; // Giai đoạn đầu
        }
    
        // Iterative Deepening
        float time_limit = 1.0f; // Giới hạn 1 giây
        int best_move = valid_moves[0];
        float best_score = -std::numeric_limits<float>::infinity();
        for (int d = 1; d <= depth; ++d) {
            auto [score, move] = minimax(board, d, -std::numeric_limits<float>::infinity(),
                                         std::numeric_limits<float>::infinity(), true, player);
            if (move != -1 && std::find(valid_moves.begin(), valid_moves.end(), move) != valid_moves.end()) {
                best_score = score;
                best_move = move;
            }
    
            auto current_time = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(current_time - start_time).count();
            if (elapsed >= time_limit) {
                break;
            }
        }
    
        auto end_time = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float>(end_time - start_time).count();
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << duration;
        std::string duration_str = ss.str();
        float formatted_duration = std::stof(duration_str);
    
        std::cout << "Connect4AI: Độ sâu " << depth 
                  << ", Nước đi " << best_move 
                  << ", Điểm " << best_score
                  << ", Thời gian " << formatted_duration << "s" << std::endl;
    
        return {best_move, static_cast<int>(best_score), depth, formatted_duration};
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