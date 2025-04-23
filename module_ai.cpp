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
    static constexpr int MAX_DEPTH = 6;
    
    // Pattern scores
    static constexpr int SCORE_WIN = 1000000;
    static constexpr int SCORE_THREE = 100;
    static constexpr int SCORE_TWO = 10;
    static constexpr int SCORE_CENTER = 3;

public:
    // Window evaluation
    static int evaluate_window(const std::vector<int>& window, int player) {
        int opponent = 3 - player;
        int score = 0;
        int player_count = 0;
        int empty_count = 0;
        int opponent_count = 0;
        
        for (int val : window) {
            if (val == player) player_count++;
            else if (val == 0) empty_count++;
            else if (val == opponent) opponent_count++;
        }
        
        if (opponent_count == 0) {
            if (player_count == 4) score += SCORE_WIN;
            else if (player_count == 3 && empty_count == 1) score += SCORE_THREE;
            else if (player_count == 2 && empty_count == 2) score += SCORE_TWO;
        }
        
        if (player_count == 0) {
            if (opponent_count == 3 && empty_count == 1) score -= SCORE_THREE * 1.2;
            else if (opponent_count == 2 && empty_count == 2) score -= SCORE_TWO * 0.8;
        }
        
        return score;
    }
    
    // Basic position evaluation
    static int evaluate_position(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        auto lines = get_winning_lines();
        
        for (const auto& line : lines) {
            std::vector<int> window;
            for (const auto& pos : line) {
                window.push_back(board[pos.first][pos.second]);
            }
            score += evaluate_window(window, player);
        }
        
        return score;
    }
    
    // Get winning lines
    static std::vector<std::vector<std::pair<int, int>>> get_winning_lines() {
        std::vector<std::vector<std::pair<int, int>>> lines;
        // Horizontal
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r, c + i});
                }
                lines.push_back(line);
            }
        }

        // Vertical
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS - 3; r++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r + i, c});
                }
                lines.push_back(line);
            }
        }
        
        // Diagonal positive slope
        for (int r = 0; r < ROWS - 3; r++) {
            for (int c = 0; c < COLS - 3; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < 4; i++) {
                    line.push_back({r + i, c + i});
                }
                lines.push_back(line);
            }
        }
        
        // Diagonal negative slope
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

    // Threat detection
    static std::tuple<int, int, int> detect_threats_existing(
            const std::vector<std::vector<int>>& board, int player) {
        int direct_threats = 0;
        int dual_threats = 0;
        int potential_threats = 0;
        
        auto winning_lines = get_winning_lines();
        std::vector<int> threatened_cols;
        
        for (int c = 0; c < COLS; c++) {
            auto [new_board, row] = make_move(board, c, player);
            if (row != -1 && check_winner(new_board) == player) {
                direct_threats++;
                threatened_cols.push_back(c);
            }
        }
        
        if (direct_threats == 0) {
            for (int c = 0; c < COLS; c++) {
                auto [temp_board, row] = make_move(board, c, player);
                if (row == -1) continue;
                
                int winning_moves = 0;
                for (int next_c = 0; next_c < COLS; next_c++) {
                    if (c == next_c) continue;
                    
                    auto [next_board, next_row] = make_move(temp_board, next_c, player);
                    if (next_row != -1 && check_winner(next_board) == player) {
                        winning_moves++;
                    }
                }
                
                if (winning_moves >= 2) {
                    dual_threats++;
                    break;
                }
                
                for (const auto& line : winning_lines) {
                    std::vector<int> window;
                    for (const auto& pos : line) {
                        window.push_back(temp_board[pos.first][pos.second]);
                    }
                    
                    int p_count = 0, e_count = 0;
                    for (int val : window) {
                        if (val == player) p_count++;
                        else if (val == 0) e_count++;
                    }
                    
                    if (p_count == 2 && e_count == 2) {
                        potential_threats++;
                    }
                }
            }
        }
        
        return {direct_threats, dual_threats, potential_threats};
    }

    // Position evaluation by square
    static int evaluate_position_by_square(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        
        int position_values[ROWS][COLS] = {
            {3, 4, 5, 7, 5, 4, 3},
            {4, 6, 8, 10, 8, 6, 4},
            {5, 8, 11, 13, 11, 8, 5},
            {5, 8, 11, 13, 11, 8, 5},
            {4, 6, 8, 10, 8, 6, 4},
            {3, 4, 5, 7, 5, 4, 3}
        };
        
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] == player) {
                    score += position_values[r][c];
                } else if (board[r][c] == opponent) {
                    score -= position_values[r][c];
                }
            }
        }
        
        return score;
    }

    // Detect critical patterns
    static int detect_threats(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        
        auto lines = get_winning_lines();
        for (const auto& line : lines) {
            std::vector<int> window;
            for (const auto& pos : line) {
                window.push_back(board[pos.first][pos.second]);
            }
            
            if (has_double_threat(board, line, player)) {
                score += 5000;
            }
            
            if (has_fork_pattern(board, line, player)) {
                score += 200;
            }
        }
        
        return score;
    }

    static bool has_double_threat(const std::vector<std::vector<int>>& board, 
                                const std::vector<std::pair<int, int>>& line, 
                                int player) {
        int opponent = 3 - player;
        int player_count = 0;
        int empty_count = 0;
        
        for (const auto& pos : line) {
            if (board[pos.first][pos.second] == player) player_count++;
            else if (board[pos.first][pos.second] == 0) empty_count++;
        }
        
        if (player_count == 2 && empty_count >= 2) {
            return true;
        }
        return false;
    }

    static bool has_fork_pattern(const std::vector<std::vector<int>>& board, 
                                const std::vector<std::pair<int, int>>& line, 
                                int player) {
        int opponent = 3 - player;
        int player_count = 0;
        int empty_count = 0;
        
        for (const auto& pos : line) {
            if (board[pos.first][pos.second] == player) player_count++;
            else if (board[pos.first][pos.second] == 0) empty_count++;
        }
        
        if (player_count == 2 && empty_count == 2) {
            return true;
        }
        return false;
    }

    // Evaluate by distance to win
    static int evaluate_by_distance_to_win(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        
        for (int c = 0; c < COLS; c++) {
            auto [test_board, row] = make_move(board, c, player);
            if (row != -1) {
                if (check_winner(test_board) == player) {
                    score += 1000;
                } else {
                    int min_moves = calculate_min_moves_to_win(test_board, player);
                    if (min_moves > 0 && min_moves < 4) {
                        score += (100 / min_moves);
                    }
                }
            }
            
            auto [opp_board, opp_row] = make_move(board, c, opponent);
            if (opp_row != -1 && check_winner(opp_board) == opponent) {
                score -= 800;
            }
        }
        
        return score;
    }

    static int calculate_min_moves_to_win(const std::vector<std::vector<int>>& board, int player) {
        auto lines = get_winning_lines();
        int min_moves = 4;
        
        for (const auto& line : lines) {
            int player_count = 0;
            int empty_count = 0;
            for (const auto& pos : line) {
                if (board[pos.first][pos.second] == player) player_count++;
                else if (board[pos.first][pos.second] == 0) empty_count++;
            }
            if (player_count + empty_count == 4 && empty_count > 0) {
                min_moves = std::min(min_moves, empty_count);
            }
        }
        return min_moves == 4 ? -1 : min_moves;
    }

    // Evaluate connectivity
    static int evaluate_connectivity(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        
        // Horizontal connectivity
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS - 1; c++) {
                if (board[r][c] == player && board[r][c+1] == player) {
                    score += 5;
                    if (c < COLS - 2 && board[r][c+2] == player) {
                        score += 15;
                    }
                }
            }
        }
        
        // Vertical connectivity
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS - 1; r++) {
                if (board[r][c] == player && board[r+1][c] == player) {
                    score += 5;
                    if (r < ROWS - 2 && board[r+2][c] == player) {
                        score += 15;
                    }
                }
            }
        }
        
        return score;
    }

    // Evaluate flexibility
    static int evaluate_flexibility(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        auto valid_moves = get_valid_moves(board);
        
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1) {
                int position_score = stage_based_evaluation(new_board, player);
                if (position_score > 50) {
                    score += 10;
                }
            }
        }
        
        return score;
    }

    // Avoid dangerous moves
    static int avoid_dangerous_moves(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        int opponent = 3 - player;
        auto valid_moves = get_valid_moves(board);
        
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1 && row > 0) {
                auto [opp_board, opp_row] = make_move(new_board, col, opponent);
                if (opp_row != -1 && check_winner(opp_board) == opponent) {
                    score -= 500;
                }
            }
        }
        
        return score;
    }

    // Combined evaluation with weights
    static int enhanced_evaluate(const std::vector<std::vector<int>>& board, int player) {
        int position_score = evaluate_position(board, player);
        int position_by_square = evaluate_position_by_square(board, player);
        int threat_score = detect_threats(board, player);
        int distance_score = evaluate_by_distance_to_win(board, player);
        int connectivity_score = evaluate_connectivity(board, player);
        int flexibility_score = evaluate_flexibility(board, player);
        int safety_score = avoid_dangerous_moves(board, player);
        
        return static_cast<int>(
            position_score * 1.0 +
            position_by_square * 0.8 +
            threat_score * 1.5 +
            distance_score * 1.2 +
            connectivity_score * 0.7 +
            flexibility_score * 0.6 +
            safety_score * 1.3
        );
    }

    // Stage-based evaluation
    static int stage_based_evaluation(const std::vector<std::vector<int>>& board, int player) {
        int piece_count = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) piece_count++;
            }
        }
        
        if (piece_count <= 10) { // Early game
            return static_cast<int>(
                evaluate_position_by_square(board, player) * 1.5 +
                evaluate_flexibility(board, player) * 1.2
            );
        } else if (piece_count <= 25) { // Mid game
            return static_cast<int>(
                detect_threats(board, player) * 1.3 +
                evaluate_position(board, player) * 1.0 +
                evaluate_connectivity(board, player) * 1.0
            );
        } else { // Late game
            return static_cast<int>(
                evaluate_by_distance_to_win(board, player) * 1.8 +
                avoid_dangerous_moves(board, player) * 1.5
            );
        }
    }

    static bool is_terminal_node(const std::vector<std::vector<int>>& board) {
        return check_winner(board) != 0 || is_board_full(board);
    }

    static int check_winner(const std::vector<std::vector<int>>& board) {
        // Horizontal check
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

        // Vertical check
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

        // Diagonal (positive slope)
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

        // Diagonal (negative slope)
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
                int score = 0;
                
                score += SCORE_CENTER * (3 - std::min(std::abs(c - COLS/2), 3));
                
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
                return {static_cast<float>(stage_based_evaluation(board, player)), -1};
            }
        }
        
        auto valid_moves = get_valid_moves(board, maximizing_player ? player : 3-player);
        
        if (maximizing_player) {
            float value = -std::numeric_limits<float>::infinity();
            int best_move = valid_moves.empty() ? -1 : valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, player);
                if (row == -1) continue;
                
                int new_depth = depth - 1;
                auto threat_result = detect_threats_existing(new_board, player);
                int player_direct = std::get<0>(threat_result);
                if (player_direct > 0) {
                    new_depth++;
                }
                
                auto [new_score, _] = minimax(new_board, new_depth, alpha, beta, false, player);

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
                
                int new_depth = depth - 1;
                auto opp_threat_result = detect_threats_existing(new_board, 3 - player);
                int opp_direct = std::get<0>(opp_threat_result);
                if (opp_direct > 0) {
                    new_depth++;
                }
                
                auto [new_score, _] = minimax(new_board, new_depth, alpha, beta, true, player);

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
        else if (piece_count < 35) return 7;
        else return 9;
    }

    static std::tuple<int, int, int, float> get_best_move(
        const std::vector<std::vector<int>>& board, int player, const std::vector<int>& valid_moves_) {
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int piece_count = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) piece_count++;
            }
        }
        
        std::vector<int> valid_moves = get_valid_moves(board, player);
        if (valid_moves.empty()) valid_moves = valid_moves_;
        
        int depth = determine_depth(piece_count);
        
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1 && check_winner(new_board) == player) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << "Connect4AI: Found immediate win at column " << col << std::endl;
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
                std::cout << "Connect4AI: Blocking opponent win at column " << col << std::endl;
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
                std::cout << "Connect4AI: Creating dual threat at column " << col << std::endl;
                return {col, SCORE_WIN/4, 2, formatted_duration};
            }
        }

        auto [score, best_move] = minimax(board, depth, -std::numeric_limits<float>::infinity(),
                                        std::numeric_limits<float>::infinity(), true, player);

        if (best_move == -1 || std::find(valid_moves.begin(), valid_moves.end(), best_move) == valid_moves.end()) {
            best_move = valid_moves[0];
            std::cout << "Connect4AI: Warning - had to fallback to first valid move!" << std::endl;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float>(end_time - start_time).count();
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << duration;
        float formatted_duration = std::stof(ss.str());

        std::cout << "Connect4AI: Stage: " << piece_count << "/" << (ROWS * COLS)
                << ", Depth " << depth 
                << ", Move " << best_move 
                << ", Score " << score
                << ", Time " << formatted_duration << "s" << std::endl;

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