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

### 1. 타이머 기반 모드 (실시간 속도 제어)

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
  stepper.start();  // 타이머 시작
  delay(5000);
  stepper.stop();   // 타이머 정지
  delay(1000);
}
```

### 2. AccelStepper 호환 모드 (가속/감속 및 위치 제어)

가속/감속 곡선과 위치 기반 제어를 제공합니다.

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
  stepper.moveTo(2000);
  while (stepper.distanceToGo() != 0) {
    stepper.run();  // 가속/감속 계산 및 실행
  }
  
  // 방법 2: moveTo with parameters (가속도/최대속도 직접 지정)
  stepper.moveTo(0, 800.0, 300.0);  // 최대속도 800, 가속도 300
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  // 방법 3: 상대 위치 이동
  stepper.move(1000);  // 현재 위치에서 +1000 이동
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
}
```

### 두 모드의 차이점

| 특징 | 타이머 기반 모드 | AccelStepper 호환 모드 |
|------|------------------|------------------------|
| **제어 방식** | 실시간 속도 제어 | 위치 기반 제어 |
| **가속/감속** | 없음 (즉시 속도 변경) | 부드러운 가속/감속 곡선 |
| **사용 함수** | `setSpeed()`, `start()`, `stop()` | `moveTo()`, `move()`, `run()` |
| **타이밍** | 하드웨어 타이머 (정확) | 소프트웨어 타이밍 |
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
void start()                            // 모터 시작
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

### 가속/감속 제어 함수 (AccelStepper 호환)

```cpp
void setAcceleration(float acceleration)          // 가속도 설정 (스텝/초²)
void setCurrentPosition(long position)            // 현재 위치 설정
void moveTo(long absolute)                        // 절대 위치로 이동
void move(long relative)                          // 상대 위치로 이동
void run()                                        // 가속/감속 계산 및 실행
void runSpeed()                                   // 현재 속도로 실행
void runToPosition()                              // 목표 위치까지 실행
void runToNewPosition(long position)              // 새 위치로 이동 후 정지
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
- moveTo, move, runToNewPosition 함수 테스트
- 진행 상황 실시간 출력
- MS 핀 설정 포함

### TimerPositionExample
타이머 기반 위치 제어의 장점을 보여주는 예제입니다:
- 메인 루프를 블로킹하지 않는 위치 제어
- 동시에 다른 작업 수행 가능 (LED 제어, 센서 읽기 등)
- 복합 이동 시퀀스
- 실시간 상태 모니터링

## 기술적 세부사항

### 타이머 설정

#### AVR 기반 (Arduino Uno, Nano, Mega 등)
- **Timer1** 사용 (16비트 타이머)
- **CTC 모드** (Clear Timer on Compare Match)
- **프리스케일러**: 1/8 (16MHz → 2MHz)
- **해상도**: 0.5마이크로초

#### ESP32 기반
- **하드웨어 타이머** 사용 (80MHz → 1MHz)
- **자동 리로드 모드**
- **해상도**: 1마이크로초
- **32비트 타이머** (더 넓은 범위 지원)

### 펄스 생성
- 최소 펄스 폭 보장
- 인터럽트 기반 정확한 타이밍
- 속도 제한 기능

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

- **v1.1.0**: AccelStepper 호환 기능 및 ESP32 지원 추가
  - 가속/감속 제어 (선형 가속도)
  - 위치 제어 (절대/상대 위치 이동)
  - 목표 위치 도달 감지
  - AccelStepper API 호환성
  - ESP32 하드웨어 타이머 지원
  - 플랫폼별 최적화된 타이머 구현
  - 새로운 예제 추가 (AccelExample, ESP32Example)

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





