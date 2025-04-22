FROM python:3.13-slim

# Tạo thư mục làm việc
WORKDIR /app

# Cài đặt các gói cần thiết cho việc biên dịch C++
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

ENV PYTHONUNBUFFERED=1

# Sao chép requirements trước để tận dụng lớp cache
COPY requirements.txt .
RUN pip install --upgrade pip
RUN pip install -r requirements.txt

# Cài đặt pybind11 để biên dịch module C++
RUN pip install pybind11

# Sao chép mã nguồn
COPY . .

# Biên dịch module C++ thành module Python
RUN c++ -O3 -Wall -shared -std=c++11 -fPIC \
    $(python3 -m pybind11 --includes) \
    module_ai.cpp -o module_ai$(python3-config --extension-suffix)

# Mở cổng động từ biến môi trường
EXPOSE $PORT

# Chạy ứng dụng
CMD uvicorn app:app --host 0.0.0.0 --port $PORT