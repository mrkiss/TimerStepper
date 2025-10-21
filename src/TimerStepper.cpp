/*
  TimerStepper.cpp - 동적 타이머 기반 스테퍼 모터 제어 라이브러리 (수정 버전)
  
  타이머 알람을 동적으로 설정하여 정확한 펄스 타이밍으로 스테퍼 모터를 제어합니다.
  스텝핀과 방향핀 두 개로 모터를 구동하며, 실시간 속도 조절이 가능합니다.
  
  작성자: mrkiss89@gmail.com
  버전: 1.2.0 (동적 타이머 적용)
  라이선스: MIT
*/

#include "TimerStepper.h"

// 정적 멤버 초기화
TimerStepper* TimerStepper::_instance = nullptr;

// 전역 디버그 구조체
ISRDebugInfo isrDebug = {0};

TimerStepper::TimerStepper(uint8_t stepPin, uint8_t dirPin) {
    _stepPin = stepPin;
    _dirPin = dirPin;
    
    // 초기 상태 설정 (변경 없음)
    _isRunning = false;
    _direction = true;
    _currentSpeed = 0.0;
    _maxSpeed = 1000.0;
    _stepInterval = 0;
    _minPulseWidth = 2;
    _lastStepTime = 0;
    _stepCount = 0;
    
    // 가속/감속 관련 초기화 (변경 없음)
    _acceleration = 100.0;
    _speed = 0.0;
    _currentPos = 0;
    _targetPos = 0;
    _minStepInterval = 0.0;
    _isAccelerating = false;
    _isDecelerating = false;
    _wasAccelerating = false;
    _wasConstantSpeed = false;
    
    // 타이머 모드 초기화 (변경 없음)
    _timerMode = TIMER_MODE_SPEED;
    _positionModeActive = false;
    
    // 궤적 계산용 변수 초기화 (변경 없음)
    _startPos = 0;
    _target1 = 0;
    _target2 = 0;
    _decelSteps = 0;
    _constantSpeedIntervalCount = 1;
    
    // 펄스 간격 배열 초기화
    _pulseIntervals = nullptr;
    _accelSteps = 0;
    _currentStepIndex = 0;
    
    // 논블로킹 펄스 생성용 변수 초기화 (변경 없음)
    _pulseState = false;
    _pulseStartTime = 0;
    
    // ESP32용 초기화
    #if defined(ESP32)
        _timer = nullptr;
        _timerGroup = 0;
        _timerNumber = 0;
        _timerMux = portMUX_INITIALIZER_UNLOCKED;
    #endif
    
    // 핀 모드 설정 (변경 없음)
    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    digitalWrite(_stepPin, LOW);
    digitalWrite(_dirPin, _direction ? HIGH : LOW);
    
    _instance = this;
    
    // 타이머 설정 (수정: 고정 100μs 제거, 동적 알람 준비)
    setupTimer();
}

// 소멸자: 메모리 해제
TimerStepper::~TimerStepper() {
    // 펄스 간격 배열 메모리 해제
    if (_pulseIntervals != nullptr) {
        free((void*)_pulseIntervals);
        _pulseIntervals = nullptr;
    }
    
    // 타이머 정리
    stopTimer();
    
    #if defined(ESP32)
        if (_timer != nullptr) {
            timerEnd(_timer);
            _timer = nullptr;
        }
    #endif
}

// 수정: setupTimer() - 고정 타이머 대신 동적 알람 지원으로 단순화
void TimerStepper::setupTimer() {
    #if defined(ESP32)
        // ESP32용 타이머 설정: 1MHz 주파수, 하지만 알람은 동적으로 설정
        _timer = timerBegin(1000000);  // 1MHz (1μs/tick)
        timerAttachInterrupt(_timer, &TimerStepper::timerISR);
        // 초기: 정지 상태로 알람 비활성화
        timerStop(_timer);
    #elif defined(__AVR__)
        // AVR 부분은 변경 없음 (기존 코드 유지)
        TCCR1A = 0;
        TCCR1B = 0;
        TCCR1B |= (1 << WGM12);
        TCCR1B |= (1 << CS11);  // 프리스케일러 1/8
        TIMSK1 |= (1 << OCIE1A);
        OCR1A = 0xFFFF;
    #endif
}

// setSpeed() (변경 없음, 하지만 _stepInterval을 us 단위로 계산)
void TimerStepper::setSpeed(float stepsPerSecond) {
    if (stepsPerSecond < 0) stepsPerSecond = 0;
    _currentSpeed = stepsPerSecond;
    if (stepsPerSecond == 0) {
        stop();
    } else {
        _stepInterval = (unsigned long)(1000000.0 / stepsPerSecond);
        if (_stepInterval < _minPulseWidth * 2) _stepInterval = _minPulseWidth * 2;
    }
}

// setDirection(), runSpeed(), stop(), pause(), resume(), isRunning() 등 기본 함수 (변경 없음)
void TimerStepper::setDirection(bool clockwise) {
    _direction = clockwise;
    digitalWrite(_dirPin, _direction ? HIGH : LOW);
}

void TimerStepper::runSpeed() {
    if (_currentSpeed > 0) {
        _isRunning = true;
        startTimer();  // 동적 시작
    }
}

void TimerStepper::stop() {
    _isRunning = false;
    stopTimer();
    #if defined(ESP32)
        gpio_set_level((gpio_num_t)_stepPin, 0);
    #else
        digitalWrite(_stepPin, LOW);
    #endif
    _pulseState = false;
    _pulseStartTime = 0;
    // _lastPulseInterval 제거됨 - 배열 방식으로 변경
}

void TimerStepper::pause() {
    _isRunning = false;
    stopTimer();
}

void TimerStepper::resume() {
    if (_currentSpeed > 0) {
        _isRunning = true;
        startTimer();
    }
}

bool TimerStepper::isRunning() {
    return _isRunning;
}

// 모드 전환 함수 (변경 없음)
void TimerStepper::switchToPositionMode() {
    if (_timerMode != TIMER_MODE_POSITION) {
        stop();
        _timerMode = TIMER_MODE_POSITION;
        _positionModeActive = false;
    }
}

void TimerStepper::switchToSpeedMode() {
    if (_timerMode != TIMER_MODE_SPEED) {
        stop();
        _timerMode = TIMER_MODE_SPEED;
        _positionModeActive = false;
    }
}

// getCurrentSpeed() 등 getter (변경 없음, 하지만 위치 모드에서 _lastPulseInterval 사용)
float TimerStepper::getCurrentSpeed() {
    if (_positionModeActive && _isRunning) {
        // 배열 방식으로 변경되어 현재 속도 계산 방식 수정
        if (_pulseIntervals != nullptr && _currentStepIndex > 0) {
            return 1000000.0 / (float)_pulseIntervals[_currentStepIndex - 1];
        }
        return 0.0;
    }
    return _currentSpeed;
}

bool TimerStepper::getDirection() { return _direction; }
unsigned long TimerStepper::getStepCount() { return _stepCount; }
void TimerStepper::resetStepCount() { _stepCount = 0; }
void TimerStepper::setMinPulseWidth(unsigned int microseconds) { _minPulseWidth = microseconds; }
void TimerStepper::setMaxSpeed(float maxStepsPerSecond) { _maxSpeed = maxStepsPerSecond; }

// 수정: startTimer() - 동적 _stepInterval로 알람 설정
void TimerStepper::startTimer() {
    if (_stepInterval > 0) {
        // isrDebug.isrCallCount++;
        // isrDebug.currentPulseState = _pulseState;
        // isrDebug.currentPosition = _currentPos;
        // isrDebug.targetPosition = _targetPos;
        // //isrDebug.lastPulseInterval = _lastPulseInterval;
        // isrDebug.lastPulseInterval = _stepInterval;
        // isrDebug.stepCounter = _stepCounter;
        #if defined(ESP32)
            unsigned long ticks = _stepInterval;  // 1MHz이므로 us = ticks
            if (ticks > 0xFFFFFFFFUL) ticks = 0xFFFFFFFFUL;
            portENTER_CRITICAL_ISR(&_timerMux);
            //timerWrite(_timer, ticks);    // Dynamic Timer Update 할때에 절대 쓰면 안되는 것!!
            timerAlarm(_timer, ticks, true, 0);  // 동적 알람, 자동 리로드
            timerStart(_timer);
            portEXIT_CRITICAL_ISR(&_timerMux);
        #elif defined(__AVR__)
            // AVR: 동적 OCR1A 설정
            unsigned long timerTicks = _stepInterval * 2;  // 0.5us/tick
            if (timerTicks > 65535) timerTicks = 65535;
            OCR1A = (uint16_t)timerTicks;
            TCNT1 = 0;
            TCCR1B |= (1 << CS11);
        #endif
    }
}

// stopTimer() (변경 없음)
void TimerStepper::stopTimer() {
    #if defined(ESP32)
        timerStop(_timer);
    #elif defined(__AVR__)
        TCCR1B &= ~(1 << CS11);
        TCNT1 = 0;
    #endif
}

// ISR 함수 (변경 없음)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::timerISR() {
    if (_instance != nullptr) _instance->doStep();
}
#else
void TimerStepper::timerISR() {
    if (_instance != nullptr) _instance->doStep();
}
#endif

// doStep() - 디버그 정보 업데이트 추가
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doStep() {
#else
void TimerStepper::doStep() {
#endif
    if (!_isRunning) return;
    
    // 디버그 정보 업데이트 (원자적 읽기)
    //isrDebug.isrCallCount++;
    // isrDebug.currentPulseState = _pulseState;
    // isrDebug.currentPosition = _currentPos;
    // isrDebug.targetPosition = _targetPos;
    
    // // volatile 변수이므로 직접 읽기 (컴파일러 최적화 방지)
    // isrDebug.lastPulseInterval = _lastPulseInterval;
    // isrDebug.stepCounter = _stepCounter;
    
    if (_timerMode == TIMER_MODE_POSITION && _positionModeActive) {
        doPositionStep();
    } else {
        doSpeedStep();
    }
}

// 수정: doSpeedStep() - 펄스 후 동적 다음 간격 재설정
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doSpeedStep() {
#else
void TimerStepper::doSpeedStep() {
#endif
    
        // 펄스 시작
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 1);
        #else
            digitalWrite(_stepPin, HIGH);
        #endif
        
        // 디버그 정보 업데이트
        strcpy((char*)isrDebug.lastAction, "Speed Pulse HIGH");
        isrDebug.lastPulseHighTime = millis();
   
        delayMicroseconds(_minPulseWidth);
            #if defined(ESP32)
                gpio_set_level((gpio_num_t)_stepPin, 0);
            #else
                digitalWrite(_stepPin, LOW);
            #endif
            
            // 디버그 정보 업데이트
            strcpy((char*)isrDebug.lastAction, "Speed Pulse LOW");
            isrDebug.lastPulseLowTime = millis();
            _stepCount += _direction ? 1 : -1;
           
}

// 수정: doPositionStep() - 펄스 후 calculateNextInterval() 호출 및 동적 타이머 업데이트
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doPositionStep() {
#else
void TimerStepper::doPositionStep() {
#endif
    if (_currentPos == _targetPos) {
        stop();
        _positionModeActive = false;
        return;
    }
    
 
  
        // 펄스 시작 조건 (간단화: 타이머 알람으로 트리거되므로 즉시 시작)
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 1);
        #else
            digitalWrite(_stepPin, HIGH);
        #endif
        _pulseState = true;
        
        // 디버그 정보 업데이트
        strcpy((char*)isrDebug.lastAction, "Pos Pulse HIGH");
        isrDebug.lastPulseHighTime = millis();
        
        // 펄스 시작 직후 다음 간격 계산
        getNextPulseInterval();
        // 위치 업데이트는 펄스 종료 시
   
        
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 0);
        #else
            digitalWrite(_stepPin, LOW);
        #endif
        
        // 디버그 정보 업데이트
        strcpy((char*)isrDebug.lastAction, "Pos Pulse LOW");
        isrDebug.lastPulseLowTime = millis();
            
        // 위치 업데이트
        _currentPos += _direction ? 1 : -1;
            
        if (_currentPos == _targetPos) {
                stop();
                _positionModeActive = false;
                strcpy((char*)isrDebug.lastAction, "Target Reached");
        }
     
    
}

// 신규: updateNextInterval() - 동적 타이머 재설정 헬퍼 (ISR 안전)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::updateNextInterval(unsigned long nextInterval) {
#else
void TimerStepper::updateNextInterval(unsigned long nextInterval) {
#endif
    // _lastPulseInterval 제거됨 - 배열 방식으로 변경
    isrDebug.isrCallCount++;
    isrDebug.currentPulseState = _pulseState;
    isrDebug.currentPosition = _currentPos;
    isrDebug.targetPosition = _targetPos;
    isrDebug.lastPulseInterval = nextInterval;
    isrDebug.stepCounter = _currentStepIndex;
   
    if (nextInterval > 0) {
        #if defined(ESP32)
        strcpy((char*)isrDebug.lastAction, "Timer Update");
        isrDebug.timerUpdateTime = millis();
            unsigned long ticks = nextInterval;
            if (ticks > 0xFFFFFFFFUL) ticks = 0xFFFFFFFFUL;
            portENTER_CRITICAL_ISR(&_timerMux);
            //timerWrite(_timer, ticks);    // Dynamic Timer Update 할때에 절대 쓰면 안되는 것!!
            timerAlarm(_timer, ticks, true, 0);
            portEXIT_CRITICAL_ISR(&_timerMux);
        #elif defined(__AVR__)
            unsigned long timerTicks = nextInterval * 2;
            if (timerTicks > 65535) timerTicks = 65535;
            OCR1A = (uint16_t)timerTicks;
            TCNT1 = 0;
        #endif
    }
}

// getNextPulseInterval() - 배열에서 다음 펄스 간격 조회 (최적화됨)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::getNextPulseInterval() {
#else
void TimerStepper::getNextPulseInterval() {
#endif
    unsigned long nextInterval;
    bool isAccelerating, isConstantSpeed, isDecelerating;
    
    // 현재 구간 판단
    if (_direction) {
        isAccelerating = _currentPos < _target1;
        isConstantSpeed = (_target1 != _target2) && (_currentPos >= _target1 && _currentPos < _target2);
        isDecelerating = _currentPos >= _target2;
    } else {
        isAccelerating = _currentPos > _target1;
        isConstantSpeed = (_target1 != _target2) && (_currentPos <= _target1 && _currentPos > _target2);
        isDecelerating = _currentPos <= _target2;
    }
    
    if (isAccelerating) {
        // 가속 구간: 배열에서 순서대로 조회
        if (_pulseIntervals != nullptr && _currentStepIndex < _accelSteps) {
            nextInterval = _pulseIntervals[_currentStepIndex];
            _currentStepIndex++;
        } else {
            nextInterval = _constantSpeedIntervalCount;  // 안전값
        }
    } else if (isConstantSpeed) {
        // 정속 구간
        nextInterval = _constantSpeedIntervalCount;
    } else if (isDecelerating) {
        // 감속 구간: 배열을 뒤에서부터 조회
        if (_pulseIntervals != nullptr && _currentStepIndex > 0) {
            _currentStepIndex--;
            nextInterval = _pulseIntervals[_currentStepIndex];
        } else {
            nextInterval = _constantSpeedIntervalCount;  // 안전값
        }
    } else {
        nextInterval = 100000;  // 100ms 안전값
    }
    
    // 동적 타이머 업데이트
    updateNextInterval(nextInterval);
    
    _wasAccelerating = isAccelerating;
    _wasConstantSpeed = isConstantSpeed;
}

// AccelStepper 호환 함수들 (moveTo 등: 변경 없음, 하지만 calculateTrajectory에서 _constantSpeedIntervalCount를 us 단위로 조정)
void TimerStepper::setAcceleration(float acceleration) {
    if (acceleration == 0.0) return;
    _acceleration = acceleration;
}

void TimerStepper::setCurrentPosition(long position) {
    _currentPos = position;
    _targetPos = position;
    // _stepInterval = 0;
    _speed = 0.0;
    _isAccelerating = false;
    _isDecelerating = false;
}

void TimerStepper::moveTo(long absolute) {
    if (_targetPos != absolute) {
        if (_acceleration <= 0.0) {
            Serial.println("경고: 가속도가 설정되지 않았습니다. setAcceleration()을 먼저 호출하세요.");
            return;
        }
        if (_maxSpeed <= 0.0) {
            Serial.println("경고: 최대속도가 설정되지 않았습니다. setMaxSpeed()을 먼저 호출하세요.");
            return;
        }
        
        switchToPositionMode();
        _targetPos = absolute;
        
        long distanceTo = _targetPos - _currentPos;
        if (distanceTo > 0) _direction = true; else _direction = false;
        setOutputPins(_direction);
        
        _pulseState = false;
        _pulseStartTime = 0;
        _wasAccelerating = false;
        _wasConstantSpeed = false;
        _currentStepIndex = 0;
        
        calculateTrajectory();
        
        // 디버그: calculateTrajectory() 후 상태 확인
        // Serial.printf("DEBUG moveTo: calculateTrajectory() 완료 후 _accelSteps=%lu\n", _accelSteps);
        
        // 위치 제어 모드에서는 첫 번째 펄스 간격으로 설정
        if (_pulseIntervals != nullptr && _accelSteps > 0) {
            _stepInterval = _pulseIntervals[0];
        } else {
            _stepInterval = _constantSpeedIntervalCount;
        }
        
        // 타이머 시작 (startTimer 사용)
        startTimer();
       
        _positionModeActive = true;
        _isRunning = true;
        
        // Serial.printf("DEBUG moveTo: 타이머 시작 완료, _stepInterval=%lu\n", _stepInterval);
    }
}

void TimerStepper::move(long relative) {
    moveTo(_currentPos + relative);
}

long TimerStepper::currentPosition() { return _currentPos; }
long TimerStepper::targetPosition() { return _targetPos; }
float TimerStepper::speed() { return getCurrentSpeed(); }
float TimerStepper::maxSpeed() { return _maxSpeed; }
float TimerStepper::acceleration() { return _acceleration; }
long TimerStepper::distanceToGo() { return _targetPos - _currentPos; }
bool TimerStepper::isAccelerating() { return _isAccelerating; }
bool TimerStepper::isDecelerating() { return _isDecelerating; }

// calculateTrajectory() (수정: 간격을 us 단위로 계산)
void TimerStepper::calculateTrajectory() {
    _startPos = _currentPos;
    long distanceTo = _targetPos - _currentPos;
    if (distanceTo == 0) return;
    
    long absDistance = abs(distanceTo);
    float accelDistance = (_maxSpeed * _maxSpeed) / (2.0 * _acceleration);
    float decelDistance = accelDistance;
    
    // 디버그: 계산 과정 확인
    //Serial.printf("DEBUG calculateTrajectory: distance=%ld, accelDistance=%.2f, _acceleration=%.2f, _maxSpeed=%.2f\n",
                  absDistance, accelDistance, _acceleration, _maxSpeed);
    
    if (absDistance >= (accelDistance + decelDistance)) {
        // 사다리꼴
        _target1 = _currentPos + (distanceTo > 0 ? accelDistance : -accelDistance);
        _target2 = _targetPos - (distanceTo > 0 ? decelDistance : -decelDistance);
        _constantSpeedIntervalCount = (unsigned long)(1000000.0 / _maxSpeed);  // us 단위로 수정
        if (_constantSpeedIntervalCount < 1) _constantSpeedIntervalCount = 1;
    } else {
        // 삼각형
        float halfDistance = absDistance / 2.0;
        _target1 = _currentPos + (distanceTo > 0 ? halfDistance : -halfDistance);
        _target2 = _target1;
        float limitedMaxSpeed = sqrt(_acceleration * absDistance);
        _constantSpeedIntervalCount = (unsigned long)(1000000.0 / limitedMaxSpeed);
        if (_constantSpeedIntervalCount < 1) _constantSpeedIntervalCount = 1;
    }
    
    _accelSteps = abs(_target1 - _currentPos);
    _decelSteps = abs(_targetPos - _target2);
    
    // 기존 배열 해제
    if (_pulseIntervals != nullptr) {
        free((void*)_pulseIntervals);
        _pulseIntervals = nullptr;
    }
    
    if (_accelSteps > 0) {
        // 가속 구간 펄스 간격 배열 생성
        _pulseIntervals = (volatile unsigned long*)malloc(_accelSteps * sizeof(unsigned long));
        if (_pulseIntervals == nullptr) {
            //Serial.println("ERROR: 펄스 간격 배열 메모리 할당 실패");
            return;
        }
        
        // 가속 구간 펄스 간격 미리 계산
        for (unsigned long i = 0; i < _accelSteps; i++) {
            unsigned long stepNum = i + 1;  // 1부터 시작
            float tempInterval = (sqrt((2.0 * stepNum) / _acceleration) - 
                                sqrt((2.0 * (stepNum - 1)) / _acceleration)) * 1000000.0;
            unsigned long interval = (unsigned long)tempInterval;
            
            // 최소값 제한: 정속 간격보다 작아지지 않도록
            if (interval < _constantSpeedIntervalCount) {
                interval = _constantSpeedIntervalCount;
            }
            
            _pulseIntervals[i] = interval;
        }
        
        // 초기 간격 설정
        _currentStepIndex = 0;
        
        //Serial.printf("DEBUG: 펄스 간격 배열 생성 완료, _accelSteps=%lu\n", _accelSteps);
    } else {
        _pulseIntervals = nullptr;
        _currentStepIndex = 0;
        //Serial.println("DEBUG: 가속 구간 없음, 정속 모드");
    }
    
}

void TimerStepper::setOutputPins(uint8_t direction) {
    #if defined(ESP32)
        gpio_set_level((gpio_num_t)_dirPin, direction ? 1 : 0);
    #else
        digitalWrite(_dirPin, direction ? HIGH : LOW);
    #endif
}

// AVR ISR (변경 없음)
#if defined(__AVR__)
ISR(TIMER1_COMPA_vect) {
    TimerStepper::timerISR();
}
#endif

// 디버그 함수들 구현
void TimerStepper::debugISRStatus() {
    Serial.println("=== ISR 디버그 상태 ===");
    Serial.printf("ISR 호출 횟수: %lu\n", isrDebug.isrCallCount);
    Serial.printf("현재 펄스 상태: %s\n", isrDebug.currentPulseState ? "HIGH" : "LOW");
    Serial.printf("현재 위치: %ld\n", isrDebug.currentPosition);
    Serial.printf("목표 위치: %ld\n", isrDebug.targetPosition);
    Serial.printf("마지막 펄스 간격: %lu us\n", isrDebug.lastPulseInterval);
    Serial.printf("스텝 카운터: %lu\n", isrDebug.stepCounter);
    Serial.printf("마지막 작업: %s\n", isrDebug.lastAction);
    Serial.printf("타이머 업데이트 시간: %lu ms\n", isrDebug.timerUpdateTime);
    Serial.printf("마지막 HIGH 시간: %lu ms\n", isrDebug.lastPulseHighTime);
    Serial.printf("마지막 LOW 시간: %lu ms\n", isrDebug.lastPulseLowTime);
    Serial.println("========================");
}

void TimerStepper::debugSimple() {
    Serial.printf("ISR: %lu회, Pos: %ld/%ld, Pulse: %s, 마지막 펄스 간격: %lu\n",
                  isrDebug.isrCallCount,
                  isrDebug.currentPosition,
                  isrDebug.targetPosition,
                  isrDebug.currentPulseState ? "HIGH" : "LOW",
                  isrDebug.lastPulseInterval);
}

void TimerStepper::debugDetailed() {
    Serial.println("=== 상세 디버그 정보 ===");
    debugISRStatus();
    Serial.printf("모터 상태: %s\n", _isRunning ? "실행중" : "정지");
    Serial.printf("타이머 모드: %s\n", (_timerMode == TIMER_MODE_POSITION) ? "위치제어" : "정속회전");
    Serial.printf("위치모드 활성: %s\n", _positionModeActive ? "예" : "아니오");
    Serial.printf("방향: %s\n", _direction ? "시계방향" : "반시계방향");
    Serial.printf("현재 속도: %.2f 스텝/초\n", getCurrentSpeed());
    Serial.printf("최대 속도: %.2f 스텝/초\n", _maxSpeed);
    Serial.printf("가속도: %.2f 스텝/초²\n", _acceleration);
    Serial.printf("최소 펄스 폭: %u μs\n", _minPulseWidth);
    Serial.printf("스텝 간격: %lu μs\n", _stepInterval);
    Serial.printf("목표1: %ld, 목표2: %ld\n", _target1, _target2);
    Serial.printf("감속 스텝: %ld\n", _decelSteps);
    Serial.printf("가속 구간 스텝 수: %lu\n", _accelSteps);
    Serial.printf("정속 간격: %lu μs\n", _constantSpeedIntervalCount);
    Serial.println("========================");
}

void TimerStepper::resetISRDebugCount() {
    isrDebug.isrCallCount = 0;
    Serial.println("ISR 디버그 카운터 리셋됨");
}

// 거리 계산 함수들 구현
long TimerStepper::calculateDistanceForTime(float timeSeconds) {
    return calculateDistanceForTime(timeSeconds, _maxSpeed, _acceleration);
}

long TimerStepper::calculateDistanceForTime(float timeSeconds, float maxSpeed, float acceleration) {
    if (timeSeconds <= 0 || maxSpeed <= 0 || acceleration <= 0) {
        return 0;
    }
    
    // 가속 구간에서 최대 속도에 도달하는 시간 계산
    float timeToMaxSpeed = maxSpeed / acceleration;
    
    // 감속 구간 시간 (가속과 동일)
    float timeToDecel = timeToMaxSpeed;
    
    // 총 가속/감속 시간
    float totalAccelDecelTime = timeToMaxSpeed + timeToDecel;
    
    if (timeSeconds <= totalAccelDecelTime) {
        // 삼각형 프로파일: 가속/감속만으로 시간이 끝남
        // 가속 구간에서의 거리 = 0.5 * acceleration * t^2
        // 감속 구간에서의 거리 = maxSpeed * t - 0.5 * acceleration * t^2
        
        if (timeSeconds <= timeToMaxSpeed) {
            // 아직 가속 중
            return (long)(0.5 * acceleration * timeSeconds * timeSeconds);
        } else {
            // 가속 완료 후 감속 중
            float accelDistance = 0.5 * acceleration * timeToMaxSpeed * timeToMaxSpeed;
            float remainingTime = timeSeconds - timeToMaxSpeed;
            float decelDistance = maxSpeed * remainingTime - 0.5 * acceleration * remainingTime * remainingTime;
            return (long)(accelDistance + decelDistance);
        }
    } else {
        // 사다리꼴 프로파일: 가속 + 정속 + 감속
        float constantSpeedTime = timeSeconds - totalAccelDecelTime;
        
        // 가속 구간 거리
        float accelDistance = 0.5 * acceleration * timeToMaxSpeed * timeToMaxSpeed;
        
        // 정속 구간 거리
        float constantDistance = maxSpeed * constantSpeedTime;
        
        // 감속 구간 거리 (가속과 동일)
        float decelDistance = accelDistance;
        
        return (long)(accelDistance + constantDistance + decelDistance);
    }
}