# 프로젝트 변수 설정
CC = gcc
TARGET = rehab_sys
SRCS = $(wildcard ./src/*.c)
CFLAGS = -Iinclude

.PHONY: all clean

# 1. 모든 소스 파일을 컴파일하여 실행 파일 생성
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo "--- Build Complete: $(TARGET) created ---"

# 2. 정리
clean:
	@rm -f $(TARGET)
	@echo "--- Clean complete ---"
