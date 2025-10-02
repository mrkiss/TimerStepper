/*
  PositionTest - 위치 이동 테스트 예제
  
  이 예제는 TimerStepper의 위치 이동 기능을 테스트합니다.
  AccelStepper 호환 기능의 moveTo, move, run 함수들을 테스트합니다.
  
  회로 연결:
  - 스테퍼 모터 드라이버의 STEP 핀을 디지털 핀 2에 연결
  - 스테퍼 모터 드라이버의 DIR 핀을 디지털 핀 3에 연결
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
  Serial.println("TimerStepper 위치 이동 테스트 시작");
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);
  // 스테퍼 모터 설정
  stepper.setMaxSpeed(1000.0);      // 위치 제어용 최대 속도: 1000 스텝/초
  stepper.setAcceleration(500.0);   // 가속도: 500 스텝/초²
  stepper.setCurrentPosition(0);    // 현재 위치를 0으로 설정
  
  Serial.println("설정 완료 - 3초 후 시작");
  delay(3000);
}

void loop() {
  // 테스트 1: 기본 위치 이동
  Serial.println("=== 테스트 1: 기본 위치 이동 ===");
  Serial.print("현재 위치: ");
  Serial.println(stepper.currentPosition());
  
  Serial.println("위치 1000으로 이동...");
  stepper.moveTo(1000);
  
  // 위치 이동 실행
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    
    // 진행 상황 출력 (100 스텝마다)
    if (stepper.currentPosition() % 100 == 0) {
      Serial.print("현재 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 목표까지: ");
      Serial.print(stepper.distanceToGo());
      Serial.print(", 현재 속도: ");
      Serial.print(stepper.speed());
      Serial.print(", 가속 중: ");
      Serial.print(stepper.isAccelerating() ? "예" : "아니오");
      Serial.print(", 감속 중: ");
      Serial.println(stepper.isDecelerating() ? "예" : "아니오");
    }
  }
  
  Serial.println("목표 위치 도달!");
  Serial.print("최종 위치: ");
  Serial.println(stepper.currentPosition());
  delay(2000);
  
  // 테스트 2: 원점 복귀
  Serial.println("=== 테스트 2: 원점 복귀 ===");
  Serial.println("원점(0)으로 복귀...");
  stepper.moveTo(0);
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  Serial.println("원점 도달!");
  Serial.print("최종 위치: ");
  Serial.println(stepper.currentPosition());
  delay(2000);
  
  // 테스트 3: 상대 위치 이동
  Serial.println("=== 테스트 3: 상대 위치 이동 ===");
  Serial.println("+500 이동...");
  stepper.move(500);
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  Serial.print("+500 이동 완료! 현재 위치: ");
  Serial.println(stepper.currentPosition());
  delay(1000);
  
  Serial.println("-300 이동...");
  stepper.move(-300);
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  Serial.print("-300 이동 완료! 현재 위치: ");
  Serial.println(stepper.currentPosition());
  delay(2000);
  
  // 테스트 4: runToNewPosition 사용
  Serial.println("=== 테스트 4: runToNewPosition 사용 ===");
  Serial.println("runToNewPosition(1500) 실행...");
  stepper.runToNewPosition(1500);
  Serial.print("1500 위치 도달! 현재 위치: ");
  Serial.println(stepper.currentPosition());
  delay(2000);
  
  Serial.println("runToNewPosition(0) 실행...");
  stepper.runToNewPosition(0);
  Serial.print("원점 복귀 완료! 현재 위치: ");
  Serial.println(stepper.currentPosition());
  delay(2000);
  
  // 상태 정보 출력
  Serial.println("=== 현재 상태 정보 ===");
  Serial.print("현재 위치: ");
  Serial.println(stepper.currentPosition());
  Serial.print("목표 위치: ");
  Serial.println(stepper.targetPosition());
  Serial.print("현재 속도: ");
  Serial.println(stepper.speed());
  Serial.print("최대 속도: ");
  Serial.println(stepper.maxSpeed());
  Serial.print("가속도: ");
  Serial.println(stepper.acceleration());
  Serial.print("남은 거리: ");
  Serial.println(stepper.distanceToGo());
  Serial.print("가속 중: ");
  Serial.println(stepper.isAccelerating() ? "예" : "아니오");
  Serial.print("감속 중: ");
  Serial.println(stepper.isDecelerating() ? "예" : "아니오");
  Serial.println("========================");
  
  delay(5000);
  Serial.println("\n다시 시작...\n");
}
