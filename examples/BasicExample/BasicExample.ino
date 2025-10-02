/*
  BasicExample - TimerStepper 기본 사용 예제
  
  타이머를 사용한 스테퍼 모터 구동의 기본적인 사용법을 보여줍니다.
  
  하드웨어 연결:
  - 스텝핀: 디지털 핀 2
  - 방향핀: 디지털 핀 3
  
  작성자: AI Assistant
  라이브러리: TimerStepper
*/

#include <TimerStepper.h>

// 핀 정의
const int STEP_PIN = 22;
const int DIR_PIN = 23;
#define MS1_PIN 13
#define MS2_PIN 12
#define MS3_PIN 27

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("TimerStepper 기본 예제 시작");
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  digitalWrite(MS1_PIN, LOW);
  digitalWrite(MS2_PIN, LOW);
  digitalWrite(MS3_PIN, LOW);
  
  // 스테퍼 모터 초기 설정
  stepper.setMinPulseWidth(2);  // 최소 펄스 폭: 2마이크로초
  stepper.setDirection(true);   // 초기 방향: 시계방향
  
  Serial.println("초기화 완료");
  delay(1000);
}

void loop() {
  // 시계방향으로 500 스텝/초 속도로 5초간 회전
  Serial.println("시계방향 회전 시작 (500 스텝/초)");
  stepper.setDirection(true);
  stepper.setSpeed(500.0);
  stepper.runSpeed();
  delay(5000);
  stepper.stop();
  Serial.println("정지");
  delay(1000);
  
  // 반시계방향으로 300 스텝/초 속도로 5초간 회전
  Serial.println("반시계방향 회전 시작 (300 스텝/초)");
  stepper.setDirection(false);
  stepper.setSpeed(300.0);
  stepper.runSpeed();
  delay(5000);
  stepper.stop();
  Serial.println("정지");
  delay(1000);
  
  // 속도 변화 테스트
  Serial.println("속도 변화 테스트 시작");
  stepper.setDirection(true);
  
  // 점진적 속도 증가
  for (int speed = 100; speed <= 800; speed += 100) {
    Serial.print("속도: ");
    Serial.print(speed);
    Serial.println(" 스텝/초");
    stepper.setSpeed(speed);
    stepper.runSpeed();
    delay(2000);
  }
  
  // 점진적 속도 감소
  for (int speed = 800; speed >= 100; speed -= 100) {
    Serial.print("속도: ");
    Serial.print(speed);
    Serial.println(" 스텝/초");
    stepper.setSpeed(speed);
    stepper.runSpeed();
    delay(2000);
  }
  
  stepper.stop();
  Serial.println("속도 변화 테스트 완료");
  delay(2000);
  
  // 스텝 카운트 출력
  Serial.print("총 스텝 수: ");
  Serial.println(stepper.getStepCount());
  stepper.resetStepCount();
  Serial.println("스텝 카운트 리셋");
  delay(2000);
  
  // 타이머 기반 위치 제어 데모
  Serial.println("=== 타이머 기반 위치 제어 데모 ===");
  stepper.setMaxSpeed(1000.0);     // 위치 제어용 최대 속도 설정
  stepper.setAcceleration(200.0);  // 가속도 설정
  stepper.setCurrentPosition(0);   // 현재 위치를 0으로 설정
  
  // 타이머 기반 위치 이동 (메인 루프를 블로킹하지 않음)
  Serial.println("위치 500으로 이동 (타이머 기반)");
  stepper.moveTo(500);
  
  // 메인 루프에서 다른 작업 수행 가능 (타이머 기반)
  unsigned long startTime = millis();
  unsigned long lastStatusTime = 0;
  while (stepper.distanceToGo() != 0) {
    // 여기서 다른 작업 수행 가능 (예: 센서 읽기, LED 제어 등)
    // 인터럽트에서 모든 계산을 처리하므로 CPU 사용량이 적음
    if (millis() - lastStatusTime >= 1000) {  // 1초마다 상태 출력
      Serial.print("이동 중 - 현재 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 남은 거리: ");
      Serial.print(stepper.distanceToGo());
      Serial.print(", 현재 속도: ");
      Serial.println(stepper.speed());
      lastStatusTime = millis();
    }
    
    // 10초 타임아웃
    if (millis() - startTime > 10000) {
      Serial.println("이동 타임아웃!");
      break;
    }
    
    delayMicroseconds(1000);  // CPU 사용량 감소
  }
  Serial.println("목표 도달!");
  delay(1000);
  
  Serial.println("원점으로 복귀 (타이머 기반)");
  stepper.moveTo(0);
  
  startTime = millis();
  lastStatusTime = 0;
  while (stepper.distanceToGo() != 0) {
    // 메인 루프에서 다른 작업 수행 가능 (타이머 기반)
    if (millis() - lastStatusTime >= 1000) {  // 1초마다 상태 출력
      Serial.print("복귀 중 - 현재 위치: ");
      Serial.print(stepper.currentPosition());
      Serial.print(", 남은 거리: ");
      Serial.print(stepper.distanceToGo());
      Serial.print(", 현재 속도: ");
      Serial.println(stepper.speed());
      lastStatusTime = millis();
    }
    
    // 10초 타임아웃
    if (millis() - startTime > 10000) {
      Serial.println("복귀 타임아웃!");
      break;
    }
    
    delayMicroseconds(1000);  // CPU 사용량 감소
  }
  Serial.println("원점 도달!");
  delay(2000);
}





