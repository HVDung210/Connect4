FROM python:3.13-slim

# Tạo thư mục làm việc
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

ENV PYTHONUNBUFFERED=1

# Sao chép requirements trước để tận dụng lớp cache
COPY requirements.txt .
RUN pip install --upgrade pip
RUN pip install --r requirements.txt

# Sao chép mã nguồn
COPY . .

EXPOSE $PORT

CMD uvicorn app:app --host 0.0.0.0 --port $PORT