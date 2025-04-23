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
    static int evaluate_window(const std::vector<int>& window, int player, bool is_diagonal = false, bool is_horizontal = false) {
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
            // Phân biệt mức độ nguy hiểm theo kiểu đường
            if (is_diagonal) {
                score -= SCORE_THREE * 1.5;
            } else if (is_horizontal) {
                score -= SCORE_THREE * 1.8; // Đường ngang nguy hiểm nhất
            } else {
                score -= SCORE_THREE * 1.2;
            }
        } else if (opponent_count == 2 && empty_count == 2) {
            if (is_diagonal) {
                score -= SCORE_TWO * 0.7;
            } else if (is_horizontal) {
                score -= SCORE_TWO * 0.9; // Cũng ưu tiên hơn cho đường ngang
            } else {
                score -= SCORE_TWO * 0.5;
            }
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
            // Xác định kiểu đường
            bool is_diagonal = false;
            bool is_horizontal = false;
            
            if (line.size() >= 2) {
                int r0 = line[0].first;
                int c0 = line[0].second;
                int r1 = line[1].first;
                int c1 = line[1].second;
                
                if (r0 == r1) {
                    is_horizontal = true;
                } else if (abs(r1 - r0) == 1 && abs(c1 - c0) == 1) {
                    is_diagonal = true;
                }
            }
            
            std::vector<int> window;
            for (const auto& pos : line) {
                window.push_back(board[pos.first][pos.second]);
            }
            
            score += evaluate_window(window, player, is_diagonal, is_horizontal);
        }
        
        // Phần còn lại của hàm evaluate_position
        // Đánh giá chiến lược vị trí
        
        // 1. Ưu tiên các vị trí ở giữa bảng
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; r++) {
                if (board[r][c] == player) {
                    // Thưởng cho vị trí gần trung tâm
                    int col_distance = abs(c - COLS/2);
                    score += (3 - std::min(col_distance, 3)) * 2;
                    
                    // Thưởng cho các vị trí ở hàng giữa
                    int row_center_distance = abs(r - ROWS/2);
                    score += (3 - std::min(row_center_distance, 3)) * 1;
                }
            }
        }
        
        // 2. Đánh giá khả năng phòng thủ
        int opponent = 3 - player;
        int opponent_threats = 0;
        
        for (int c = 0; c < COLS; c++) {
            auto [new_board, row] = make_move(board, c, opponent);
            if (row != -1 && check_winner(new_board) == opponent) {
                opponent_threats++;
            }
        }
        
        // Trừ điểm nếu có nhiều đe dọa từ đối thủ
        score -= opponent_threats * 200;
        
        // 3. Đánh giá khả năng kiểm soát
        int control_score = 0;
        for (int c = 0; c < COLS; c++) {
            int player_pieces = 0;
            int opponent_pieces = 0;
            
            for (int r = 0; r < ROWS; r++) {
                if (board[r][c] == player) player_pieces++;
                else if (board[r][c] == opponent) opponent_pieces++;
            }
            
            // Thưởng cho việc kiểm soát cột
            if (player_pieces > opponent_pieces) {
                control_score += 5 * (player_pieces - opponent_pieces);
            }
        }
        
        score += control_score;
        
        // 4. Thưởng cho các mẫu chiến thuật đặc biệt
        for (int r = 0; r < ROWS-1; r++) {
            for (int c = 0; c < COLS-1; c++) {
                // Mẫu hình chữ L (L-shape pattern)
                if (r+2 < ROWS && c+1 < COLS) {
                    if (board[r][c] == player && 
                        board[r+1][c] == player && 
                        board[r+2][c] == 0 && 
                        board[r+2][c+1] == 0) {
                        score += 25;
                    }
                }
                
                // Mẫu đường dốc (Slope pattern)
                if (r+2 < ROWS && c+2 < COLS) {
                    if (board[r][c] == player && 
                        board[r+1][c+1] == player && 
                        board[r+2][c+2] == 0) {
                        score += 30;
                    }
                }
            }
        }
        
        return score;
    }

    static bool is_setting_up_opponent(const std::vector<std::vector<int>>& board, int col, int player) {
        int opponent = 3 - player;
        auto [new_board, row] = make_move(board, col, player);
        
        if (row == -1 || row == 0) return false; // Cột đã đầy hoặc là hàng trên cùng
        
        // Kiểm tra nếu đối thủ đặt quân phía trên
        auto [opp_board, opp_row] = make_move(new_board, col, opponent);
        
        if (opp_row != -1) {
            // Kiểm tra các kiểu thắng theo hàng ngang và chéo sau nước đi này của đối thủ
            
            // Kiểm tra hàng ngang
            int count = 1; // Đếm quân đối thủ liên tiếp theo hàng ngang
            
            // Kiểm tra bên trái
            for (int c = col - 1; c >= 0 && count < 4; c--) {
                if (opp_board[opp_row][c] == opponent) count++;
                else break;
            }
            
            // Kiểm tra bên phải
            for (int c = col + 1; c < COLS && count < 4; c++) {
                if (opp_board[opp_row][c] == opponent) count++;
                else break;
            }
            
            if (count >= 3) return true; // Nguy hiểm nếu đối thủ đã có 3+ quân liên tiếp
            
            // Kiểm tra các hướng chéo
            // Chéo xuống bên phải
            count = 1;
            for (int i = 1; i < 4; i++) {
                int r = opp_row + i;
                int c = col + i;
                if (r < ROWS && c < COLS && opp_board[r][c] == opponent) count++;
                else break;
            }
            for (int i = 1; i < 4; i++) {
                int r = opp_row - i;
                int c = col - i;
                if (r >= 0 && c >= 0 && opp_board[r][c] == opponent) count++;
                else break;
            }
            if (count >= 3) return true;
            
            // Chéo xuống bên trái
            count = 1;
            for (int i = 1; i < 4; i++) {
                int r = opp_row + i;
                int c = col - i;
                if (r < ROWS && c >= 0 && opp_board[r][c] == opponent) count++;
                else break;
            }
            for (int i = 1; i < 4; i++) {
                int r = opp_row - i;
                int c = col + i;
                if (r >= 0 && c < COLS && opp_board[r][c] == opponent) count++;
                else break;
            }
            if (count >= 3) return true;
        }
        
        return false;
    }

    static bool is_losing_move(const std::vector<std::vector<int>>& board, int col, int player) {
        int opponent = 3 - player;
        auto [new_board, row] = make_move(board, col, player);
        
        if (row == -1) return false; // Cột đã đầy
        
        // Kiểm tra xem sau nước đi này, đối thủ có thể thắng ngay không
        for (int c = 0; c < COLS; c++) {
            auto [opp_board, opp_row] = make_move(new_board, c, opponent);
            if (opp_row != -1 && check_winner(opp_board) == opponent) {
                // Đối thủ có thể thắng ở cột c sau nước đi này
                
                // Kiểm tra xem cột c có phải là "nước đi bắt buộc" không
                // (ví dụ như đối thủ đã có 3 quân liên tiếp và cần chặn)
                auto [test_board, _] = make_move(board, c, opponent);
                if (check_winner(test_board) == opponent) {
                    // Nếu đối thủ có thể thắng ngay lập tức ở cột c
                    // thì chúng ta phải chặn ở cột c, không phải lỗi nếu đánh ở col
                    continue;
                }
                
                // Kiểm tra xem nước đi tại col có tạo ra "nước đi bắt buộc" ở cột c không
                // Nếu là "nước đi bắt buộc" nghĩa là nó là hàng trống cuối cùng trước khi đối thủ có thể đặt vào
                if (opp_row == row - 1 && c == col) {
                    return true; // Đây là nước đi tự sát: ta đặt xuống và đối thủ đặt lên trên để thắng
                }
            }
        }
        
        return false;
    }

    static bool is_diagonal_threat(const std::vector<std::vector<int>>& board, int col, int player) {
        int opponent = 3 - player;
        int row = get_next_open_row(board, col);
        if (row == -1) return false;
        
        // Kiểm tra 4 hướng chéo
        const int dr[] = {-1, -1, 1, 1};
        const int dc[] = {-1, 1, -1, 1};
        
        for (int dir = 0; dir < 4; dir++) {
            int count = 0;
            // Đếm quân của đối thủ theo hướng này
            for (int i = 1; i <= 3; i++) {
                int nr = row + dr[dir] * i;
                int nc = col + dc[dir] * i;
                if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
                    if (board[nr][nc] == opponent) count++;
                    else break;
                }
            }
            
            // Đếm quân theo hướng ngược lại
            for (int i = 1; i <= 3; i++) {
                int nr = row - dr[dir] * i;
                int nc = col - dc[dir] * i;
                if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
                    if (board[nr][nc] == opponent) count++;
                    else break;
                }
            }
            
            // Nếu tổng số quân đếm được >= 3, đây là mối đe dọa lớn
            if (count >= 2) {
                return true;
            }
        }
        return false;
    }

    static bool is_dangerous_board(const std::vector<std::vector<int>>& board, int player) {
        int opponent = 3 - player;
        
        // Kiểm tra các thế trận nguy hiểm
        // 1. Đối thủ có hai vị trí đe dọa thắng
        int threat_count = 0;
        std::vector<int> threat_columns;
        
        for (int c = 0; c < COLS; c++) {
            auto [test_board, row] = make_move(board, c, opponent);
            if (row != -1 && check_winner(test_board) == opponent) {
                threat_count++;
                threat_columns.push_back(c);
            }
        }
        
        if (threat_count >= 2) {
            return true;
        }
        
        // 2. Kiểm tra các thế "bẫy" phổ biến của Connect 4
        for (int c = 0; c < COLS-1; c++) {
            if (c > 0) {
                // Kiểm tra mẫu "trapping pattern" như .O.
                                                  //OXO
                int r = get_next_open_row(board, c);
                if (r <= ROWS-2 && r >= 0) {
                    if (c > 0 && c < COLS-1) {
                        if (board[r+1][c-1] == opponent && 
                            board[r+1][c] == player && 
                            board[r+1][c+1] == opponent &&
                            board[r][c-1] == 0 && 
                            board[r][c+1] == 0) {
                            return true;
                        }
                    }
                }
                
                // Thêm mẫu bẫy mới: Thế tấn công hai phía (Sandwich attack)
                // X O . O X (với X là đối thủ, O là người chơi, . là vị trí trống)
                if (c > 1 && c < COLS-2) {
                    int r = get_next_open_row(board, c);
                    if (r >= 0) {
                        if (board[r][c-2] == opponent && 
                            board[r][c-1] == player && 
                            board[r][c+1] == player && 
                            board[r][c+2] == opponent) {
                            return true;
                        }
                    }
                }
                
                // Mẫu bẫy: Đường chéo tạo thành cổng (Diagonal gate)
                if (c > 0 && c < COLS-2 && r < ROWS-2 && r > 0) {
                    int r = get_next_open_row(board, c);
                    if (r >= 0) {
                        if (board[r+1][c-1] == opponent && 
                            board[r+1][c+1] == opponent && 
                            board[r+2][c] == opponent) {
                            return true;
                        }
                    }
                }
                
                // Mẫu bẫy: Kiểm tra bẫy "7" (7-trap)
                if (c < COLS-3 && r < ROWS-3 && r >= 0) {
                    int r = get_next_open_row(board, c);
                    if (r >= 0) {
                        if (board[r][c+1] == opponent && 
                            board[r][c+2] == opponent && 
                            board[r+1][c] == opponent && 
                            board[r+2][c] == opponent) {
                            return true;
                        }
                    }
                }
            }
        }
        
        // 3. Kiểm tra thế "bẫy chiến thuật tiếp theo" (next move trap)
        // Đối thủ có thể tạo thế 2 vị trí đe dọa trong nước đi tiếp theo
        for (int c = 0; c < COLS; c++) {
            auto [new_board, row] = make_move(board, c, opponent);
            if (row == -1) continue;
            
            int next_threat_count = 0;
            for (int next_c = 0; next_c < COLS; next_c++) {
                auto [next_board, next_row] = make_move(new_board, next_c, opponent);
                if (next_row != -1) {
                    // Đếm số vị trí đe dọa sau 2 nước đi
                    for (int test_c = 0; test_c < COLS; test_c++) {
                        auto [test_board, test_row] = make_move(next_board, test_c, opponent);
                        if (test_row != -1 && check_winner(test_board) == opponent) {
                            next_threat_count++;
                        }
                    }
                    if (next_threat_count >= 3) {
                        return true;
                    }
                }
            }
        }
        
        return false;
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
            if (get_next_open_row(board, c) != -1) {
                int score = SCORE_CENTER * (3 - std::min(std::abs(c - COLS/2), 3));
                
                // Đánh mạnh penalty nếu là nước đi tự sát
                if (player != 0 && is_losing_move(board, c, player)) {
                    score -= 50000; // Penalty rất lớn cho nước tự sát
                }
                
                // Phần còn lại của hàm đánh giá nước đi
                if (player != 0) {
                    // Simulate the move and evaluate position
                    auto [new_board, row] = make_move(board, c, player);
                    if (row != -1) {
                        // Add score for creating threats
                        score += count_open_threes(new_board, player) * 100;
                        
                        // Penalize moves that set up opponent
                        if (is_setting_up_opponent(board, c, player)) {
                            score -= 5000;
                        }
                        
                        // Prefer moves that block diagonal threats
                        if (is_diagonal_threat(board, c, player)) {
                            score += 200;
                        }
                        
                        // Check if move causes dangerous board
                        if (is_dangerous_board(new_board, player)) {
                            score -= 3000;
                        }
                        
                        // Bonus for moves that create multiple threats
                        int threat_count = 0;
                        for (int test_col = 0; test_col < COLS; test_col++) {
                            auto [test_board, test_row] = make_move(new_board, test_col, player);
                            if (test_row != -1 && check_winner(test_board) == player) {
                                threat_count++;
                            }
                        }
                        score += threat_count * 300;
                        
                        // Use the full position evaluation
                        score += evaluate_position(new_board, player) / 10;
                    }
                }
                
                scored_moves.push_back({c, score});
            }
        }
        
        // Sắp xếp theo điểm số
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
        
        // Improved move ordering - tính nhanh các nước đi đầy tiềm năng trước
        auto valid_moves = get_valid_moves(board, maximizing_player ? player : 3-player);
        
        // Thêm kiểm tra cắt tỉa nhanh (killers and history heuristic)
        // Kiểm tra xem có nước đi ngay lập tức tạo thành thắng/thua
        if (maximizing_player) {
            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, player);
                if (row != -1 && check_winner(new_board) == player) {
                    return {SCORE_WIN * 10, col}; // Trả về ngay nếu tìm thấy nước thắng
                }
            }
        } else {
            for (int col : valid_moves) {
                auto [new_board, row] = make_move(board, col, 3 - player);
                if (row != -1 && check_winner(new_board) == 3 - player) {
                    return {-SCORE_WIN * 10, col}; // Trả về ngay nếu tìm thấy nước thua
                }
            }
        }
        
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
        
        // Opening book moves (first few moves)
        if (piece_count <= 2) {
            // Mở đầu: ưu tiên cột giữa hoặc gần giữa
            int middle_col = COLS / 2;
            if (std::find(valid_moves.begin(), valid_moves.end(), middle_col) != valid_moves.end()) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Using opening book, middle column " << middle_col 
                          << ", Time: " << formatted_duration << "s" << std::endl;
                return {middle_col, SCORE_CENTER, 1, formatted_duration};
            }
        }
        
        // Check instant wins and blocks first
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
        
        // Check for dual threats (winning moves)
        for (int col : valid_moves) {
            auto [temp_board, row] = make_move(board, col, player);
            if (row == -1) continue;
            
            int winning_moves = 0;
            std::vector<int> winning_columns;
            for (int next_col : valid_moves) {
                if (next_col == col) continue;
                
                auto [next_board, next_row] = make_move(temp_board, next_col, player);
                if (next_row != -1 && check_winner(next_board) == player) {
                    winning_moves++;
                    winning_columns.push_back(next_col);
                }
            }
            
            if (winning_moves >= 2) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Creating dual threat at column " << col 
                          << " (winning moves: " << winning_columns[0] << ", " << winning_columns[1]
                          << "), Time: " << formatted_duration << "s" << std::endl;
                return {col, SCORE_WIN/4, 2, formatted_duration};
            }
        }
        
        // New: Check for prevention of opponent's dual threats
        for (int col : valid_moves) {
            auto [temp_board, row] = make_move(board, col, player);
            if (row == -1) continue;
            
            bool found_dual_threat = false;
            
            // Kiểm tra xem đối thủ có thể tạo ra dual threat trong nước đi tiếp theo không
            for (int opp_col : valid_moves) {
                if (opp_col == col) continue;
                
                auto [opp_board, opp_row] = make_move(board, opp_col, opponent);
                if (opp_row == -1) continue;
                
                int winning_moves = 0;
                for (int next_col : valid_moves) {
                    if (next_col == opp_col) continue;
                    
                    auto [next_board, next_row] = make_move(opp_board, next_col, opponent);
                    if (next_row != -1 && check_winner(next_board) == opponent) {
                        winning_moves++;
                    }
                }
                
                if (winning_moves >= 2) {
                    found_dual_threat = true;
                    break;
                }
            }
            
            // Nếu sau nước đi của chúng ta, đối thủ không thể tạo dual threat
            // thì đây là nước đi tốt để ngăn chặn đối thủ
            if (!found_dual_threat) {
                auto end_time = std::chrono::high_resolution_clock::now();
                float duration = std::chrono::duration<float>(end_time - start_time).count();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << duration;
                float formatted_duration = std::stof(ss.str());
                std::cout << timestamp << " Connect4AI: Preventing opponent's dual threat at column " << col 
                          << ", Time: " << formatted_duration << "s" << std::endl;
                return {col, SCORE_WIN/5, 2, formatted_duration};
            }
        }
        
        // New: Check for creating potential trap setups
        for (int col : valid_moves) {
            auto [temp_board, row] = make_move(board, col, player);
            if (row == -1) continue;
            
            // Kiểm tra nếu nước đi này tạo ra một "trap" (bẫy)
            int threat_count = 0;
            
            // Kiểm tra mẫu "bẫy chữ V" (v-trap) hoặc "bẫy chữ X" (x-trap)
            if (row > 0) {  // Có thể đặt thêm quân phía trên
                for (int c = 0; c < COLS; c++) {
                    if (c == col) continue;
                    
                    auto [next_board, next_row] = make_move(temp_board, c, player);
                    if (next_row != -1) {
                        for (int final_c = 0; final_c < COLS; final_c++) {
                            auto [final_board, final_row] = make_move(next_board, final_c, player);
                            if (final_row != -1 && check_winner(final_board) == player) {
                                threat_count++;
                            }
                        }
                    }
                }
                
                // Nếu có đủ đe dọa (tối thiểu 3 vị trí có thể thắng)
                if (threat_count >= 3) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    float duration = std::chrono::duration<float>(end_time - start_time).count();
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(6) << duration;
                    float formatted_duration = std::stof(ss.str());
                    std::cout << timestamp << " Connect4AI: Creating trap setup at column " << col 
                              << " (threats: " << threat_count << "), Time: " << formatted_duration << "s" << std::endl;
                    return {col, SCORE_WIN/6, 2, formatted_duration};
                }
            }
        }
        
        // Iterative deepening
        int max_depth = determine_depth(piece_count);
        int best_move = valid_moves[0];
        float best_score = -std::numeric_limits<float>::infinity();
        int final_depth = 1;
        
        const auto timeout = std::chrono::milliseconds(1500); // 1.5 second time limit
        
        for (int depth = 1; depth <= max_depth; depth++) {
            auto [score, move] = minimax(board, depth, -std::numeric_limits<float>::infinity(),
                                        std::numeric_limits<float>::infinity(), true, player);
            
            auto current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time) > timeout) {
                std::cout << timestamp << " Connect4AI: Time limit reached at depth " << depth << std::endl;
                break;
            }
            
            if (move != -1 && std::find(valid_moves.begin(), valid_moves.end(), move) != valid_moves.end()) {
                best_move = move;
                best_score = score;
                final_depth = depth;
            }
            
            // If we found a winning line, no need to search deeper
            if (score >= SCORE_WIN || score <= -SCORE_WIN) {
                break;
            }
        }
    
        auto end_time = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration<float>(end_time - start_time).count();
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << duration;
        float formatted_duration = std::stof(ss.str());
    
        std::cout << timestamp << " Connect4AI: Stage: " << piece_count << "/" << (ROWS * COLS)
                  << ", Depth: " << final_depth 
                  << ", Move: " << best_move 
                  << ", Score: " << best_score
                  << ", Time: " << formatted_duration << "s" << std::endl;
    
        return {best_move, static_cast<int>(best_score), final_depth, formatted_duration};
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