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

/*
 * 동시성 처리해야함
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "ipc_config.h"

// 세마포어 P 연산 (잠금)
void sem_lock(int semid) {
  struct sembuf p = {0, -1, SEM_UNDO};
  semop(semid, &p, 1);
}

// 세마포어 V 연산 (해제)
void sem_unlock(int semid) {
  struct sembuf v = {0, +1, SEM_UNDO};
  semop(semid, &v, 1);
}

// 각도 기반 재활 안전성 판단 함수
int check_safety(double angle) {
  if (angle < 5 || angle > 65)
    return -1; // 위험
  else if ((angle >= 5 && angle < 10) || (angle > 55 && angle <= 65))
    return 1; // 경고
  else
    return 0; // 정상
}

int main(int argc, char *argv[]) {
  printf("[Controller] Started.\n");

  // ===== 1. SHM 연결 =====
  int shmid = shmget(SHM_KEY_SENSOR, sizeof(sensor_shm_data_t), 0666);
  if (shmid < 0) {
    perror("[Controller] shmget");
    exit(1);
  }

  sensor_shm_data_t *shm = shmat(shmid, NULL, 0);
  if (shm == (void *)-1) {
    perror("[Controller] shmat");
    exit(1);
  }

  // ===== 2. 세마포어 연결 =====
  int semid = semget(SEM_KEY_SYNC, 1, 0666);
  if (semid < 0) {
    perror("[Controller] semget");
    exit(1);
  }

  // ===== 3. Controller PID 기록 =====
  pid_t my_pid = getpid();
  shm->controller_pid = my_pid;
  printf("[Controller] my PID = %d\n", my_pid);

  // ===== 4. Watchdog PID 읽기 =====
  // watchdog이 먼저 shm에 자신의 pid 기록해줘야 한다
  while (shm->watchdog_pid == 0) {
    printf("[Controller] Waiting for Watchdog PID...\n");
    usleep(300000);
  }
  pid_t watchdog_pid = shm->watchdog_pid;
  printf("[Controller] Watchdog PID = %d\n", watchdog_pid);

  // ===== 5. 메인 루프 시작 =====
  while (1) {

    // === 5-1. Sensor → angle 읽기 (세마포어로 보호) ===
    sem_lock(semid);
    double angle = shm->joint_angle;
    sem_unlock(semid);

    // === 5-2. 안전성 판단 ===
    int state = check_safety(angle);
    shm->is_safe = state;

    // === 5-3. 상태 출력 ===
    if (state == 0)
      printf("[Controller] angle=%.2f  → NORMAL\n", angle);
    else if (state == 1)
      printf("[Controller] angle=%.2f  → WARNING\n", angle);
    else
      printf("[Controller] angle=%.2f  → DANGER\n", angle);

    // === 5-4. heartbeat 보내기 ===
    if (state >= 0) {
      // 정상 또는 경고일 때만 heartbeat 전송
      kill(watchdog_pid, SIGUSR1);
    } else {
      printf("[Controller] !! DANGER: Heartbeat stopped. Watchdog will restart "
             "me.\n");
      // heartbeat 중단 → watchdog이 timeout 감지하고 바로 kill + restart
    }

    usleep(500000); // 0.5초 간격
  }

  return 0;
}
