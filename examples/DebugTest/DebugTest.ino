/*
  DebugTest - 위치 이동 디버그 테스트 예제
  
  이 예제는 위치 이동이 작동하지 않는 문제를 디버깅합니다.
  Serial Monitor에서 디버그 출력을 확인하세요.
  
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
  Serial.println("TimerStepper 위치 이동 디버그 테스트");
  
  // MS 핀 설정 (마이크로스텝)
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);
  
  // 스테퍼 모터 설정
  stepper.setMaxSpeed(5000.0);      // 위치 제어용 최대 속도: 5000 스텝/초
  stepper.setAcceleration(1000.0);   // 가속도: 1000 스텝/초²
  stepper.setCurrentPosition(0);    // 현재 위치를 0으로 설정
  
  Serial.println("설정 완료 - 3초 후 시작");
  delay(3000);
}

void loop() {
  Serial.println("=== 디버그 테스트 시작 ===");
  
  // 현재 상태 출력
  Serial.print("현재 위치: ");
  Serial.println(stepper.currentPosition());
  Serial.print("목표 위치: ");
  Serial.println(stepper.targetPosition());
  Serial.print("최대 속도: ");
  Serial.println(stepper.maxSpeed());
  Serial.print("가속도: ");
  Serial.println(stepper.acceleration());
  
  // moveTo 호출
  Serial.println("moveTo(10000) 호출...");
  stepper.moveTo(10000);
  
  // run() 루프 실행
  Serial.println("run() 루프 시작...");
  unsigned long startTime = millis();
  unsigned long lastPrintTime = 0;
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    
    // // 1초마다 상태 출력
    // if (millis() - lastPrintTime >= 1000) {
    //   Serial.print("시간: ");
    //   Serial.print((millis() - startTime) / 1000);
    //   Serial.print("초, 현재 위치: ");
    //   Serial.print(stepper.currentPosition());
    //   Serial.print(", 목표까지: ");
    //   Serial.print(stepper.distanceToGo());
    //   Serial.print(", 현재 속도: ");
    //   Serial.print(stepper.speed());
    //   Serial.print(", 가속 중: ");
    //   Serial.print(stepper.isAccelerating() ? "예" : "아니오");
    //   Serial.print(", 감속 중: ");
    //   Serial.println(stepper.isDecelerating() ? "예" : "아니오");
    //   lastPrintTime = millis();
    // }
    
    // 10초 후 타임아웃
    if (millis() - startTime > 10000) {
      Serial.println("타임아웃! 10초 후에도 목표에 도달하지 못했습니다.");
      break;
    }
  }
  
  Serial.println("목표 위치 도달!");
  Serial.print("최종 위치: ");
  Serial.println(stepper.currentPosition());
  
  delay(5000);
  Serial.println("\n다시 시작...\n");
}
