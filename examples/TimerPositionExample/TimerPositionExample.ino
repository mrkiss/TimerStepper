/*
  TimerPositionExample - 타이머 기반 위치 제어 데모
  
  이 예제는 TimerStepper 라이브러리의 타이머 기반 위치 제어 기능을 보여줍니다.
  메인 루프를 블로킹하지 않고 정확한 위치 제어가 가능합니다.
  
  하드웨어 연결:
  - STEP 핀: 2번 핀
  - DIR 핀: 3번 핀
  - MS1, MS2, MS3 핀: 4, 5, 6번 핀 (마이크로스테핑용)
  
  작성자: TimerStepper Library
  버전: 1.1.0
*/

#include "TimerStepper.h"

// 핀 정의
#define STEP_PIN 2
#define DIR_PIN 3
#define MS1_PIN 4
#define MS2_PIN 5
#define MS3_PIN 6

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

// LED 핀 (타이머 기반 제어 중 다른 작업 수행 예시)
#define LED_PIN 13

void setup() {
  Serial.begin(115200);
  Serial.println("TimerStepper 타이머 기반 위치 제어 예제");
  Serial.println("=====================================");
  
  // 마이크로스테핑 설정 (1/16 스텝)
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);
  
  // LED 핀 설정
  pinMode(LED_PIN, OUTPUT);
  
  // 가속도와 최대속도 설정
  stepper.setAcceleration(300.0);  // 300 스텝/초²
  stepper.setMaxSpeed(1000.0);     // 위치 제어용 최대 속도: 1000 스텝/초
  stepper.setCurrentPosition(0);   // 현재 위치를 0으로 설정
  
  Serial.println("설정 완료!");
  delay(2000);
}

void loop() {
  // 1. 정방향 이동
  Serial.println("\n=== 정방향 이동 (1000 스텝) ===");
  stepper.moveTo(1000);
  
  // 타이머 기반으로 동작하므로 메인 루프에서 다른 작업 수행 가능
  performOtherTasks("정방향 이동");
  
  // 2. 원점 복귀
  Serial.println("\n=== 원점 복귀 ===");
  stepper.moveTo(0);
  
  performOtherTasks("원점 복귀");
  
  // 3. 역방향 이동
  Serial.println("\n=== 역방향 이동 (-500 스텝) ===");
  stepper.moveTo(-500);
  
  performOtherTasks("역방향 이동");
  
  // 4. 다시 원점으로
  Serial.println("\n=== 원점 복귀 ===");
  stepper.moveTo(0);
  
  performOtherTasks("원점 복귀");
  
  // 5. 복합 이동 (여러 위치를 순차적으로)
  Serial.println("\n=== 복합 이동 시퀀스 ===");
  long positions[] = {500, -200, 800, -300, 0};
  int numPositions = sizeof(positions) / sizeof(positions[0]);
  
  for (int i = 0; i < numPositions; i++) {
    Serial.print("위치 ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(numPositions);
    Serial.print(": ");
    Serial.println(positions[i]);
    
    stepper.moveTo(positions[i]);
    performOtherTasks("복합 이동");
  }
  
  Serial.println("\n=== 시퀀스 완료 ===");
  delay(3000);
}

// 타이머 기반 제어 중 다른 작업 수행 함수
void performOtherTasks(String taskName) {
  unsigned long startTime = millis();
  unsigned long lastStatusTime = 0;
  bool ledState = false;
  
  while (stepper.distanceToGo() != 0) {
    unsigned long currentTime = millis();
    
    // LED 깜빡이기 (다른 작업 예시)
    if (currentTime - lastStatusTime > 200) {  // 200ms마다
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      lastStatusTime = currentTime;
    }
    
    // 1초마다 상태 출력
    if (currentTime % 1000 < 50) {
      Serial.print(taskName);
      Serial.print(" - 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 목표까지: ");
      Serial.print(stepper.distanceToGo());
      Serial.print(", 속도: ");
      Serial.print(stepper.speed(), 1);
      Serial.print(" 스텝/초");
      
      if (stepper.isAccelerating()) {
        Serial.print(" [가속중]");
      } else if (stepper.isDecelerating()) {
        Serial.print(" [감속중]");
      } else {
        Serial.print(" [정속]");
      }
      Serial.println();
    }
    
    // 15초 타임아웃
    if (currentTime - startTime > 15000) {
      Serial.println("작업 타임아웃!");
      stepper.stop();
      break;
    }
  }
  
  // LED 끄기
  digitalWrite(LED_PIN, LOW);
  
  Serial.print(taskName);
  Serial.println(" 완료!");
  delay(1000);
}
