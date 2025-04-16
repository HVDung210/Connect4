from fastapi import FastAPI, HTTPException
import uvicorn
from pydantic import BaseModel
from typing import List, Optional, Tuple
from fastapi.middleware.cors import CORSMiddleware
import math
import time

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Hằng số game
ROWS = 6
COLS = 7
WIN_LENGTH = 4
MAX_DEPTH = 6  # Giữ độ sâu 6 từ cải tiến trước

class GameState(BaseModel):
    board: List[List[int]]
    current_player: int
    valid_moves: List[int]

class AIResponse(BaseModel):
    move: int
    evaluation: Optional[int] = None
    depth: Optional[int] = None
    execution_time: Optional[float] = None

class Connect4AI:
    @staticmethod
    def evaluate_window(window: List[int], player: int) -> int:
        """Đánh giá một cửa sổ 4 ô liên tiếp"""
        opponent = 3 - player
        score = 0
        
        if window.count(player) == 4:
            score += 100000  # Tăng trọng số
        elif window.count(player) == 3 and window.count(0) == 1:
            score += 100     # Tăng trọng số
        elif window.count(player) == 2 and window.count(0) == 2:
            score += 10      # Tăng trọng số
            
        if window.count(opponent) == 3 and window.count(0) == 1:
            score -= 80      # Phạt nặng hơn
            
        return score

    @staticmethod
    def get_winning_lines() -> List[List[Tuple[int, int]]]:
        """Lấy tất cả các đường có thể thắng"""
        lines = []
        # Horizontal
        for r in range(ROWS):
            for c in range(COLS - 3):
                lines.append([(r, c + i) for i in range(4)])
        # Vertical
        for c in range(COLS):
            for r in range(ROWS - 3):
                lines.append([(r + i, c) for i in range(4)])
        # Diagonal /
        for r in range(ROWS - 3):
            for c in range(COLS - 3):
                lines.append([(r + i, c + i) for i in range(4)])
        # Diagonal \
        for r in range(3, ROWS):
            for c in range(COLS - 3):
                lines.append([(r - i, c + i) for i in range(4)])
        return lines

    @staticmethod
    def count_open_threes(board: List[List[int]], player: int) -> int:
        """Đếm số lượng 'open threes' cho người chơi"""
        count = 0
        winning_lines = Connect4AI.get_winning_lines()
        for line in winning_lines:
            pieces = [board[r][c] for r, c in line]
            if pieces.count(player) == 3 and pieces.count(0) == 1:
                # Find the empty position
                for idx, piece in enumerate(pieces):
                    if piece == 0:
                        r, c = line[idx]
                        # Check if this is the lowest empty row in column c
                        r_empty = Connect4AI.get_next_open_row(board, c)
                        if r_empty == r:
                            count += 1
                            break
        return count

    @staticmethod
    def evaluate_position(board: List[List[int]], player: int) -> int:
        """Đánh giá toàn bộ bảng cho người chơi hiện tại"""
        score = 0
        
        # Ưu tiên cột giữa (cột 4: +3, cột 3 và 5: +2)
        for c in range(COLS):
            for r in range(ROWS):
                if board[r][c] == player:
                    if c == 3:  # Cột 4
                        score += 3
                    elif c in [2, 4]:  # Cột 3 và 5
                        score += 2
        
        # Đánh giá hàng ngang
        for r in range(ROWS):
            for c in range(COLS - 3):
                window = [board[r][c + i] for i in range(4)]
                score += Connect4AI.evaluate_window(window, player)
        
        # Đánh giá hàng dọc
        for c in range(COLS):
            for r in range(ROWS - 3):
                window = [board[r + i][c] for i in range(4)]
                score += Connect4AI.evaluate_window(window, player)
        
        # Đánh giá đường chéo /
        for r in range(ROWS - 3):
            for c in range(COLS - 3):
                window = [board[r + i][c + i] for i in range(4)]
                score += Connect4AI.evaluate_window(window, player)
        
        # Đánh giá đường chéo \
        for r in range(3, ROWS):
            for c in range(COLS - 3):
                window = [board[r - i][c + i] for i in range(4)]
                score += Connect4AI.evaluate_window(window, player)
        
        # Đánh giá các mối đe dọa (open threes)
        player_threes = Connect4AI.count_open_threes(board, player)
        opponent = 3 - player
        opponent_threes = Connect4AI.count_open_threes(board, opponent)
        
        if player_threes >= 2:
            score += 10000  # Tăng trọng số
        elif opponent_threes >= 2:
            score -= 10000  # Tăng trọng số
        else:
            score += player_threes * 100 - opponent_threes * 100  # Tăng trọng số
        
        return score

    @staticmethod
    def is_terminal_node(board: List[List[int]]) -> bool:
        """Kiểm tra xem trò chơi đã kết thúc chưa"""
        return Connect4AI.check_winner(board) != 0 or Connect4AI.is_board_full(board)

    @staticmethod
    def check_winner(board: List[List[int]]) -> int:
        """Kiểm tra người chiến thắng"""
        # Kiểm tra hàng ngang
        for r in range(ROWS):
            for c in range(COLS - WIN_LENGTH + 1):
                if board[r][c] != 0 and all(board[r][c] == board[r][c+i] for i in range(1, WIN_LENGTH)):
                    return board[r][c]
        
        # Kiểm tra hàng dọc
        for r in range(ROWS - WIN_LENGTH + 1):
            for c in range(COLS):
                if board[r][c] != 0 and all(board[r][c] == board[r+i][c] for i in range(1, WIN_LENGTH)):
                    return board[r][c]
        
        # Kiểm tra đường chéo xuống
        for r in range(ROWS - WIN_LENGTH + 1):
            for c in range(COLS - WIN_LENGTH + 1):
                if board[r][c] != 0 and all(board[r][c] == board[r+i][c+i] for i in range(1, WIN_LENGTH)):
                    return board[r][c]
        
        # Kiểm tra đường chéo lên
        for r in range(WIN_LENGTH - 1, ROWS):
            for c in range(COLS - WIN_LENGTH + 1):
                if board[r][c] != 0 and all(board[r][c] == board[r-i][c+i] for i in range(1, WIN_LENGTH)):
                    return board[r][c]
        
        return 0

    @staticmethod
    def is_board_full(board: List[List[int]]) -> bool:
        """Kiểm tra xem bảng đã đầy chưa"""
        return all(board[0][c] != 0 for c in range(COLS))

    @staticmethod
    def get_valid_moves(board: List[List[int]]) -> List[int]:
        """Lấy danh sách các nước đi hợp lệ, ưu tiên cột giữa"""
        valid_moves = [c for c in range(COLS) if board[0][c] == 0]
        # Sắp xếp để ưu tiên cột giữa trước
        valid_moves.sort(key=lambda x: abs(x - COLS//2))
        return valid_moves

    @staticmethod
    def get_next_open_row(board: List[List[int]], col: int) -> int:
        """Tìm hàng còn trống thấp nhất trong cột"""
        for r in range(ROWS-1, -1, -1):
            if board[r][col] == 0:
                return r
        return -1

    @staticmethod
    def make_move(board: List[List[int]], col: int, player: int) -> Tuple[List[List[int]], int]:
        """Thực hiện nước đi và trả về bảng mới cùng hàng được điền"""
        row = Connect4AI.get_next_open_row(board, col)
        if row == -1:
            return (board, -1)
        
        new_board = [row[:] for row in board]
        new_board[row][col] = player
        return (new_board, row)

    @staticmethod
    def minimax(board: List[List[int]], depth: int, alpha: float, beta: float, maximizing_player: bool, player: int) -> Tuple[int, Optional[int]]:
        """Thuật toán Minimax với Alpha-Beta Pruning"""
        valid_moves = Connect4AI.get_valid_moves(board)
        is_terminal = Connect4AI.is_terminal_node(board)
        
        if depth == 0 or is_terminal:
            if is_terminal:
                winner = Connect4AI.check_winner(board)
                if winner == player:
                    return (100000000000000, None)
                elif winner == 3 - player:
                    return (-100000000000000, None)
                else:  # Hòa
                    return (0, None)
            else:  # Độ sâu = 0
                return (Connect4AI.evaluate_position(board, player), None)
        
        if maximizing_player:
            value = -math.inf
            best_move = valid_moves[0] if valid_moves else None
            
            for col in valid_moves:
                new_board, _ = Connect4AI.make_move(board, col, player)
                new_score, _ = Connect4AI.minimax(new_board, depth-1, alpha, beta, False, player)
                
                if new_score > value:
                    value = new_score
                    best_move = col
                
                alpha = max(alpha, value)
                if alpha >= beta:
                    break
                    
            return (value, best_move)
        else:  # Minimizing player
            value = math.inf
            best_move = valid_moves[0] if valid_moves else None
            
            for col in valid_moves:
                new_board, _ = Connect4AI.make_move(board, col, 3 - player)
                new_score, _ = Connect4AI.minimax(new_board, depth-1, alpha, beta, True, player)
                
                if new_score < value:
                    value = new_score
                    best_move = col
                
                beta = min(beta, value)
                if alpha >= beta:
                    break
                    
            return (value, best_move)

@app.post("/api/connect4-move")
async def make_move(game_state: GameState) -> AIResponse:
    try:
        if not game_state.valid_moves:
            raise ValueError("Không có nước đi hợp lệ")
        
        start_time = time.time()
        
        # 1. Kiểm tra nước thắng ngay lập tức
        for col in game_state.valid_moves:
            new_board, row = Connect4AI.make_move(game_state.board, col, game_state.current_player)
            if row != -1 and Connect4AI.check_winner(new_board) == game_state.current_player:
                return AIResponse(
                    move=col,
                    evaluation=100,
                    depth=0,
                    execution_time=time.time() - start_time
                )
        
        # 2. Ngăn chặn đối thủ thắng ở nước tiếp theo
        opponent = 3 - game_state.current_player
        for col in game_state.valid_moves:
            new_board, row = Connect4AI.make_move(game_state.board, col, opponent)
            if row != -1 and Connect4AI.check_winner(new_board) == opponent:
                return AIResponse(
                    move=col,
                    evaluation=-100,
                    depth=0,
                    execution_time=time.time() - start_time
                )
        
        # 3. Sử dụng Minimax với Alpha-Beta Pruning
        score, best_move = Connect4AI.minimax(
            game_state.board,
            MAX_DEPTH,
            -math.inf,
            math.inf,
            True,
            game_state.current_player
        )
        
        # Fallback nếu không tìm được nước đi tốt
        if best_move is None or best_move not in game_state.valid_moves:
            best_move = game_state.valid_moves[0]
        
        return AIResponse(
            move=best_move,
            evaluation=score,
            depth=MAX_DEPTH,
            execution_time=time.time() - start_time
        )
        
    except Exception as e:
        if game_state.valid_moves:
            return AIResponse(move=game_state.valid_moves[0])
        raise HTTPException(status_code=400, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)