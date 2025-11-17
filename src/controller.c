/*
 * 핵심 제어 및 heartbeat
 * 공유 메모리에서 데이터를 읽어 재활 판단 로직을 수행.
 * watchdog에게 주기적으로 생존 신호를 전송
 */

/*
 * 공유 메모리에서 데이터를 읽기
 * 재활 판단 로직 수행
 * watchdog에게 주기적으로 생존 신호 전송
 */

/*
 * 공유 메모리 데이터 sharedData(구조체?)
 * 생존 신호: heartBeat
 * watchdog에게 어떻게 전송?
 * 시간 주기: timeout
 */

#include "ipc_config.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

// 임계값 설정
#define NORMAL_LOW 10
#define NORMAL_HIGH 55
#define WARN_LOW 5
#define WARN_HIGH 65

int main() {

  // 1. 공유 메모리 연결
  int shmid = shmget(SHM_KEY_SENSOR, sizeof(sensor_shm_data_t), 0666);
  sensor_shm_data_t *shm = shmat(shmid, NULL, 0);

  // 2. Controller PID 기록
  shm->controller_pid = getpid();

  // 3. Watchdog PID 읽기
  pid_t wpid = shm->watchdog_pid;

  while (1) {
    double angle = shm->joint_angle;

    // --- 4. 임계값 검사 ---
    if (angle >= NORMAL_LOW && angle <= NORMAL_HIGH) {
      shm->is_safe = 0; // 정상
    } else if ((angle >= WARN_LOW && angle < NORMAL_LOW) ||
               (angle > NORMAL_HIGH && angle <= WARN_HIGH)) {
      shm->is_safe = 1; // 경고
    } else {
      shm->is_safe = -1; // 비정상
    }

    // --- 5. 에러 상태일 때 heartbeat 중단 ---
    if (shm->is_safe != -1) {
      kill(wpid, SIGUSR1); // heartbeat 전송
    }

    usleep(500000); // 0.5초 간격
  }

  return 0;
}
