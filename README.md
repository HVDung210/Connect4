# Connect4 AI

Một trí tuệ nhân tạo chơi game Connect4 được xây dựng bằng Python và C++, sử dụng thuật toán Minimax với Alpha-Beta Pruning.

## Tính năng

- AI thông minh sử dụng thuật toán Minimax với Alpha-Beta Pruning
- Tối ưu hóa hiệu suất bằng C++ thông qua pybind11
- API RESTful được xây dựng bằng FastAPI
- Hỗ trợ Docker để triển khai dễ dàng
- Tự động điều chỉnh độ sâu tìm kiếm dựa trên tình huống game
- Xử lý các trường hợp đặc biệt như nước đi thắng ngay lập tức

## Yêu cầu hệ thống

- Python 3.13+
- C++ compiler với hỗ trợ C++17
- CMake
- Docker (tùy chọn)

## Cài đặt

### Cài đặt trực tiếp

1. Clone repository:
```bash
git clone <repository-url>
cd connect4
```

2. Cài đặt các dependencies:
```bash
pip install -r requirements.txt
```

3. Biên dịch module C++:
```bash
python setup.py build_ext --inplace
```

### Sử dụng Docker

1. Build Docker image:
```bash
docker build -t connect4-ai .
```

2. Chạy container:
```bash
docker run -p 8080:8080 connect4-ai
```

## Sử dụng API

API cung cấp các endpoint sau:

### Health Check
```http
GET /health
```
Kiểm tra trạng thái hoạt động của service.

### Lấy nước đi từ AI
```http
POST /api/connect4-move
```

Request body:
```json
{
    "board": [[0,0,0,0,0,0,0], ...],  // 6x7 matrix
    "current_player": 1,              // 1 hoặc 2
    "valid_moves": [0,1,2,3,4,5,6],  // Các cột có thể đi
    "is_new_game": false             // Tùy chọn
}
```

Response:
```json
{
    "move": 3,           // Cột được chọn (0-6)
    "evaluation": 100,   // Đánh giá vị trí
    "depth": 5,         // Độ sâu tìm kiếm
    "execution_time": 0.123  // Thời gian tính toán (giây)
}
```

## Cấu trúc dự án

- `app.py`: FastAPI server và xử lý logic game
- `module_ai.cpp`: Module C++ chứa thuật toán AI
- `setup.py`: Cấu hình build cho module C++
- `Dockerfile`: Cấu hình Docker
- `requirements.txt`: Python dependencies

## Thuật toán AI

### Thuật toán Minimax với Alpha-Beta Pruning

AI sử dụng thuật toán Minimax với Alpha-Beta Pruning, một phương pháp tìm kiếm theo chiều sâu trong không gian trạng thái của game. Thuật toán này hoạt động bằng cách:

1. **Tìm kiếm theo chiều sâu**: Duyệt qua các nước đi có thể xảy ra trong tương lai
2. **Đánh giá vị trí**: Sử dụng hàm heuristic để đánh giá lợi thế của mỗi vị trí
3. **Cắt tỉa Alpha-Beta**: Loại bỏ các nhánh không cần thiết để tăng hiệu suất

### Các tối ưu hóa

1. **Điều chỉnh độ sâu thông minh**:
   - Giai đoạn đầu (≤10 quân): độ sâu 5
   - Giai đoạn giữa (≤20 quân): độ sâu 7
   - Giai đoạn cuối (≤30 quân): độ sâu 8-10
   - Tăng thêm 2-3 độ sâu khi phát hiện mối đe dọa/cơ hội

2. **Hàm đánh giá nâng cao**:
   - Ưu tiên vị trí trung tâm (điểm cao hơn cho cột 3,4)
   - Phát hiện và đánh giá các cửa sổ 4 ô liên tiếp
   - Xử lý đặc biệt cho "open three" (3 quân liên tiếp có thể thắng)
   - Phân tích mối đe dọa từ đối thủ

3. **Xử lý trường hợp đặc biệt**:
   - Kiểm tra nước đi thắng ngay lập tức
   - Chặn nước đi thắng của đối thủ
   - Ưu tiên nước đi trung tâm trong ván mới

### Ưu điểm

1. **Hiệu quả cao**:
   - Tìm được nước đi tối ưu trong thời gian hợp lý
   - Cắt tỉa Alpha-Beta giảm đáng kể số lượng nút cần xét
   - Tự động điều chỉnh độ sâu phù hợp với tình huống

2. **Thông minh và linh hoạt**:
   - Có thể xử lý nhiều tình huống phức tạp
   - Học hỏi từ các mẫu thắng/thua
   - Thích nghi với phong cách chơi của đối thủ

3. **Hiệu suất tốt**:
   - Tối ưu hóa bằng C++ thông qua pybind11
   - Thời gian phản hồi nhanh (thường < 1 giây)
   - Sử dụng ít tài nguyên hệ thống

### Nhược điểm

1. **Giới hạn tính toán**:
   - Không thể xem xét tất cả các khả năng trong trò chơi
   - Độ sâu tìm kiếm bị giới hạn bởi thời gian
   - Có thể bỏ sót các nước đi tốt ở độ sâu lớn

2. **Phụ thuộc vào hàm đánh giá**:
   - Chất lượng phụ thuộc vào độ chính xác của hàm heuristic
   - Có thể đánh giá sai trong một số tình huống đặc biệt
   - Khó tinh chỉnh các tham số đánh giá

3. **Không có khả năng học**:
   - Không cải thiện qua thời gian
   - Không thể học từ các ván đã chơi
   - Cần cập nhật thủ công các tham số

### So sánh với các phương pháp khác

1. **So với Monte Carlo Tree Search (MCTS)**:
   - Minimax: Tìm kiếm có định hướng, hiệu quả cho game có luật rõ ràng
   - MCTS: Tìm kiếm ngẫu nhiên, tốt cho game phức tạp và không gian trạng thái lớn
   - Kết luận: Minimax phù hợp hơn cho Connect4 do luật game đơn giản và không gian trạng thái có thể quản lý được

2. **So với Deep Learning**:
   - Minimax: Dựa trên quy tắc và tìm kiếm, không cần dữ liệu huấn luyện
   - Deep Learning: Cần nhiều dữ liệu huấn luyện, có thể học các chiến thuật phức tạp
   - Kết luận: Minimax đơn giản và hiệu quả hơn cho Connect4, trong khi Deep Learning có thể quá phức tạp không cần thiết

3. **So với Rule-based AI**:
   - Minimax: Tìm kiếm động và thích nghi với tình huống
   - Rule-based: Dựa trên các quy tắc cố định, dễ dự đoán
   - Kết luận: Minimax linh hoạt và mạnh mẽ hơn, có thể xử lý nhiều tình huống phức tạp

### Kết luận

Thuật toán Minimax với Alpha-Beta Pruning là lựa chọn tối ưu cho Connect4 AI vì:
- Phù hợp với không gian trạng thái của game
- Có thể tìm được nước đi tốt trong thời gian hợp lý
- Dễ dàng tối ưu hóa và tinh chỉnh
- Không yêu cầu dữ liệu huấn luyện hay tài nguyên tính toán lớn

