FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libsfml-dev \
    libsqlite3-dev \
    fonts-dejavu \
    file && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# СОЗДАЕМ ПАПКИ
RUN mkdir -p database feedback

# ДЕТАЛЬНАЯ ПРОВЕРКА ЗВУКОВЫХ ФАЙЛОВ
RUN echo "=== ДЕТАЛЬНАЯ ПРОВЕРКА ЗВУКОВЫХ ФАЙЛОВ ===" && \
    if [ -d "assets/sounds" ]; then \
        echo "✅ Папка sounds найдена" && \
        echo "Содержимое:" && \
        ls -la assets/sounds/ && \
        echo "" && \
        echo "Типы файлов:" && \
        for f in assets/sounds/*.wav; do \
            if [ -f "\$f" ]; then \
                echo "  \$f:" && \
                file "\$f" | head -c 100; \
                echo "" && \
                echo "  Размер: \$(stat -c%s "\$f") байт"; \
            fi; \
        done; \
    else \
        echo "❌ Папка sounds не найдена"; \
    fi

# СБОРКА
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

CMD ["./build/memory_game"]
