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
#include <ipc_config.h>
#include <func_defs.h>
#include <stdbool.h>

#define TIMEOUT 3 // heartBeat가 오는 주기, 이 주기보다 늦으면 오류

time_t last_hb_time; // heartBeat주기를 알기 위한 heartBeat가 마지막으로 온 시간
pid_t controller_pid; // controller의 pid

void heartBeatHandler(int sig); 
// heartBeat가 왔는지 안왔는지 갱신해주고, last_hb_time를 갱신하는 함수 
void setupSignalHandler(void);
// main이 시작할때 SIGUSR1 수신시 heartBeatHandler와 연결
// heartBeatHandler와 역할을 분리
void performRecovery(void);
// controller를 강제 종료하고, 정리, 새 프로세스 시작, 프로그램 시작 수행 함수

int main() {
	printf("--- Watchdog process started ---");
	setupSignalHandler();
	unsigned int sleepTime;
	for(;;) {
		// cpu부담을 줄이기 위한 sleep, TIMEOUT보다 작아야 한다
		if((sleepTime = sleep(2)) < 2) perror("sleep interupt error");
		// 시그널이 온 시간이 timeout보다 작을때, 즉 오류가 생겼다고 판단될 시
		// performRecovery를 실행
		if(time(NULL) - last_hb_time > TIMEOUT) performRecovery();
	}
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
	if((sigaction(SIGUSR1, &act, NULL)) < 0) perror("sigaction error");
}

void performRecovery(void) {

}
