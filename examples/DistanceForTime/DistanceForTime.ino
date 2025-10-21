/*
  DistanceCalculationExample - 시간에 따른 이동 가능 거리 계산 예제
  
  이 예제는 TimerStepper 라이브러리의 calculateDistanceForTime 함수를 사용하여
  주어진 시간 동안 얼마나 이동할 수 있는지 계산하는 방법을 보여줍니다.
  
  회로 연결:
  - 스테퍼 모터 드라이버의 STEP 핀을 Arduino의 2번 핀에 연결
  - 스테퍼 모터 드라이버의 DIR 핀을 Arduino의 3번 핀에 연결
  - 적절한 전원 공급 (모터 드라이버와 모터용)
  
  작성자: AI Assistant
  라이선스: MIT
*/

#include "TimerStepper.h"

// 스테퍼 모터 핀 설정
const int STEP_PIN = 2;
const int DIR_PIN = 3;

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("TimerStepper 거리 계산 예제");
  Serial.println("==========================");
  
  // 모터 설정
  stepper.setMaxSpeed(1000.0);        // 최대 속도: 1000 스텝/초
  stepper.setAcceleration(500.0);     // 가속도: 500 스텝/초²
  stepper.setCurrentPosition(0);      // 현재 위치를 0으로 설정
  
  Serial.println("모터 설정:");
  Serial.printf("최대 속도: %.1f 스텝/초\n", stepper.maxSpeed());
  Serial.printf("가속도: %.1f 스텝/초²\n", stepper.acceleration());
  Serial.println();
  
  // 다양한 시간에 대한 이동 가능 거리 계산
  testDistanceCalculations();
}

void loop() {
  // 실제 이동 테스트
  testActualMovement();
  delay(5000);
}

void testDistanceCalculations() {
  Serial.println("=== 시간별 이동 가능 거리 계산 ===");
  
  // 현재 설정된 속도와 가속도로 계산
  float testTimes[] = {0.1, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0};
  int numTests = sizeof(testTimes) / sizeof(testTimes[0]);
  
  for (int i = 0; i < numTests; i++) {
    float timeSeconds = testTimes[i];
    long distance = stepper.calculateDistanceForTime(timeSeconds);
    
    Serial.printf("%.1f초 동안 이동 가능한 거리: %ld 스텝\n", timeSeconds, distance);
    
    if (distance > 0) {
      float avgSpeed = (float)distance / timeSeconds;
      Serial.printf("  평균 속도: %.1f 스텝/초\n", avgSpeed);
    }
    Serial.println();
  }
  
  Serial.println("=== 다른 속도/가속도 조합 테스트 ===");
  
  // 다른 속도와 가속도 조합으로 계산
  float customMaxSpeed = 2000.0;    // 2000 스텝/초
  float customAcceleration = 1000.0; // 1000 스텝/초²
  float testTime = 2.0;             // 2초
  
  long customDistance = stepper.calculateDistanceForTime(testTime, customMaxSpeed, customAcceleration);
  
  Serial.printf("사용자 정의 설정 (최대속도: %.0f, 가속도: %.0f)\n", customMaxSpeed, customAcceleration);
  Serial.printf("%.1f초 동안 이동 가능한 거리: %ld 스텝\n", testTime, customDistance);
  
  if (customDistance > 0) {
    float avgSpeed = (float)customDistance / testTime;
    Serial.printf("평균 속도: %.1f 스텝/초\n", avgSpeed);
  }
  Serial.println();
}

void testActualMovement() {
  Serial.println("=== 실제 이동 테스트 ===");
  
  // 1초 동안 이동 가능한 거리 계산
  float testTime = 1.0;
  long calculatedDistance = stepper.calculateDistanceForTime(testTime);
  
  Serial.printf("계산된 거리: %ld 스텝 (%.1f초)\n", calculatedDistance, testTime);
  
  if (calculatedDistance > 0) {
    // 실제로 이동해보기
    Serial.println("실제 이동 시작...");
    unsigned long startTime = millis();
    
    stepper.moveTo(calculatedDistance);
    
    // 이동 완료까지 대기
    while (stepper.isRunning()) {
      delay(10);
    }
    
    unsigned long actualTime = millis() - startTime;
    float actualTimeSeconds = actualTime / 1000.0;
    
    Serial.printf("실제 이동 시간: %.3f초\n", actualTimeSeconds);
    Serial.printf("계산 시간과의 차이: %.3f초\n", abs(testTime - actualTimeSeconds));
    Serial.printf("실제 평균 속도: %.1f 스텝/초\n", (float)calculatedDistance / actualTimeSeconds);
    Serial.println();
    
    // 원점으로 복귀
    Serial.println("원점으로 복귀...");
    stepper.moveTo(0);
    while (stepper.isRunning()) {
      delay(10);
    }
    Serial.println("복귀 완료");
  } else {
    Serial.println("이동할 거리가 없습니다.");
  }
  
  Serial.println("==========================");
}
