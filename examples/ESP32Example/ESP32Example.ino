/*
  ESP32Example - ESP32용 TimerStepper 예제
  
  이 예제는 ESP32에서 TimerStepper 라이브러리를 사용하는 방법을 보여줍니다.
  ESP32의 하드웨어 타이머를 사용하여 정확한 스테퍼 모터 제어를 구현합니다.
  
  회로 연결:
  - 스테퍼 모터 드라이버의 STEP 핀을 GPIO 2에 연결
  - 스테퍼 모터 드라이버의 DIR 핀을 GPIO 3에 연결
  - 모터 드라이버의 전원과 GND 연결
  - ESP32의 3.3V 또는 5V를 드라이버 VCC에 연결
  
  작성자: AI Assistant
  버전: 1.1.0
*/

#include "TimerStepper.h"

// ESP32 핀 정의 (GPIO 번호 사용)
// ESP32에서는 D2, D3 대신 GPIO 번호를 직접 사용
#define STEP_PIN 2  // GPIO 2
#define DIR_PIN 3   // GPIO 3

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 TimerStepper 예제 시작");
  
  // ESP32 정보 출력
  Serial.print("ESP32 칩 모델: ");
  Serial.println(ESP.getChipModel());
  Serial.print("ESP32 칩 리비전: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("ESP32 CPU 주파수: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  
  // 스테퍼 모터 설정
  stepper.setMaxSpeed(2000.0);      // 위치 제어용 최대 속도 (ESP32는 더 높은 속도 지원 가능)
  stepper.setAcceleration(1000.0);   // 가속도 설정
  stepper.setMinPulseWidth(1);       // 최소 펄스 폭 설정
  
  // 현재 위치를 0으로 설정
  stepper.setCurrentPosition(0);
  
  Serial.println("ESP32 타이머 기반 스테퍼 모터 제어 준비 완료");
  Serial.println("3초 후 시작...");
  delay(3000);
}

void loop() {
  // 예제 1: 기본 속도 제어
  Serial.println("=== 예제 1: 기본 속도 제어 ===");
  stepper.setDirection(true);
  stepper.setSpeed(1000.0);
  stepper.runSpeed();
  delay(3000);
  stepper.stop();
  delay(1000);
  
  stepper.setDirection(false);
  stepper.setSpeed(500.0);
  stepper.runSpeed();
  delay(3000);
  stepper.stop();
  delay(2000);
  
  // 예제 2: 가속/감속 제어
  Serial.println("=== 예제 2: 가속/감속 제어 ===");
  stepper.setAcceleration(800.0);
  stepper.setMaxSpeed(1500.0);
  
  // 절대 위치로 이동
  Serial.println("위치 2000으로 이동 (가속/감속 포함)");
  stepper.moveTo(2000);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    // 진행 상황 출력
    if (stepper.currentPosition() % 200 == 0) {
      Serial.print("현재 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 목표까지: ");
      Serial.print(stepper.distanceToGo());
      Serial.print(", 현재 속도: ");
      Serial.println(stepper.speed());
    }
  }
  Serial.println("목표 위치 도달!");
  delay(2000);
  
  // 예제 3: 원점 복귀
  Serial.println("=== 예제 3: 원점 복귀 ===");
  stepper.moveTo(0);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  Serial.println("원점 도달!");
  delay(2000);
  
  // 예제 4: 상대 위치 이동
  Serial.println("=== 예제 4: 상대 위치 이동 ===");
  stepper.move(1000);  // +1000 이동
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  Serial.println("+1000 이동 완료!");
  delay(1000);
  
  stepper.move(-500);  // -500 이동
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  Serial.println("-500 이동 완료!");
  delay(2000);
  
  // 예제 5: runToNewPosition 사용
  Serial.println("=== 예제 5: runToNewPosition 사용 ===");
  stepper.runToNewPosition(1500);
  Serial.println("1500 위치 도달!");
  delay(2000);
  
  stepper.runToNewPosition(0);
  Serial.println("원점 복귀 완료!");
  delay(2000);
  
  // 예제 6: 가속도 변경 테스트
  Serial.println("=== 예제 6: 가속도 변경 테스트 ===");
  
  // 낮은 가속도로 테스트
  Serial.println("낮은 가속도 (200)로 테스트");
  stepper.setAcceleration(200.0);
  stepper.runToNewPosition(1000);
  delay(1000);
  
  // 높은 가속도로 테스트
  Serial.println("높은 가속도 (2000)로 테스트");
  stepper.setAcceleration(2000.0);
  stepper.runToNewPosition(0);
  delay(2000);
  
  // 예제 7: 최대 속도 변경 테스트
  Serial.println("=== 예제 7: 최대 속도 변경 테스트 ===");
  
  // 낮은 최대 속도
  Serial.println("낮은 최대 속도 (500)로 테스트");
  stepper.setMaxSpeed(500.0);
  stepper.setAcceleration(1000.0);
  stepper.runToNewPosition(1000);
  delay(1000);
  
  // 높은 최대 속도
  Serial.println("높은 최대 속도 (3000)로 테스트");
  stepper.setMaxSpeed(3000.0);
  stepper.runToNewPosition(0);
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
  Serial.print("총 스텝 수: ");
  Serial.println(stepper.getStepCount());
  Serial.println("========================");
  
  // ESP32 시스템 정보
  Serial.println("=== ESP32 시스템 정보 ===");
  Serial.print("사용 가능한 힙 메모리: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("사용 가능한 PSRAM: ");
  Serial.print(ESP.getFreePsram());
  Serial.println(" bytes");
  Serial.print("업타임: ");
  Serial.print(millis() / 1000);
  Serial.println(" 초");
  Serial.println("========================");
  
  delay(5000);
  Serial.println("\n다시 시작...\n");
}
