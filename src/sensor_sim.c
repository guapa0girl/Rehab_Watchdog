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
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define BASE_ANGLE 35.0    // 정현파 중심값
#define AMP 25.0           // 정현파 진폭 (10~60도 범위)
#define PERIOD_SEC 4.0     // 한 주기 = 4초
#define UPDATE_USEC 500000 // 0.5초마다 갱신

#define ERROR_CHANCE 5 // 5% 확률로 비정상(재활 위험) 각도 생성

/* -----------------------------
 * 세마포어 P 연산 (잠금)
 * -1을 더하므로 세마포어 값이 1 → 0 이 되며 접근 권한 획득
 * 다른 프로세스는 잠금 해제될 때까지 접근 불가
 * -----------------------------*/
void sem_lock(int semid) {
  struct sembuf p = {0, -1, SEM_UNDO};
  semop(semid, &p, 1);
}

/* -----------------------------
 * 세마포어 V 연산 (해제)
 * +1을 더하므로 세마포어 값이 0 → 1 이 되며 접근 권한 반환
 * 상대 프로세스가 접근 가능해짐
 * -----------------------------*/
void sem_unlock(int semid) {
  struct sembuf v = {0, +1, SEM_UNDO};
  semop(semid, &v, 1);
}

int main() {
  printf("[Sensor] Sensor Simulator Started (Semaphore + Sine Wave)\n");

  /* ----------------------------------------
   * 1. 공유 메모리 생성 (IPC_CREAT 포함)
   * Sensor가 최초 생성자(Producer 역할)
   * ----------------------------------------*/
  int shmid = shmget(SHM_KEY_SENSOR, sizeof(sensor_shm_data_t), 0666);
  if (shmid < 0) {
    perror("[Sensor] shmget");
    exit(1);
  }

  /* ----------------------------------------
   * 2. 공유 메모리 attach (연결)
   * Sensor와 Controller 둘 다 동일한 shm을 바라봄
   * ----------------------------------------*/
  sensor_shm_data_t *shm = shmat(shmid, NULL, 0);
  if (shm == (void *)-1) {
    perror("[Sensor] shmat");
    exit(1);
  }

  /* ----------------------------------------
   * 3. 세마포어 생성
   * Sensor가 initialize (값 = 1)
   * Controller는 생성된 세마포어를 사용
   * ----------------------------------------*/
  int semid = semget(SEM_KEY_SYNC, 1, 0666);
  if (semid < 0) {
    perror("[Sensor] semget");
    exit(1);
  }

  // 세마포어 값 초기화 (최초 실행 시에만 의미 있음)
  semctl(semid, 0, SETVAL, 1);

  printf("[Sensor] Shared Memory + Semaphore Ready.\n");

  srand(time(NULL));
  struct timespec ts;
  double angle;

  /* ----------------------------------------
   * 4. 무한 루프: 센서값 지속 생성
   * Controller는 다음 센서값이 올 때까지 계속 읽어감
   * ----------------------------------------*/
  while (1) {

    /* -----------------------------
     * 4-1. 시간 기반 정현파 생성
     * sin함수로 자연스러운 손목 굴곡/신전 모델링
     * -----------------------------*/
    clock_gettime(CLOCK_REALTIME, &ts);
    double t = ts.tv_sec + ts.tv_nsec / 1e9;
    angle = BASE_ANGLE + AMP * sin((2 * M_PI / PERIOD_SEC) * t);

    /* -----------------------------
     * 4-2. 재활 위험각도(비정상) 발생
     * 5% 확률로 실제 치료에서 위험한 움직임을 표현
     * -----------------------------*/
    if (rand() % 100 < ERROR_CHANCE) {
      if (rand() % 2 == 0)
        angle = (rand() % 4) + 1; // 1~4도 → 과도 굴곡(위험)
      else
        angle = (rand() % 15) + 70; // 70~85도 → 과도 신전(위험)

      printf("[Sensor] ** Abnormal Movement Detected: %.2f **\n", angle);
    } else {
      printf("[Sensor] angle = %.2f\n", angle);
    }

    /* -----------------------------
     * 4-3. 세마포어 잠금 → shm 쓰기
     * Sensor가 단독으로 shm에 접근하는 순간
     * -----------------------------*/
    sem_lock(semid);

    shm->joint_angle = angle; // Sensor는 오직 angle만 기록

    sem_unlock(semid);
    /* -----------------------------
     * 4-4. 잠금 해제 (Controller에게 접근권 넘김)
     * -----------------------------*/

    usleep(UPDATE_USEC); // 0.5초 대기 후 다음 값 생성
  }

  return 0;
}
