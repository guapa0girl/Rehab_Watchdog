#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "ipc_config.h"

#define NUM_PROCESSES 3

// 실행할 파일들의 경로
char *process_paths[NUM_PROCESSES] = {
	{"./watchdog"},
	{"./controller"},
	{"./sensor_sim"}
};

// execv에 전달할 인자 목록 (argv[0]은 프로그램의 이름이어야 함)
char *watchdog_argv[] = {"watchdog", NULL};
char *controller_argv[] = {"controller", NULL};
char *sensor_sim_argv[] = {"sensor_sim", NULL};

char *argv_lists[] = {
	watchdog_argv,
	controller_argv,
	sensor_sim_argv
};

// process를 fork하고 execv 로 실행하는 함수
int run_process(char *path, char *argv[])
{
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork failed");
		return -1;
	} else if (pid == 0) {
		// 자식 프로세스: execv를 사용하여 새로운 프로그램으로 대체
		printf("[MAIN] Starting %s...\n", argv[0]);
		// exec는첫 번째 인수로 실행할 파일의 경로, 두 번째 인수로 인자 리스트를 받음
		if(execv(path, argv) == -1) {
			perror("execv failed");
			exit(EXIT_FAILURE);
		}
	}
	return pid; // 부모 프로세스에서는 자식의 pid를 반환
}

int main() {
	printf("--- Rehab Watchdog System Start ---\n");
	//1. IPC 자원 초기화
	pid_t pids[NUM_PROCESSES];
	
	//2. 프로세스 실행 Watchdog -> Controller -> Sensor 순서로 실행이 되야함
	for (int i = 0; i < NUM_PROCESSES; i++) {
		pids[i] = run_process(process_paths[i], argv_lists[i]);
		if(pids[i] < 0) {
 			perror("process error");
			return 1;
		}
	}

	printf("[Main] All core processes launched. PIDs: Watchdog:%d, Controller:%d, sensor_sim:%d\n", pids[0], pids[1], pids[2]);

	//3. 시스템 감시 및 종료 대기
	int status;
	pid_t terminated_pid;

	//자식 중 하나가 종료할 때까지 대기
	while ((terminated_pid = wait(&status)) > 0) {
		printf("[MAIN] Child process %d terminated.\n", terminated_pid);
		
		// Watchdog이 종료되면 전체 시스템 종료
		if (terminated_pid == pids[0] && (WIFEXITED(status) || WIFSIGNALED(status))){
			printf("[MAIN] Watchdog terminated. Full system will shutdown.\n");
			break;
		}
	}

	printf("--- Syystem Shutdown Complete ---\n");
	return 0;
}
