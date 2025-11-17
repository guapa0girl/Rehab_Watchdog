/* 
* IPC키를 사용할 예정
* Controller의 pid를 일정 주기마다 살아있는지 체크
* 만약 Controller가 죽었다고 판단 시
* 비정상 상태, 혹은 정지상태라고 판단
* 그러면 감시 대상(Controller)를 강제종료
* 다시 재실행하여 시스템 복구
*/

/*
* 그러면 Controller에서 여기로 생존신호를 주기적으로 보내야 함
* 받은 생존신호를 기준으로 살았는지 죽었는지 계속 체크
* 만약 죽었다면 Controller를 바로 강제 종료후 바로 재실행
*/

/*
* 생존신호: heartBeat
* heartBeat가 오는 주기: timeout
*/

#include <bits/types/sigset_t.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <ipc_config.h>
#include <func_defs.h>
#include <stdbool.h>

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

#define TIMEOUT 3 // heartBeat가 오는 주기, 이 주기보다 늦으면 오류

int g_shmid = -1;
int g_semid = -1;
sensor_shm_data_t *g_shm = NULL;


time_t last_hb_time; // heartBeat주기를 알기 위한 heartBeat가 마지막으로 온 시간
pid_t controller_pid; // controller의 pid

void heartBeatHandler(int sig); 
// heartBeat가 왔는지 안왔는지 갱신해주고, last_hb_time를 갱신하는 함수 
void setupSignalHandler(void);
// main이 시작할때 SIGUSR1 수신시 heartBeatHandler와 연결
// heartBeatHandler와 역할을 분리
void performRecovery(void);
// controller를 강제 종료하고, 정리, 새 프로세스 시작, 프로그램 시작 수행 함수
void setupIPC(void);
void waitForControllerPID(void);


int main() {
	printf("--- Watchdog process started ---\n");
	
    // 1. IPC 연결 및 PID 교환
    setupIPC(); 
    waitForControllerPID();

    // 2. 시그널 핸들러 설정
	setupSignalHandler();

    // 3. 감시 루프
	last_hb_time = time(NULL); // 초기 Heartbeat 시간 설정
	unsigned int sleepTime;
    
	for(;;) {
		// CPU 부담을 줄이기 위한 sleep
		if((sleepTime = sleep(2)) < 2) {
            // 시그널 인터럽트 발생 시 (정상 Heartbeat 수신 시)
            // perror 대신 간단히 무시하거나 로그를 남기는 것이 좋음
        }

		// 타임아웃 검사
		if(time(NULL) - last_hb_time > TIMEOUT) {
            performRecovery();
        }
	}
    // shmdt(g_shm); // 프로세스가 종료될 때 자동으로 분리되지만 명시적으로 호출할 수도 있음
	return 0;
}

void heartBeatHandler(int sig) {
	last_hb_time = time(NULL);
}

void setupSignalHandler(void) {
	//sigaction을 사용하기 위한 act구조체 선언
	struct sigaction act;
	//act구조체에 sigaction이 일어났을 때 실행할 함수 연결 및, 기타 변수 초기화
	act.sa_handler = heartBeatHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	//sigaction 호출
	if((sigaction(SIGUSR1, &act, NULL)) < 0){
		perror("sigaction error -- Watchdog");
		exit(EXIT_FAILURE);
	}
}

void setupIPC(void) {
	struct sembuf p_buf, v_buf;
	p_buf.sem_num = 0;
	p_buf.sem_op = -1;
	p_buf.sem_flg = 0;

	v_buf.sem_num = 0;
	v_buf.sem_op = 1;
	v_buf.sem_flg = 0;

	// 1. 공유 메모리 연결
	if ((g_shmid = shmget(SHM_KEY_SENSOR, sizeof(sensor_shm_data_t), 0666)) < 0){
		perror("[Watchdog] shmget error");
		exit(EXIT_FAILURE);
	}

	if ((g_shm = shmat(g_shmid, NULL, 0)) == (void *)-1){
		perror("[Watchdog] shmat error");
		exit(EXIT_FAILURE);
	}

	// 2. 세마포어 연결
	if ((g_semid = semget(SEM_KEY_SYNC, 1, 0666)) < 0) {
		perror("[Watchdog] semget error");
		exit(EXIT_FAILURE);
	}
	
	// 3. 자신의 PID 기록 (Controller가 Heartbeat 대상을 알도록)
    if (semop(g_semid, &p_buf, 1) < 0) { perror("[Watchdog] semop P failed"); exit(EXIT_FAILURE); }
    g_shm->watchdog_pid = getpid();
    if (semop(g_semid, &v_buf, 1) < 0) { perror("[Watchdog] semop V failed"); exit(EXIT_FAILURE); }
    
    printf("[Watchdog] PID %d recorded in SHM.\n", getpid());
}

void waitForControllerPID(void) {
    struct sembuf p_buf, v_buf;
    p_buf.sem_num = 0;
	p_buf.sem_op = -1;
	p_buf.sem_flg = 0; // Lock
   
	v_buf.sem_num = 0;
	v_buf.sem_op = 1;  
	v_buf.sem_flg = 0;  // Unlock
    
    printf("[Watchdog] Waiting for Controller to start and record its PID...\n");
    
    while (1) {
        if (semop(g_semid, &p_buf, 1) < 0) { perror("[Watchdog] semop P failed"); exit(EXIT_FAILURE); }
        
        controller_pid = g_shm->controller_pid;
        
        if (semop(g_semid, &v_buf, 1) < 0) { perror("[Watchdog] semop V failed"); exit(EXIT_FAILURE); }
        
        if (controller_pid > 0) {
            printf("[Watchdog] Controller PID %d received. Monitoring started.\n", controller_pid);
            break;
        }
        sleep(1); // 1초 대기 후 다시 시도
    }
}

void performRecovery(void) {
	pid_t newControllerPid;
	
    printf("\n[Watchdog] Recovery initiated! (Controller PID: %d unresponsive)\n", controller_pid);

	char *const argv[] = {
		(char *)"./controller",
		(char *)NULL // Controller가 Watchdog PID를 SHM에서 읽어가므로 인자 불필요
	};

	// 1. Controller 강제 종료
	if((kill(controller_pid, SIGKILL) < 0)) perror("[Watchdog] kill error");
    
	// 2. 종료된 자식 회수 (좀비 프로세스 정리)
	if((waitpid(controller_pid, NULL, 0)) < 0) perror("[Watchdog] waitpid error");
	
	// 3. 새 Controller 프로세스 재시작
	if((newControllerPid = fork()) < 0) {
		perror("[Watchdog] fork error");
	} else if (newControllerPid == 0) {
		// 자식 프로세스: 새로운 Controller 실행
		printf("[Watchdog] Relaunching Controller...\n");
		execv("./controller", argv);

		// execv 실패 시
		perror("[Watchdog] execv failed");
		exit(EXIT_FAILURE);
	} else {
		// 부모 프로세스: 새로운 Controller의 PID 갱신
		controller_pid = newControllerPid;
        printf("[Watchdog] New Controller PID: %d\n", controller_pid);
        
        // ⭐️ 중요: last_hb_time 갱신하여 즉시 재복구 방지
        last_hb_time = time(NULL); 
	}
}
