/*
  SpeedControl - TimerStepper 속도 제어 예제
  
  가변저항과 버튼을 사용하여 스테퍼 모터의 속도와 방향을 실시간으로 제어합니다.
  
  하드웨어 연결:
  - 스텝핀: 디지털 핀 2
  - 방향핀: 디지털 핀 3
  - 가변저항: 아날로그 핀 A0 (속도 조절용)
  - 방향 전환 버튼: 디지털 핀 4
  - 시작/정지 버튼: 디지털 핀 5
  
  작성자: AI Assistant
  라이브러리: TimerStepper
*/

#include <TimerStepper.h>

// 핀 정의
const int STEP_PIN = 22;
const int DIR_PIN = 23;
const int SPEED_POT_PIN = A0;
const int DIR_BUTTON_PIN = 4;
const int START_STOP_BUTTON_PIN = 5;

// TimerStepper 객체 생성
TimerStepper stepper(STEP_PIN, DIR_PIN);

// 변수들
bool lastDirButtonState = HIGH;
bool lastStartStopButtonState = HIGH;
bool motorRunning = false;
float currentSpeed = 0;
unsigned long lastStatusTime = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("TimerStepper 속도 제어 예제 시작");
  
  // 버튼 핀 설정
  pinMode(DIR_BUTTON_PIN, INPUT_PULLUP);
  pinMode(START_STOP_BUTTON_PIN, INPUT_PULLUP);
  
  // 스테퍼 모터 초기 설정
  stepper.setMinPulseWidth(2);  // 최소 펄스 폭: 2마이크로초
  stepper.setDirection(true);   // 초기 방향: 시계방향
  stepper.setSpeed(0);          // 초기 속도: 정지
  
  Serial.println("초기화 완료");
  Serial.println("가변저항으로 속도 조절, 버튼으로 방향/시작정지 제어");
  Serial.println("명령어:");
  Serial.println("  's' - 현재 상태 출력");
  Serial.println("  'r' - 스텝 카운트 리셋");
  Serial.println("  'h' - 도움말 출력");
}

void loop() {
  // 가변저항으로 속도 조절 (0 ~ 1000 스텝/초)
  int potValue = analogRead(SPEED_POT_PIN);
  float newSpeed = map(potValue, 0, 1023, 0, 1000);
  
  // 속도가 변경되었을 때만 업데이트
  if (abs(newSpeed - currentSpeed) > 5) {
    currentSpeed = newSpeed;
    stepper.setSpeed(currentSpeed);
    
    Serial.print("속도 설정: ");
    Serial.print(currentSpeed);
    Serial.println(" 스텝/초");
  }
  
  // 방향 전환 버튼 처리
  bool dirButtonState = digitalRead(DIR_BUTTON_PIN);
  if (dirButtonState == LOW && lastDirButtonState == HIGH) {
    // 버튼이 눌렸을 때
    bool newDirection = !stepper.getDirection();
    stepper.setDirection(newDirection);
    
    Serial.print("방향 변경: ");
    Serial.println(newDirection ? "시계방향" : "반시계방향");
    
    delay(50);  // 디바운싱
  }
  lastDirButtonState = dirButtonState;
  
  // 시작/정지 버튼 처리
  bool startStopButtonState = digitalRead(START_STOP_BUTTON_PIN);
  if (startStopButtonState == LOW && lastStartStopButtonState == HIGH) {
    // 버튼이 눌렸을 때
    if (motorRunning) {
      stepper.stop();
      motorRunning = false;
      Serial.println("모터 정지");
    } else {
      if (currentSpeed > 0) {
        stepper.runSpeed();
        motorRunning = true;
        Serial.println("모터 시작");
      } else {
        Serial.println("속도를 0보다 크게 설정하세요");
      }
    }
    
    delay(50);  // 디바운싱
  }
  lastStartStopButtonState = startStopButtonState;
  
  // 시리얼 명령어 처리
  if (Serial.available()) {
    char command = Serial.read();
    handleSerialCommand(command);
  }
  
  // 모터 상태 모니터링 (1초마다)
  if (millis() - lastStatusTime > 1000) {
    if (motorRunning && stepper.isRunning()) {
      Serial.print("실행 중 - 속도: ");
      Serial.print(stepper.getCurrentSpeed());
      Serial.print(" 스텝/초, 방향: ");
      Serial.print(stepper.getDirection() ? "시계방향" : "반시계방향");
      Serial.print(", 스텝 카운트: ");
      Serial.println(stepper.getStepCount());
    }
    lastStatusTime = millis();
  }
  
  // 속도가 0이 되면 자동으로 정지
  if (currentSpeed == 0 && motorRunning) {
    stepper.stop();
    motorRunning = false;
    Serial.println("속도 0으로 인한 자동 정지");
  }
  
  delay(10);  // 메인 루프 지연
}

void handleSerialCommand(char command) {
  switch (command) {
    case 's':
    case 'S':
      printStatus();
      break;
      
    case 'r':
    case 'R':
      stepper.resetStepCount();
      Serial.println("스텝 카운트 리셋 완료");
      break;
      
    case 'h':
    case 'H':
      printHelp();
      break;
      
    case '\n':
    case '\r':
      // 개행 문자 무시
      break;
      
    default:
      Serial.print("알 수 없는 명령어: ");
      Serial.println(command);
      Serial.println("'h'를 입력하여 도움말을 확인하세요.");
      break;
  }
}

void printStatus() {
  Serial.println("=== 모터 상태 ===");
  Serial.print("동작 상태: ");
  Serial.println(stepper.isRunning() ? "실행 중" : "정지");
  Serial.print("현재 속도: ");
  Serial.print(stepper.getCurrentSpeed());
  Serial.println(" 스텝/초");
  Serial.print("방향: ");
  Serial.println(stepper.getDirection() ? "시계방향" : "반시계방향");
  Serial.print("스텝 카운트: ");
  Serial.println(stepper.getStepCount());
  Serial.print("최대 속도 (위치제어용): ");
  Serial.print(stepper.getMaxSpeed());
  Serial.println(" 스텝/초");
  Serial.println("================");
}

void printHelp() {
  Serial.println("=== 도움말 ===");
  Serial.println("하드웨어 제어:");
  Serial.println("  - 가변저항: 속도 조절 (0-1000 스텝/초)");
  Serial.println("  - 방향 버튼: 회전 방향 전환");
  Serial.println("  - 시작/정지 버튼: 모터 시작/정지");
  Serial.println("");
  Serial.println("시리얼 명령어:");
  Serial.println("  's' - 현재 상태 출력");
  Serial.println("  'r' - 스텝 카운트 리셋");
  Serial.println("  'h' - 이 도움말 출력");
  Serial.println("==============");
}






