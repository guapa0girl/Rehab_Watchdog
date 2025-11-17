# --- 환경 변수 설정 ---
CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lm # 수학 라이브러리 링크를 위한 플래그

# --- 대상 파일 정의 ---
TARGETS = main watchdog controller sensor_sim
SRCS = ./src/main.c ./src/watchdog.c ./src/controller.c ./src/sensor_sim.c

# --- 기본 빌드 목표 (make) ---
all: $(TARGETS)

# --- 1. main.c 컴파일 ---
main: ./src/main.c
	$(CC) $(CFLAGS) $< -o $@

# --- 2. watchdog.c 컴파일 ---
watchdog: ./src/watchdog.c
	$(CC) $(CFLAGS) $< -o $@

# --- 3. controller.c 컴파일 ---
controller: ./src/controller.c
	$(CC) $(CFLAGS) $< -o $@

# --- 4. sensor_sim.c 컴파일 (수학 라이브러리(-lm) 필수) ---
sensor_sim: ./src/sensor_sim.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# --- 정리 목표 (make clean) ---
clean:
	rm -f $(TARGETS)

.PHONY: all clean
