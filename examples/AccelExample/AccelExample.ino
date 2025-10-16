/*
  AccelExample - TimerStepper 가속/감속 및 위치 제어 예제
  
  이 예제는 TimerStepper 라이브러리의 AccelStepper 호환 기능들을 보여줍니다:
  - 가속/감속 제어
  - 절대 위치 이동
  - 상대 위치 이동
  - 목표 위치 도달 감지
  
  회로 연결:
  - 스테퍼 모터 드라이버의 STEP 핀을 디지털 핀 2에 연결
  - 스테퍼 모터 드라이버의 DIR 핀을 디지털 핀 3에 연결
  - 모터 드라이버의 전원과 GND 연결
  
  작성자: AI Assistant
  버전: 1.0.0
*/

#include "TimerStepper.h"

// 스테퍼 모터 핀 정의
#define STEP_PIN 22
#define DIR_PIN 23

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(9600);
  Serial.println("TimerStepper 가속/감속 예제 시작");
  
  // 가속도와 최대 속도 설정 (위치 제어용)
  stepper.setMaxSpeed(1000.0);      // 위치 제어용 최대 속도: 1000 스텝/초
  stepper.setAcceleration(500.0);   // 가속도: 500 스텝/초²
  
  // 현재 위치를 0으로 설정
  stepper.setCurrentPosition(0);
  
  Serial.println("설정 완료 - 3초 후 시작");
  delay(3000);
}

void loop() {
  // 예제 1: 절대 위치로 이동 (타이머 기반 - run() 루프 불필요)
  Serial.println("예제 1: 절대 위치 2000으로 이동");
  stepper.moveTo(2000);
  
  // 진행 상황 모니터링 (선택사항)
  unsigned long lastStatusTime = 0;
  while (stepper.distanceToGo() != 0) {
    // 인터럽트에서 모든 계산을 처리하므로 단순 대기
    if (millis() - lastStatusTime >= 500) {  // 0.5초마다 상태 출력
      Serial.print("현재 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 목표까지: ");
      Serial.println(stepper.distanceToGo());
      lastStatusTime = millis();
    }
    delayMicroseconds(1000);  // CPU 사용량 감소
  }
  Serial.println("목표 위치 도달!");
  delay(2000);
  
  // 예제 2: 원점으로 복귀 (타이머 기반)
  Serial.println("예제 2: 원점(0)으로 복귀");
  stepper.moveTo(0);
  while (stepper.distanceToGo() != 0) {
    // 인터럽트에서 모든 계산을 처리
    delayMicroseconds(1000);
  }
  Serial.println("원점 도달!");
  delay(2000);
  
  // 예제 3: 상대 위치 이동 (타이머 기반)
  Serial.println("예제 3: 상대 위치 +1000 이동");
  stepper.move(1000);
  while (stepper.distanceToGo() != 0) {
    delayMicroseconds(1000);
  }
  Serial.println("+1000 이동 완료!");
  delay(2000);
  
  // 예제 4: 반대 방향으로 이동 (타이머 기반)
  Serial.println("예제 4: 상대 위치 -500 이동");
  stepper.move(-500);
  while (stepper.distanceToGo() != 0) {
    delayMicroseconds(1000);
  }
  Serial.println("-500 이동 완료!");
  delay(2000);
  

  
  // 상태 정보 출력
  Serial.println("=== 현재 상태 ===");
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
  Serial.println("==================");
  
  delay(5000);
  Serial.println("\n다시 시작...\n");
}
