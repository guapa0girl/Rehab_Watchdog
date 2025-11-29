```
rehab_watchdog_min/
├── include/
│   ├── ipc_config.h     # IPC 키 및 공용 구조체 정의 (공통)
│   └── func_defs.h      # 프로세스 간 함수 프로토타입 정의 (공통)
│
├── src/
│   ├── main.c           # 시스템 시작 및 프로세스 생성/관리 (너 담당)
│   ├── watchdog.c       # 🛡️ 감시 및 복구 로직 (너 담당)
│   ├── controller.c     # ⚙️ 핵심 제어 로직 & Heartbeat (너 담당)
│   ├── sensor_sim.c     # 🌊 모의 센서 데이터 생성 (팀원 1 담당)
│   ├── test_cases.c     # ⚠️ 오류 주입 및 테스트 함수 (팀원 1 담당)
│   └── ui_logger.c      # 📊 간단한 터미널 출력 및 로깅 (팀원 2 담당)
│
└── Makefile             # 컴파일 자동화 (공통)
```
