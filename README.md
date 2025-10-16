# TimerStepper

타이머 기반 스테퍼 모터 제어 라이브러리

## 개요

TimerStepper는 하드웨어 타이머를 사용하여 정확한 펄스 타이밍으로 스테퍼 모터를 제어하는 Arduino 라이브러리입니다. 스텝핀과 방향핀 두 개로 모터를 구동하며, 실시간 속도 조절이 가능합니다.

## 주요 특징

- **하드웨어 타이머 사용**: Timer1/ESP32 타이머를 사용하여 정확한 펄스 타이밍 구현
- **비블로킹 동작**: 메인 루프를 블록하지 않는 인터럽트 기반 제어
- **실시간 속도 조절**: 동작 중에도 속도 변경 가능
- **2핀 제어**: 스텝핀과 방향핀만으로 모터 제어
- **스텝 카운팅**: 정확한 스텝 수 추적
- **안정성**: 최소 펄스 폭 보장 및 속도 제한
- **AccelStepper 호환**: 가속/감속 및 위치 제어 기능 지원
- **선형 가속도**: 부드러운 가속/감속 곡선
- **위치 제어**: 절대/상대 위치 이동 및 목표 도달 감지
- **타이머 기반 위치 제어**: 위치 제어도 타이머를 사용하여 정확하고 효율적
- **논블로킹 인터럽트**: 인터럽트 내부에서 블로킹 없이 안정적인 동작

## 하드웨어 요구사항

- **AVR 기반**: Arduino Uno, Nano, Mega 등
- **ESP32 기반**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3 등
- 스테퍼 모터 드라이버 (A4988, DRV8825 등)
- 스테퍼 모터

## 설치

1. 이 라이브러리를 Arduino의 `libraries` 폴더에 복사합니다.
2. Arduino IDE를 재시작합니다.
3. `파일 > 예제 > TimerStepper`에서 예제를 확인할 수 있습니다.

## 하드웨어 연결

### AVR 기반 (Arduino Uno, Nano, Mega 등)
```
Arduino    스테퍼 드라이버
------    --------------
Pin 2  -> STEP
Pin 3  -> DIR
GND   -> GND
5V    -> VCC (드라이버 전원)
```

### ESP32 기반
```
ESP32      스테퍼 드라이버
------     --------------
GPIO 2  -> STEP
GPIO 3  -> DIR
GND    -> GND
3.3V   -> VCC (드라이버 전원)
```

## 기본 사용법

TimerStepper는 두 가지 사용 모드를 제공합니다:

### 1. 정속모드

하드웨어 타이머를 사용하여 정확한 타이밍으로 모터를 제어합니다.

```cpp
#include <TimerStepper.h>

const int STEP_PIN = 2;
const int DIR_PIN = 3;
TimerStepper stepper(STEP_PIN, DIR_PIN);

void setup() {
  stepper.setMaxSpeed(1000.0);  // 최대 속도 설정
  stepper.setMinPulseWidth(2);  // 최소 펄스 폭 설정
}

void loop() {
  // 시계방향으로 500 스텝/초 속도로 회전
  stepper.setDirection(true);
  stepper.setSpeed(500.0);
  stepper.runSpeed();  // 정속 회전 시작
  delay(5000);
  stepper.stop();      // 타이머 정지
  delay(1000);
}
```

### 2. 가속도 제어 가능힌 위치 제어모드

가속/감속 가능한 위치 기반 제어를 제공합니다.

```cpp
#include <TimerStepper.h>

TimerStepper stepper(2, 3);

void setup() {
  // 가속도와 최대 속도 설정 (필수!)
  stepper.setMaxSpeed(1000.0);      // 최대 속도: 1000 스텝/초
  stepper.setAcceleration(500.0);   // 가속도: 500 스텝/초²
  stepper.setCurrentPosition(0);    // 현재 위치를 0으로 설정
}

void loop() {
  // 방법 1: 기본 moveTo (미리 설정된 가속도/최대속도 사용)
  stepper.moveTo(2000);  // 2000 스텝까지 이동
  // 타이머 인터럽트 기반으로 자동 실행되므로 run() 함수 불필요
  // while (stepper.distanceToGo() != 0) {  // 제어권을 유지하고 싶을 때 사용
  //   // 다른 작업 수행 가능
  // }
  
  // 방법 2: moveTo with parameters (가속도/최대속도 직접 지정)
  stepper.moveTo(0, 800.0, 300.0);  // 최대속도 800, 가속도 300
  // while (stepper.distanceToGo() != 0) { 
  // }
  
  // 방법 3: 상대 위치 이동
  stepper.move(1000);  // 현재 위치에서 +1000 이동
  
  // 방법 3: 목표 위치까지 대기 (블로킹)
  stepper.moveTo(0);
  stepper.runToPosition();  // 목표 위치 도달까지 대기
  
  delay(1000);
}
```

### 두 모드의 차이점

| 특징 | 타이머 기반 모드 | AccelStepper 호환 모드 |
|------|------------------|------------------------|
| **제어 방식** | 실시간 속도 제어 | 위치 기반 제어 |
| **가속/감속** | 없음 (즉시 속도 변경) | 부드러운 가속/감속 곡선 |
| **사용 함수** | `runSpeed()`, `stop()` | `moveTo()`, `move()` |
| **타이밍** | 하드웨어 타이머 (정확) | 하드웨어 타이머 (정확) |
| **실행 방식** | `runSpeed()` 호출 필요 | `moveTo()` 호출 시 자동 실행 |
| **적용 분야** | 실시간 제어, 속도 변화 | 정밀 위치 제어, CNC |

## API 참조

### 생성자

```cpp
TimerStepper(uint8_t stepPin, uint8_t dirPin)
```
- `stepPin`: 스텝 펄스 출력 핀
- `dirPin`: 방향 제어 핀

### 모터 제어 함수

```cpp
void setSpeed(float stepsPerSecond)     // 속도 설정 (스텝/초)
void setDirection(bool clockwise)       // 방향 설정 (true: 시계방향)
void runSpeed()                         // 정속 회전 시작
void stop()                             // 모터 정지
void pause()                            // 모터 일시정지
void resume()                           // 모터 재시작
```

### 상태 확인 함수

```cpp
bool isRunning()                        // 모터 동작 상태 확인
float getCurrentSpeed()                 // 현재 속도 반환
bool getDirection()                     // 현재 방향 반환
unsigned long getStepCount()            // 스텝 카운트 반환
void resetStepCount()                   // 스텝 카운트 리셋
```

### 설정 함수

```cpp
void setMinPulseWidth(unsigned int microseconds)  // 최소 펄스 폭 설정
void setMaxSpeed(float maxStepsPerSecond)         // 최대 속도 제한
```

### 거리 계산 함수

```cpp
long calculateDistanceForTime(float timeSeconds)  // 주어진 시간(초)으로 이동 가능한 거리 계산
long calculateDistanceForTime(float timeSeconds, float maxSpeed, float acceleration)  // 매개변수로 속도/가속도 지정
```

이 함수들은 주어진 시간 동안 가속/감속 프로파일을 고려하여 이동 가능한 거리를 계산합니다:

- **삼각형 프로파일**: 짧은 시간의 경우 가속 → 감속만으로 구성
- **사다리꼴 프로파일**: 긴 시간의 경우 가속 → 정속 → 감속으로 구성
- **정확한 계산**: 물리학적 공식을 사용하여 정밀한 거리 계산
- **실시간 활용**: 정확한 타이밍이 필요한 애플리케이션에서 유용

**사용 예제:**
```cpp
// 현재 설정으로 1초 동안 이동 가능한 거리 계산
long distance = stepper.calculateDistanceForTime(1.0);

// 특정 속도/가속도로 2초 동안 이동 가능한 거리 계산  
long distance = stepper.calculateDistanceForTime(2.0, 1500.0, 800.0);
```

### 가속/감속 제어 함수 (AccelStepper 호환)

```cpp
void setAcceleration(float acceleration)          // 가속도 설정 (스텝/초²)
void setCurrentPosition(long position)            // 현재 위치 설정
void moveTo(long absolute)                        // 절대 위치로 이동
void move(long relative)                          // 상대 위치로 이동
```

### 위치 및 상태 확인 함수

```cpp
long currentPosition()                            // 현재 위치 반환
long targetPosition()                             // 목표 위치 반환
float speed()                                     // 현재 속도 반환
float maxSpeed()                                  // 최대 속도 반환
float acceleration()                              // 가속도 반환
long distanceToGo()                               // 남은 거리 반환
bool isAccelerating()                             // 가속 중인지 확인
bool isDecelerating()                             // 감속 중인지 확인
```

### 디버그 함수

```cpp
void debugISRStatus()                             // ISR 상태 정보 출력
void debugSimple()                                // 간단한 디버그 정보 출력
void debugDetailed()                              // 상세 디버그 정보 출력
void resetISRDebugCount()                         // ISR 디버그 카운터 리셋
void debugTimerStatus()                           // 타이머 상태 확인
```

## 예제

### BasicExample
기본적인 사용법을 보여주는 예제입니다. 기본 속도 제어와 AccelStepper 호환 기능을 모두 포함합니다.

### SpeedControl
가변저항과 버튼을 사용하여 실시간으로 속도와 방향을 제어하는 예제입니다.

### AccelExample
AccelStepper 호환 기능들을 자세히 보여주는 예제입니다:
- 가속/감속 제어
- 절대 위치 이동
- 상대 위치 이동
- 목표 위치 도달 감지
- 가속도 및 최대 속도 변경

### ESP32Example
ESP32 전용 예제입니다:
- ESP32 하드웨어 타이머 사용
- 높은 성능 (최대 3000 스텝/초)
- ESP32 시스템 정보 출력
- 플랫폼별 최적화된 설정

### TwoModesExample
두 가지 사용 모드를 비교하는 예제입니다:
- 타이머 기반 모드 vs AccelStepper 호환 모드
- 각 모드의 특징과 사용법 비교
- 실제 동작 차이 확인

### PositionTest
위치 이동 기능을 테스트하는 예제입니다:
- moveTo, move 함수 테스트
- 진행 상황 실시간 출력
- MS 핀 설정 포함

### DebugTest
디버그 기능을 테스트하는 예제입니다:
- ISR 상태 모니터링
- 타이머 상태 확인
- 디버그 정보 출력 기능 테스트

### TimerPositionExample
타이머 기반 위치 제어의 장점을 보여주는 예제입니다:
- 메인 루프를 블로킹하지 않는 위치 제어
- 동시에 다른 작업 수행 가능 (LED 제어, 센서 읽기 등)
- 복합 이동 시퀀스
- 실시간 상태 모니터링

### DistanceCalculationExample
시간에 따른 이동 가능 거리 계산 기능을 보여주는 예제입니다:
- 주어진 시간 동안 이동 가능한 거리 계산
- 다양한 시간과 속도/가속도 조합 테스트
- 계산된 거리로 실제 이동 테스트 및 검증
- 삼각형/사다리꼴 프로파일 분석

## 기술적 세부사항

### 타이머 설정

#### AVR 기반 (Arduino Uno, Nano, Mega 등)
- **Timer1** 사용 (16비트 타이머)
- **CTC 모드** (Clear Timer on Compare Match)
- **프리스케일러**: 1/8 (16MHz → 2MHz)
- **해상도**: 0.5마이크로초
- **동적 OCR1A 설정**: 각 펄스마다 간격 조정

#### ESP32 기반
- **하드웨어 타이머** 사용 (80MHz → 1MHz)
- **자동 리로드 모드**
- **해상도**: 1마이크로초
- **32비트 타이머** (더 넓은 범위 지원)
- **동적 타이머 알람**: timerAlarm()으로 펄스별 간격 실시간 조정

### 펄스 생성
- 최소 펄스 폭 보장
- 인터럽트 기반 정확한 타이밍
- 속도 제한 기능
- **동적 타이머 제어**: 각 펄스 간격을 개별적으로 설정하여 부드러운 가속/감속 구현
- **ISR 논블로킹**: 인터럽트 내부에서 delayMicroseconds()로 펄스 폭 제어 후 즉시 복귀
- **펄스 간격 배열**: 가속 구간의 펄스 간격을 미리 계산하여 배열에 저장, ISR에서 빠르게 조회
- **자동 실행**: moveTo() 호출 시 타이머 인터럽트가 자동으로 모든 제어를 처리

### 성능

#### AVR 기반
- **최대 속도**: 약 4000 스텝/초 (16MHz 클럭 기준)
- **최소 속도**: 1 스텝/초 이하도 지원
- **정확도**: 0.5마이크로초 단위 정밀 제어

#### ESP32 기반
- **최대 속도**: 약 8000 스텝/초 (80MHz 클럭 기준)
- **최소 속도**: 1 스텝/초 이하도 지원
- **정확도**: 1마이크로초 단위 정밀 제어
- **더 높은 성능**: 32비트 타이머로 더 넓은 속도 범위

## 주의사항

1. **타이머 충돌**: 
   - AVR: Timer1을 사용하므로 다른 라이브러리와 충돌 가능
   - ESP32: 하드웨어 타이머를 사용하므로 충돌 가능성 낮음
2. **전원 공급**: 스테퍼 모터에 충분한 전원 공급 필요
3. **펄스 폭**: 드라이버에 맞는 최소 펄스 폭 설정 필요
4. **속도 제한**: 모터와 드라이버의 최대 속도 고려
5. **ESP32 전용**: 
   - GPIO 핀 번호 사용 (D2, D3 대신 2, 3)
   - 3.3V 로직 레벨 (5V 드라이버 사용 시 레벨 시프터 필요)

## 라이선스

MIT License

## 버전 히스토리

- **v1.2.0**: 거리 계산 기능 추가
  - `calculateDistanceForTime()` 함수 추가
  - 주어진 시간 동안 이동 가능한 거리 계산
  - 삼각형/사다리꼴 프로파일 지원
  - 물리학적 공식 기반 정확한 계산
  - DistanceCalculationExample 예제 추가

- **v1.1.0**: AccelStepper 호환 기능 및 ESP32 지원 추가 (동적 타이머 적용)
  - 동적 타이머 알람 기반 제어 (펄스별 간격 동적 조정)
  - ISR 내부 논블로킹 펄스 생성
  - 펄스 간격 배열 방식의 가속/감속 처리
  - 가속/감속 제어 (선형 가속도)
  - 위치 제어 (절대/상대 위치 이동)
  - 목표 위치 도달 감지
  - AccelStepper API 호환성
  - ESP32 하드웨어 타이머 지원
  - 플랫폼별 최적화된 타이머 구현
  - 디버그 정보 및 상태 모니터링 기능 추가
  - 새로운 예제 추가 (AccelExample, ESP32Example, TimerPositionExample)

- **v1.0.0**: 초기 릴리스
  - 기본 타이머 기반 스테퍼 제어
  - 실시간 속도 조절
  - 스텝 카운팅 기능

## 기여

버그 리포트나 기능 제안은 GitHub Issues를 통해 해주세요.

## 지원

문제가 있으시면 다음을 확인해보세요:
1. 하드웨어 연결 확인
2. 전원 공급 상태 확인
3. 드라이버 설정 확인
4. 예제 코드 참조





