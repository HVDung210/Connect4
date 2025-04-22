from fastapi import FastAPI, HTTPException
import uvicorn
from pydantic import BaseModel
from typing import List, Optional
from fastapi.middleware.cors import CORSMiddleware
import module_ai

app = FastAPI()

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

@app.post("/api/connect4-move")
async def make_move(game_state: GameState) -> AIResponse:
    try:
        if not game_state.valid_moves:
            raise ValueError("Không có nước đi hợp lệ")

        # Xử lý trường hợp nước đi đầu tiên của ván mới
        if game_state.is_new_game and game_state.current_player == 1:
            # Nước đi tốt nhất ở lượt đầu là cột giữa (cột 3)
            if 3 in game_state.valid_moves:
                return AIResponse(move=3)
            # Nếu cột giữa không khả dụng (hiếm khi xảy ra), chọn cột gần giữa nhất
            for col in [2, 4, 1, 5, 0, 6]:
                if col in game_state.valid_moves:
                    return AIResponse(move=col)
        
        # Sử dụng AI để chọn nước đi trong các trường hợp khác
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
        print("Error occurred in make_move:", str(e))
        # Fallback: nếu có lỗi, chọn nước đi đầu tiên từ các nước đi hợp lệ
        if game_state.valid_moves:
            return AIResponse(move=game_state.valid_moves[0])
        raise HTTPException(status_code=400, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)