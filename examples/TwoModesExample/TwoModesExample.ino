/*
  TwoModesExample - 타이머 기반 vs AccelStepper 호환 모드 비교 예제
  
  이 예제는 TimerStepper 라이브러리의 두 가지 사용 방식을 보여줍니다:
  1. 타이머 기반 모드: 하드웨어 타이머를 사용한 실시간 속도 제어
  2. AccelStepper 호환 모드: 가속/감속 및 위치 제어
  
  회로 연결:
  - 스테퍼 모터 드라이버의 STEP 핀을 디지털 핀 22에 연결
  - 스테퍼 모터 드라이버의 DIR 핀을 디지털 핀 23에 연결
  - 모터 드라이버의 전원과 GND 연결
  
  작성자: AI Assistant
  버전: 1.1.0
*/

#include "TimerStepper.h"

// 스테퍼 모터 핀 정의
#define STEP_PIN 22
#define DIR_PIN 23
#define MS1_PIN 13
#define MS2_PIN 12
#define MS3_PIN 27

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(9600);
  Serial.println("TimerStepper 두 가지 모드 비교 예제");
  
  // MS 핀 설정 (마이크로스텝)
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);
  
  Serial.println("설정 완료 - 3초 후 시작");
  delay(3000);
}

void loop() {
  // ===== 모드 1: 타이머 기반 모드 =====
  Serial.println("=== 모드 1: 타이머 기반 모드 ===");
  Serial.println("하드웨어 타이머를 사용한 실시간 속도 제어");
  
  // 타이머 기반 설정
  stepper.setMinPulseWidth(2);  // 최소 펄스 폭 설정
  
  // 시계방향으로 1000 스텝/초 속도로 3초간 회전
  Serial.println("시계방향 회전 (1000 스텝/초, 3초)");
  stepper.setDirection(true);
  stepper.setSpeed(1000.0);
  stepper.runSpeed();
  delay(3000);
  stepper.stop();
  delay(1000);
  
  // 반시계방향으로 500 스텝/초 속도로 3초간 회전
  Serial.println("반시계방향 회전 (500 스텝/초, 3초)");
  stepper.setDirection(false);
  stepper.setSpeed(500.0);
  stepper.runSpeed();
  delay(3000);
  stepper.stop();
  delay(2000);
  
  // ===== 모드 2: AccelStepper 호환 모드 =====
  Serial.println("=== 모드 2: AccelStepper 호환 모드 ===");
  Serial.println("가속/감속 및 위치 제어");
  
  // AccelStepper 호환 설정
  stepper.setMaxSpeed(1500.0);      // 위치 제어용 최대 속도: 1500 스텝/초
  stepper.setAcceleration(800.0);   // 가속도: 800 스텝/초²
  stepper.setCurrentPosition(0);    // 현재 위치를 0으로 설정
  
  // 방법 1: 기본 moveTo 사용 (타이머 기반 - run() 루프 불필요)
  Serial.println("방법 1: 기본 moveTo 사용");
  Serial.println("위치 2000으로 이동...");
  stepper.moveTo(2000);
  while (stepper.distanceToGo() != 0) {
    // 인터럽트에서 모든 계산을 처리
    if (stepper.currentPosition() % 200 == 0) {
      Serial.print("위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 속도: ");
      Serial.println(stepper.speed());
    }
    delayMicroseconds(1000);
  }
  Serial.println("목표 도달!");
  delay(2000);
  
  // 방법 2: moveTo with parameters (타이머 기반)
  Serial.println("방법 2: moveTo with parameters");
  Serial.println("원점으로 복귀 (낮은 가속도로)...");
  stepper.moveTo(0, 800.0, 300.0);  // 최대속도 800, 가속도 300
  while (stepper.distanceToGo() != 0) {
    delayMicroseconds(1000);
  }
  Serial.println("원점 도달!");
  delay(2000);
  
  // 방법 3: move with parameters (타이머 기반)
  Serial.println("방법 3: move with parameters");
  Serial.println("+1000 이동 (높은 가속도로)...");
  stepper.move(1000, 2000.0, 1500.0);  // 최대속도 2000, 가속도 1500
  while (stepper.distanceToGo() != 0) {
    delayMicroseconds(1000);
  }
  Serial.println("+1000 이동 완료!");
  delay(1000);
  
  Serial.println("-500 이동 (중간 가속도로)...");
  stepper.move(-500, 1200.0, 600.0);  // 최대속도 1200, 가속도 600
  while (stepper.distanceToGo() != 0) {
    delayMicroseconds(1000);
  }
  Serial.println("-500 이동 완료!");
  delay(2000);
  
  // 방법 4: runToNewPosition 사용
  Serial.println("방법 4: runToNewPosition 사용");
  Serial.println("runToNewPosition(1500) 실행...");
  stepper.runToNewPosition(1500);
  Serial.println("1500 위치 도달!");
  delay(2000);
  
  // ===== 두 모드 비교 요약 =====
  Serial.println("=== 두 모드 비교 요약 ===");
  Serial.println("타이머 기반 모드:");
  Serial.println("- 하드웨어 타이머 사용으로 정확한 타이밍");
  Serial.println("- 실시간 속도 변경 가능");
  Serial.println("- 단순한 속도 제어");
  Serial.println("- start()/stop() 사용");
  
  Serial.println("AccelStepper 호환 모드:");
  Serial.println("- 가속/감속 곡선 지원");
  Serial.println("- 위치 기반 제어");
  Serial.println("- moveTo()/move() 사용");
  Serial.println("- 타이머 인터럽트에서 모든 계산 처리 (run() 루프 불필요)");
  
  // 상태 정보 출력
  Serial.println("=== 현재 상태 정보 ===");
  Serial.print("현재 위치: ");
  Serial.println(stepper.currentPosition());
  Serial.print("목표 위치: ");
  Serial.println(stepper.targetPosition());
  Serial.print("현재 속도: ");
  Serial.println(stepper.speed());
  Serial.print("최대 속도 (위치제어용): ");
  Serial.println(stepper.maxSpeed());
  Serial.print("가속도: ");
  Serial.println(stepper.acceleration());
  Serial.print("남은 거리: ");
  Serial.println(stepper.distanceToGo());
  Serial.println("========================");
  
  delay(5000);
  Serial.println("\n다시 시작...\n");
}
