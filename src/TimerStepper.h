/*
  TimerStepper.h - 타이머 기반 스테퍼 모터 제어 라이브러리
  
  타이머를 사용하여 정확한 펄스 타이밍으로 스테퍼 모터를 제어합니다.
  스텝핀과 방향핀 두 개로 모터를 구동하며, 실시간 속도 조절이 가능합니다.
  
  작성자: AI Assistant
  버전: 1.0.0
  라이선스: MIT
*/

#ifndef TimerStepper_h
#define TimerStepper_h

#include <Arduino.h>

// 플랫폼별 타이머 설정
#if defined(ESP32)
  #include "esp32-hal-timer.h"
  #include "driver/timer.h"
  #include "driver/gpio.h"
#elif defined(__AVR__)
  #include <avr/interrupt.h>
#endif

class TimerStepper {
public:
    // 생성자: 스텝핀과 방향핀 설정
    TimerStepper(uint8_t stepPin, uint8_t dirPin);
    
    // ===== 타이머 기반 모터 제어 함수들 =====
    void setSpeed(float stepsPerSecond);  // 속도 설정 (스텝/초) - 타이머 기반
    void setDirection(bool clockwise);    // 방향 설정 (true: 시계방향, false: 반시계방향)
    void runSpeed();                      // 정속 회전 시작 - 타이머 기반
    void stop();                          // 모터 정지 - 타이머 기반
    void pause();                         // 모터 일시정지 - 타이머 기반
    void resume();                        // 모터 재시작 - 타이머 기반
    
    // ===== AccelStepper 호환 함수들 (타이머 기반) =====
    void setAcceleration(float acceleration);  // 가속도 설정 (스텝/초²)
    void setCurrentPosition(long position);    // 현재 위치 설정
    void moveTo(long absolute);                // 절대 위치로 이동 (타이머 기반)
    void moveTo(long absolute, float maxSpeed, float acceleration);  // 가속도/최대속도와 함께 이동
    void move(long relative);                  // 상대 위치로 이동 (타이머 기반)
    void move(long relative, float maxSpeed, float acceleration);    // 가속도/최대속도와 함께 상대 이동
    bool run();                                // 모터 상태 확인 (타이머 기반에서는 단순 상태 확인)
    void runToPosition();                      // 목표 위치까지 대기 (블로킹)
    // 타이머 기반 모드에서는 moveTo() 호출 즉시 인터럽트에서 모든 계산 처리
    
    // 상태 확인 함수들
    bool isRunning();                     // 모터가 동작 중인지 확인
    float getCurrentSpeed();              // 현재 속도 반환
    bool getDirection();                  // 현재 방향 반환
    unsigned long getStepCount();         // 스텝 카운트 반환
    void resetStepCount();                // 스텝 카운트 리셋
    
    // 위치 및 가속 관련 상태 확인
    long currentPosition();               // 현재 위치 반환
    long targetPosition();                // 목표 위치 반환
    float speed();                        // 현재 속도 반환 (getCurrentSpeed와 동일)
    float maxSpeed();                     // 최대 속도 반환
    float acceleration();                 // 가속도 반환
    long distanceToGo();                  // 남은 거리 반환 (0이면 목표 도달)
    bool isAccelerating();                // 가속 중인지 확인
    bool isDecelerating();                // 감속 중인지 확인
    
    // 타이머 설정 함수들
    void setMinPulseWidth(unsigned int microseconds);  // 최소 펄스 폭 설정
    void setMaxSpeed(float maxStepsPerSecond);         // 위치 제어용 최대 속도 제한
    
private:
    // 핀 설정
    uint8_t _stepPin;
    uint8_t _dirPin;
    
    // 모터 상태 (인터럽트에서 접근하므로 volatile)
    volatile bool _isRunning;
    volatile bool _direction;          // true: 시계방향, false: 반시계방향
    volatile float _currentSpeed;      // 현재 속도 (스텝/초)
    volatile float _maxSpeed;          // 최대 속도 (스텝/초)
    
    // 가속/감속 관련 (인터럽트에서 접근하므로 volatile)
    volatile float _acceleration;      // 가속도 (스텝/초²)
    volatile float _speed;             // 현재 속도 (스텝/초)
    volatile long _currentPos;         // 현재 위치
    volatile long _targetPos;          // 목표 위치
    volatile float _stepInterval;      // 현재 스텝 간격 (마이크로초)
    volatile unsigned long _lastStepTime;  // 마지막 스텝 시간
    volatile float _minStepInterval;   // 최소 스텝 간격 (최대 속도)
    volatile bool _isAccelerating;     // 가속 중인지
    volatile bool _isDecelerating;     // 감속 중인지
    volatile bool _wasAccelerating;    // 이전 스텝에서 가속 중이었는지 (구간 전환 감지)
    volatile bool _wasConstantSpeed;   // 이전 스텝에서 정속 중이었는지 (구간 전환 감지)
    
    // 타이머 모드 관련 (인터럽트에서 접근하므로 volatile)
    enum TimerMode {
        TIMER_MODE_SPEED,     // 정속 회전 모드
        TIMER_MODE_POSITION   // 위치 기반 제어 모드
    };
    volatile TimerMode _timerMode;     // 현재 타이머 모드
    volatile bool _positionModeActive; // 위치 모드 활성화 여부
    
    // 논블로킹 펄스 생성용 변수들 (인터럽트에서 접근하므로 volatile)
    volatile bool _pulseState;         // 펄스 상태 (HIGH/LOW)
    volatile unsigned long _pulseStartTime;  // 펄스 시작 시간
    
    // 타이밍 관련
    volatile unsigned int _minPulseWidth;      // 최소 펄스 폭 (마이크로초)
    
    // 카운터 (인터럽트에서 접근하므로 volatile)
    volatile unsigned long _stepCount;         // 총 스텝 수
    
    // 타이머 관련
    void setupTimer();
    void startTimer();
    void stopTimer();
    void switchToPositionMode();  // 위치 모드로 전환
    void switchToSpeedMode();     // 속도 모드로 전환
    
    // 가속/감속 계산 함수들
    void calculateTrajectory();  // 궤적 계산 (moveTo에서 호출)
    #if defined(ESP32)
        void IRAM_ATTR calculateNextInterval();  // 다음 펄스 간격 계산 (인터럽트에서 호출)
    #else
        void calculateNextInterval();  // 다음 펄스 간격 계산 (인터럽트에서 호출)
    #endif
    #if defined(ESP32)
        void IRAM_ATTR generateStepPulse();  // 펄스 생성 (인터럽트용)
    #else
        void generateStepPulse();  // 펄스 생성 (인터럽트용)
    #endif
    void setOutputPins(uint8_t direction);
    #if defined(ESP32)
        void IRAM_ATTR doSpeedStep();       // 정속 회전 모드용 스텝 실행
        void IRAM_ATTR doPositionStep();    // 위치 기반 제어 모드용 스텝 실행
    #else
        void doSpeedStep();       // 정속 회전 모드용 스텝 실행
        void doPositionStep();    // 위치 기반 제어 모드용 스텝 실행
    #endif
    
    // 정적 함수 (ISR에서 호출)
    static TimerStepper* _instance;
    #if defined(ESP32)
        static void IRAM_ATTR timerISR();
    #else
        static void timerISR();
    #endif
    
    // ISR에서 호출되는 실제 스텝 함수
    #if defined(ESP32)
        void IRAM_ATTR doStep();
    #else
        void doStep();
    #endif
    
    // 궤적 계산용 변수들 (인터럽트에서 접근하므로 volatile)
    volatile long _startPos;             // 이동 시작 위치
    volatile long _target1;              // 가속 구간 끝점 (정속 구간 시작)
    volatile long _target2;              // 감속 구간 시작점 (정속 구간 끝)
    volatile long _decelSteps;           // 감속 구간 스텝 수
    volatile unsigned long _constantSpeedIntervalCount;  // 정속 구간에서 건너뛸 인터럽트 수
    volatile unsigned long _intervalCounter;    // 현재 인터벌 카운트
    volatile unsigned long _lastPulseInterval;  // 마지막 펄스 인터벌 (스케일 안됨)
    volatile unsigned long _initialInterval;    // 가속 시작/감속 종료 간격 (스케일 안됨)
    volatile unsigned long _stepCounter;       // 현재 스텝 카운터 (가속/감속 계산용)
    
    // 플랫폼별 타이머 변수
    #if defined(ESP32)
        hw_timer_t* _timer;
        int _timerGroup;
        int _timerNumber;
        portMUX_TYPE _timerMux;  // 인터럽트 안전을 위한 mutex
    #elif defined(__AVR__)
        // AVR용 변수는 이미 있음
    #endif
};

#endif