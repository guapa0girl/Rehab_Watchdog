/*
 * mock데이터 공급
 * 실시간 정현파 각도 데이터를 생성하여 공유 메모리에 저장
 * 공유 메모리 위치: ipc_config.h
 */
#include "ipc_config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

// 정현파 기본값 설정
#define BASE_ANGLE 35.0    // 중간값(중간에서의 offset)
#define AMP 25.0           // 진폭
#define PERIOD_SEC 4.0     // 한 사이클 4초
#define UPDATE_USEC 500000 // 0.5초 마다 업데이트

int main() {
  printf("---Sensor Simulator Started (Auto Sine-Wave Mode) ---\n");

  // shm에 연결
  int shmid = shmget(SHM_KEY_SENSOR, sizeof(sensor_shm_data_t), 0666);
  if (shmid < 0) {
    perror("shmget failed - Sensor");
    exit(EXIT_FAILURE);
  }

  sensor_shm_data_t *shm = shmat(shmid, NULL, 0);
  if (shm == (void *)-1) {
    perror("shmat failed - Sensor");
    exit(EXIT_FAILURE);
  }
  printf("[Sensor] Shared memory attached.\n");

  // 시간 기반 정현파 생성
  struct timespec ts;
  double angle;

  while (1) {
    // 현재 시간 가져오기
    clock_gettime(CLOCK_REALTIME, &ts);
    double t = ts.tv_sec + ts.tv_nsec / 1e9;

    // 정현파 생성: angle = 35+-25*sin()
    angle = BASE_ANGLE + AMP * sin((2 * M_PI / PERIOD_SEC) * t);

    // 공유 메모리에 angle 기록
    shm->joint_angle = angle;

    // 디버깅 출력
    printf("[Sensor] angle: %.2f\n", angle);

    usleep(UPDATE_USEC); // 0.5초
  }
  return 0;
}