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
    
    // Pattern recognition constants
    static constexpr int WIN_SCORE = 100000000;
    static constexpr int THREE_IN_ROW_SCORE = 5000;
    static constexpr int TWO_IN_ROW_SCORE = 500;
    static constexpr int CENTER_PREFERENCE = 6;

public:
    // Improved evaluation function with better pattern recognition
    static int evaluate_window(const std::vector<int>& window, int player) {
        int opponent = 3 - player;
        int score = 0;

        // Count pieces in the window
        int player_count = 0;
        int empty_count = 0;
        int opponent_count = 0;

        for (int val : window) {
            if (val == player) player_count++;
            else if (val == 0) empty_count++;
            else if (val == opponent) opponent_count++;
        }

        // Pattern recognition with more nuanced scoring
        if (player_count == 4) {
            // Four in a row is a win
            score += WIN_SCORE;
        } else if (player_count == 3 && empty_count == 1) {
            // Three in a row with an empty spot - very strong threat
            score += THREE_IN_ROW_SCORE;
        } else if (player_count == 2 && empty_count == 2) {
            // Two in a row with two empty spots - moderate threat
            score += TWO_IN_ROW_SCORE;
        }

        // Defensive evaluation - react to opponent threats
        if (opponent_count == 3 && empty_count == 1) {
            // Opponent has three in a row - block immediately
            score -= THREE_IN_ROW_SCORE * 1.2; // Prioritize blocking
        } else if (opponent_count == 2 && empty_count == 2) {
            // Opponent has two in a row - consider blocking
            score -= TWO_IN_ROW_SCORE * 0.8;
        }

        return score;
    }

    static std::vector<std::vector<std::pair<int, int>>> get_winning_lines() {
        std::vector<std::vector<std::pair<int, int>>> lines;
        
        // Horizontal lines
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < WIN_LENGTH; i++) {
                    line.push_back({r, c + i});
                }
                lines.push_back(line);
            }
        }

        // Vertical lines
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r <= ROWS - WIN_LENGTH; r++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < WIN_LENGTH; i++) {
                    line.push_back({r + i, c});
                }
                lines.push_back(line);
            }
        }
        
        // Diagonal lines (positive slope)
        for (int r = 0; r <= ROWS - WIN_LENGTH; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < WIN_LENGTH; i++) {
                    line.push_back({r + i, c + i});
                }
                lines.push_back(line);
            }
        }
        
        // Diagonal lines (negative slope)
        for (int r = WIN_LENGTH - 1; r < ROWS; r++) {
            for (int c = 0; c <= COLS - WIN_LENGTH; c++) {
                std::vector<std::pair<int, int>> line;
                for (int i = 0; i < WIN_LENGTH; i++) {
                    line.push_back({r - i, c + i});
                }
                lines.push_back(line);
            }
        }
        
        return lines;
    }

    // Enhanced pattern recognition for better threat detection
    static int count_patterns(const std::vector<std::vector<int>>& board, int player) {
        int score = 0;
        auto winning_lines = get_winning_lines();
        
        for (const auto& line : winning_lines) {
            std::vector<int> pieces;
            for (const auto& pos : line) {
                int r = pos.first;
                int c = pos.second;
                pieces.push_back(board[r][c]);
            }
            
            // Check for pattern XX_X (Three with gap)
            if (pieces.size() >= 4) {
                for (size_t i = 0; i <= pieces.size() - 4; i++) {
                    // Pattern XX_X
                    if (pieces[i] == player && pieces[i+1] == player && 
                        pieces[i+2] == 0 && pieces[i+3] == player) {
                        int r = line[i+2].first;
                        int c = line[i+2].second;
                        if (is_valid_move(board, c, r)) {
                            score += THREE_IN_ROW_SCORE * 1.5;
                        }
                    }
                    
                    // Pattern X_XX
                    if (pieces[i] == player && pieces[i+1] == 0 && 
                        pieces[i+2] == player && pieces[i+3] == player) {
                        int r = line[i+1].first;
                        int c = line[i+1].second;
                        if (is_valid_move(board, c, r)) {
                            score += THREE_IN_ROW_SCORE * 1.5;
                        }
                    }
                }
            }
            
            // Check for pattern XX_XX (Four with gap in middle)
            if (pieces.size() >= 5) {
                for (size_t i = 0; i <= pieces.size() - 5; i++) {
                    if (pieces[i] == player && pieces[i+1] == player &&
                        pieces[i+2] == 0 && pieces[i+3] == player &&
                        pieces[i+4] == player) {
                        int r = line[i+2].first;
                        int c = line[i+2].second;
                        if (is_valid_move(board, c, r)) {
                            score += THREE_IN_ROW_SCORE * 2.0;
                        }
                    }
                }
            }
        }
        
        return score;
    }

    // Helper to check if a move is valid at a specific position
    static bool is_valid_move(const std::vector<std::vector<int>>& board, int col, int row) {
        // Check if the column is valid
        if (col < 0 || col >= COLS) return false;
        
        // For Connect4, we need to check if this position is the next valid row
        int valid_row = get_next_open_row(board, col);
        return valid_row == row;
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
        int opponent = 3 - player;

        // Center column preference
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] == player) {
                    // Prefer center columns
                    if (c == 3) {
                        score += CENTER_PREFERENCE;
                    } else if (c == 2 || c == 4) {
                        score += CENTER_PREFERENCE - 2;
                    } else if (c == 1 || c == 5) {
                        score += CENTER_PREFERENCE - 4;
                    }
                }
            }
        }

        // Evaluate all possible winning lines
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

        // Advanced pattern recognition
        score += count_patterns(board, player);
        score -= count_patterns(board, opponent) * 1.2;  // Prioritize defense slightly

        // Check for multiple threats
        int player_threes = count_open_threes(board, player);
        int opponent_threes = count_open_threes(board, opponent);

        if (player_threes >= 2) {
            score += THREE_IN_ROW_SCORE * 3; // Multiple threats likely wins
        } else if (opponent_threes >= 2) {
            score -= THREE_IN_ROW_SCORE * 3; // Multiple opponent threats likely loses
        } else {
            score += player_threes * THREE_IN_ROW_SCORE * 0.3;
            score -= opponent_threes * THREE_IN_ROW_SCORE * 0.4; // Prioritize defense
        }

        return score;
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

        // Diagonal check (positive slope)
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

        // Diagonal check (negative slope)
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

    static std::vector<int> get_valid_moves(const std::vector<std::vector<int>>& board) {
        std::vector<int> valid_moves;
        for (int c = 0; c < COLS; c++) {
            if (board[0][c] == 0) {
                valid_moves.push_back(c);
            }
        }
        
        // Sort by column preference - prioritize center columns
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

    // Improved minimax with better move ordering and pruning
    static std::pair<float, int> minimax(
        const std::vector<std::vector<int>>& board, 
        int depth, 
        float alpha, 
        float beta, 
        bool maximizing_player, 
        int player) {
        
        auto valid_moves = get_valid_moves(board);
        bool is_terminal = is_terminal_node(board);

        if (depth == 0 || is_terminal) {
            if (is_terminal) {
                int winner = check_winner(board);
                if (winner == player) {
                    return {WIN_SCORE * 1000.0f, -1};
                } else if (winner == 3 - player) {
                    return {-WIN_SCORE * 1000.0f, -1};
                } else {
                    return {0, -1};
                }
            } else {
                return {static_cast<float>(evaluate_position(board, player)), -1};
            }
        }

        // Using a simpler approach for move ordering
        if (maximizing_player) {
            float value = -std::numeric_limits<float>::infinity();
            int best_move = valid_moves.empty() ? -1 : valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, player);
                if (row == -1) continue;
                
                auto [new_score, _unused] = minimax(new_board, depth - 1, alpha, beta, false, player);

                if (new_score > value) {
                    value = new_score;
                    best_move = col;
                }

                alpha = std::max(alpha, value);
                if (alpha >= beta) {
                    break;  // Beta cutoff
                }
            }
            return {value, best_move};
        } else {
            float value = std::numeric_limits<float>::infinity();
            int best_move = valid_moves.empty() ? -1 : valid_moves[0];

            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, 3 - player);
                if (row == -1) continue;
                
                auto [new_score, _unused] = minimax(new_board, depth - 1, alpha, beta, true, player);

                if (new_score < value) {
                    value = new_score;
                    best_move = col;
                }

                beta = std::min(beta, value);
                if (alpha >= beta) {
                    break;  // Alpha cutoff
                }
            }
            return {value, best_move};
        }
    }

    // Pattern-based immediate threat detection
    static int detect_immediate_threats(
        const std::vector<std::vector<int>>& board, 
        const std::vector<int>& valid_moves,
        int player) {
        
        int opponent = 3 - player;
        
        // First check for immediate wins
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, player);
            if (row != -1 && check_winner(new_board) == player) {
                return col;  // Winning move
            }
        }
        
        // Then check for opponent's immediate threats
        for (int col : valid_moves) {
            auto [new_board, row] = make_move(board, col, opponent);
            if (row != -1 && check_winner(new_board) == opponent) {
                return col;  // Blocking move
            }
        }
        
        // Check for specific patterns
        auto winning_lines = get_winning_lines();
        
        // Find patterns like XX_X, X_XX that can lead to wins next move
        for (const auto& line : winning_lines) {
            // Skip if line is less than 4 positions
            if (line.size() < 4) continue;
            
            // Check each position in the line
            for (size_t i = 0; i <= line.size() - 4; i++) {
                // Create a window of 4 positions
                std::vector<std::pair<int, int>> window;
                for (size_t j = i; j < i + 4; j++) {
                    window.push_back(line[j]);
                }
                
                // Now check the pattern
                std::vector<int> pieces;
                std::vector<int> pos_of_empty;
                
                for (size_t j = 0; j < window.size(); j++) {
                    int r = window[j].first;
                    int c = window[j].second;
                    pieces.push_back(board[r][c]);
                    if (board[r][c] == 0) {
                        pos_of_empty.push_back(j);
                    }
                }
                
                // Pattern XX_X (player)
                if (pieces[0] == player && pieces[1] == player && 
                    pieces[2] == 0 && pieces[3] == player) {
                    int r = window[2].first;
                    int c = window[2].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
                
                // Pattern X_XX (player)
                if (pieces[0] == player && pieces[1] == 0 && 
                    pieces[2] == player && pieces[3] == player) {
                    int r = window[1].first;
                    int c = window[1].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
                
                // Pattern XX_X (opponent)
                if (pieces[0] == opponent && pieces[1] == opponent && 
                    pieces[2] == 0 && pieces[3] == opponent) {
                    int r = window[2].first;
                    int c = window[2].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
                
                // Pattern X_XX (opponent)
                if (pieces[0] == opponent && pieces[1] == 0 && 
                    pieces[2] == opponent && pieces[3] == opponent) {
                    int r = window[1].first;
                    int c = window[1].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
            }
        }
        
        // Check for pattern XX_XX in 5-long windows
        for (const auto& line : winning_lines) {
            // Skip if line is less than 5 positions
            if (line.size() < 5) continue;
            
            // Check each position in the line
            for (size_t i = 0; i <= line.size() - 5; i++) {
                // Create a window of 5 positions
                std::vector<std::pair<int, int>> window;
                for (size_t j = i; j < i + 5; j++) {
                    window.push_back(line[j]);
                }
                
                // Now check the pattern
                std::vector<int> pieces;
                for (size_t j = 0; j < window.size(); j++) {
                    int r = window[j].first;
                    int c = window[j].second;
                    pieces.push_back(board[r][c]);
                }
                
                // Pattern XX_XX (player)
                if (pieces[0] == player && pieces[1] == player && 
                    pieces[2] == 0 && pieces[3] == player && pieces[4] == player) {
                    int r = window[2].first;
                    int c = window[2].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
                
                // Pattern XX_XX (opponent)
                if (pieces[0] == opponent && pieces[1] == opponent && 
                    pieces[2] == 0 && pieces[3] == opponent && pieces[4] == opponent) {
                    int r = window[2].first;
                    int c = window[2].second;
                    if (is_valid_move(board, c, r)) {
                        return c;
                    }
                }
            }
        }
        
        return -1;  // No immediate threats found
    }

    static std::tuple<int, int, int, float> get_best_move(
        const std::vector<std::vector<int>>& board, 
        int player, 
        const std::vector<int>& valid_moves) {
        
        auto start_time = std::chrono::high_resolution_clock::now();

        // Check for immediate threats first
        int immediate_move = detect_immediate_threats(board, valid_moves, player);
        if (immediate_move != -1) {
            auto end_time = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration<float>(end_time - start_time).count();
            
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << duration;
            std::string duration_str = ss.str();
            float formatted_duration = std::stof(duration_str);
            
            std::cout << "Connect4AI: Detected immediate threat, playing column " 
                      << immediate_move << ", time " << formatted_duration << "s" << std::endl;
                      
            return {immediate_move, 100000, 1, formatted_duration};
        }
        
        // Compute piece count to determine game stage
        int piece_count = 0;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (board[r][c] != 0) {
                    piece_count++;
                }
            }
        }
        
        // Adjust depth based on game stage
        int depth = MAX_DEPTH;
        if (piece_count >= 30) {
            depth = 10;  // Late game - deeper search
        } else if (piece_count <= 10) {
            depth = 6;   // Early game - faster responses
        }

        // Run minimax with move ordering for efficiency
        auto [score, best_move] = minimax(
            board, 
            depth, 
            -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(), 
            true, 
            player);

        // Safety check
        if (best_move == -1 || std::find(valid_moves.begin(), valid_moves.end(), best_move) == valid_moves.end()) {
            // Fallback to center-prioritized move if minimax fails
            best_move = valid_moves[0];
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float>(end_time - start_time).count();
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << duration;
        std::string duration_str = ss.str();
        float formatted_duration = std::stof(duration_str);

        std::cout << "Connect4AI: Depth " << depth 
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