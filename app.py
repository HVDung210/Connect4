from fastapi import FastAPI, HTTPException
import uvicorn
from pydantic import BaseModel
from typing import List, Optional
from fastapi.middleware.cors import CORSMiddleware
import module_ai
import logging

# Thiết lập logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI()

# Thiết lập CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class GameState(BaseModel):
    board: List[List[int]]
    current_player: int
    valid_moves: List[int]
    is_new_game: Optional[bool] = False

class AIResponse(BaseModel):
    move: int
    evaluation: Optional[int] = None
    depth: Optional[int] = None
    execution_time: Optional[float] = None

class HealthResponse(BaseModel):
    status: str
    message: str

def is_board_empty(board: List[List[int]]) -> bool:
    """Kiểm tra xem bảng có trống hoàn toàn hay không."""
    return all(cell == 0 for row in board for cell in row)

def generate_valid_moves(board: List[List[int]]) -> List[int]:
    """Tạo danh sách các cột hợp lệ (các cột có ô trống ở hàng trên cùng)."""
    return [col for col in range(7) if board[0][col] == 0]

@app.get("/health")
async def health_check():
    """Health check endpoint cho giám sát triển khai."""
    return HealthResponse(status="ok", message="Service is running")

@app.post("/api/connect4-move")
async def make_move(game_state: GameState) -> AIResponse:
    try:
        # Ghi log trạng thái đầu vào
        logger.info(f"Received game state: is_new_game={game_state.is_new_game}, "
                   f"current_player={game_state.current_player}, valid_moves={game_state.valid_moves}")

        # Xử lý trường hợp nước đi đầu tiên của ván mới
        if game_state.is_new_game:
            if not is_board_empty(game_state.board):
                logger.warning("is_new_game is True but board is not empty")
                # Chuyển sang sử dụng minimax nếu bảng không trống
                move, evaluation, depth, execution_time = module_ai.Connect4AI.get_best_move(
                    game_state.board,
                    game_state.current_player,
                    game_state.valid_moves
                )
                return AIResponse(
                    move=move,
                    evaluation=evaluation,
                    depth=depth,
                    execution_time=execution_time
                )

            logger.info("New game detected: Applying first move strategy")
            # Trong bảng trống, tất cả cột đều hợp lệ
            valid_moves = generate_valid_moves(game_state.board)
            if not valid_moves:
                logger.error("No valid moves available in new game")
                raise ValueError("Không có nước đi hợp lệ trong ván mới")

            # Ưu tiên cột giữa (cột 3)
            if 3 in valid_moves:
                logger.info("Selecting center column (3) for new game")
                return AIResponse(move=3)
            # Nếu cột giữa không khả dụng (hiếm), chọn cột gần trung tâm nhất
            for col in [2, 4, 1, 5, 0, 6]:
                if col in valid_moves:
                    logger.info(f"Center column unavailable, selecting column {col}")
                    return AIResponse(move=col)

        # Kiểm tra valid_moves
        if not game_state.valid_moves:
            logger.warning("No valid moves provided, generating from board")
            game_state.valid_moves = generate_valid_moves(game_state.board)
            if not game_state.valid_moves:
                logger.error("No valid moves available")
                raise ValueError("Không có nước đi hợp lệ")

        # Sử dụng AI để chọn nước đi trong các trường hợp khác
        logger.info("Using AI to determine best move")
        move, evaluation, depth, execution_time = module_ai.Connect4AI.get_best_move(
            game_state.board,
            game_state.current_player,
            game_state.valid_moves
        )

        return AIResponse(
            move=move,
            evaluation=evaluation,
            depth=depth,
            execution_time=execution_time
        )

    except Exception as e:
        logger.error(f"Error in make_move: {str(e)}")
        # Fallback: chọn nước đi đầu tiên từ valid_moves
        if game_state.valid_moves:
            fallback_move = game_state.valid_moves[0]
            logger.info(f"Falling back to first valid move: {fallback_move}")
            return AIResponse(move=fallback_move)
        raise HTTPException(status_code=400, detail=str(e))

if __name__ == "__main__":
    import os
    port = int(os.environ.get("PORT", 8080))
    uvicorn.run(app, host="0.0.0.0", port=port)