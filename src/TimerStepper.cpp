/*
  TimerStepper.cpp - 타이머 기반 스테퍼 모터 제어 라이브러리
  
  타이머를 사용하여 정확한 펄스 타이밍으로 스테퍼 모터를 제어합니다.
  스텝핀과 방향핀 두 개로 모터를 구동하며, 실시간 속도 조절이 가능합니다.
  
  작성자: AI Assistant
  버전: 1.0.0
  라이선스: MIT
*/

#include "TimerStepper.h"

// 정적 멤버 초기화
TimerStepper* TimerStepper::_instance = nullptr;

TimerStepper::TimerStepper(uint8_t stepPin, uint8_t dirPin) {
    _stepPin = stepPin;
    _dirPin = dirPin;
    
    // 초기 상태 설정
    _isRunning = false;
    _direction = true;  // 기본값: 시계방향
    _currentSpeed = 0.0;
    _maxSpeed = 1000.0;  // 기본 최대 속도: 1000 스텝/초
    _stepInterval = 0;
    _minPulseWidth = 5;  // 기본 최소 펄스 폭: 5마이크로초
    _lastStepTime = 0;
    _stepCount = 0;
    
    // 가속/감속 관련 초기화
    _acceleration = 100.0;  // 기본 가속도: 100 스텝/초²
    _speed = 0.0;
    _currentPos = 0;
    _targetPos = 0;
    _stepInterval = 0.0;
    _minStepInterval = 0.0;
    _isAccelerating = false;
    _isDecelerating = false;
    _wasAccelerating = false;
    _wasConstantSpeed = false;
    
    // 타이머 모드 초기화
    _timerMode = TIMER_MODE_SPEED;  // 기본값: 정속 회전 모드
    _positionModeActive = false;
    
    // 궤적 계산용 변수 초기화
    _startPos = 0;
    _target1 = 0;
    _target2 = 0;
    _pulseIntervalIncrement = 0.0;
    _fractionalAccumulator = 0.0;
    _constantSpeedIntervalCount = 1;
    _intervalCounter = 0;
    _lastPulseInterval = 0;
    _scaledPulseInterval = 0;
    _initialInterval = 0;
    
    // 논블로킹 펄스 생성용 변수 초기화
    _pulseState = false;
    _pulseStartTime = 0;
    // 인터럽트에서 모든 계산을 처리하므로 플래그 변수 불필요
    
    // 플랫폼별 타이머 초기화
    #if defined(ESP32)
        _timer = nullptr;
        _timerGroup = 0;
        _timerNumber = 0;
        _timerMux = portMUX_INITIALIZER_UNLOCKED;  // mutex 초기화
    #endif
    
    // 핀 모드 설정
    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    
    // 초기 핀 상태
    digitalWrite(_stepPin, LOW);
    digitalWrite(_dirPin, _direction ? HIGH : LOW);
    
    // 싱글톤 인스턴스 설정
    _instance = this;
    
    // 타이머 설정
    setupTimer();
}

void TimerStepper::setupTimer() {
    #if defined(ESP32)
        // ESP32용 타이머 설정 (Arduino Core 3.3.0+)
        _timer = timerBegin(1000000);  // 1MHz 주파수
        timerAttachInterrupt(_timer, &TimerStepper::timerISR);
        timerWrite(_timer, 100);  // 100마이크로초 (비활성화 상태)
        timerAlarm(_timer, 100, true, 0);  // 자동 리로드 활성화
        timerStop(_timer);  // 초기에는 정지 상태
    #elif defined(__AVR__)
        // AVR용 Timer1 설정 (16비트 타이머)
    // CTC 모드 (Clear Timer on Compare Match)
    TCCR1A = 0;  // Normal port operation
    TCCR1B = 0;  // 타이머 정지
    
    // CTC 모드 설정
    TCCR1B |= (1 << WGM12);
    
    // 프리스케일러 설정 (1/8)
    // 16MHz / 8 = 2MHz, 1 tick = 0.5마이크로초
    TCCR1B |= (1 << CS11);
    
    // 인터럽트 활성화
    TIMSK1 |= (1 << OCIE1A);
    
    // 초기 비교값 설정 (타이머 정지 상태)
    OCR1A = 0xFFFF;  // 최대값으로 설정하여 인터럽트 비활성화
    #endif
}

void TimerStepper::setSpeed(float stepsPerSecond) {
    // 속도 제한 (음수만 방지)
    if (stepsPerSecond < 0) {
        stepsPerSecond = 0;
    }
    
    _currentSpeed = stepsPerSecond;
    
    if (stepsPerSecond == 0) {
        stop();
    } else {
        // 스텝 간격 계산 (마이크로초)
        _stepInterval = (unsigned long)(1000000.0 / stepsPerSecond);
        
        // 최소 펄스 폭 고려
        if (_stepInterval < _minPulseWidth * 2) {
            _stepInterval = _minPulseWidth * 2;
        }
        
    }
}

void TimerStepper::setDirection(bool clockwise) {
    _direction = clockwise;
    digitalWrite(_dirPin, _direction ? HIGH : LOW);
}

void TimerStepper::runSpeed() {
        if (_currentSpeed > 0) {
            _isRunning = true;
            startTimer();
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
    
    // 펄스 상태 리셋
    _pulseState = false;
    _pulseStartTime = 0;
    
    // 스케일된 간격 리셋 (다음 moveTo를 위해)
    _scaledPulseInterval = 0;
    _lastPulseInterval = 0;
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

// 위치 모드로 전환
void TimerStepper::switchToPositionMode() {
    if (_timerMode != TIMER_MODE_POSITION) {
        stop();  // 현재 동작 정지
        _timerMode = TIMER_MODE_POSITION;
        _positionModeActive = false;
    }
}

// 속도 모드로 전환
void TimerStepper::switchToSpeedMode() {
    if (_timerMode != TIMER_MODE_SPEED) {
        stop();  // 현재 동작 정지
        _timerMode = TIMER_MODE_SPEED;
        _positionModeActive = false;
    }
}

float TimerStepper::getCurrentSpeed() {
    // 위치 제어 모드에서는 실제 펄스 간격으로부터 속도 계산
    if (_positionModeActive && _isRunning) {
        if (_lastPulseInterval > 0) {
            // 타이머 간격 100마이크로초 기준
            // 실제 스텝 간격 = _lastPulseInterval * 100 마이크로초
            // 속도 (스텝/초) = 1000000 / (간격 마이크로초) = 10000 / _lastPulseInterval
            return 10000.0 / (float)_lastPulseInterval;
        }
        return 0.0;
    }
    
    // 정속 회전 모드에서는 _currentSpeed 사용
    return _currentSpeed;
}

bool TimerStepper::getDirection() {
    return _direction;
}

unsigned long TimerStepper::getStepCount() {
    return _stepCount;
}

void TimerStepper::resetStepCount() {
    _stepCount = 0;
}

void TimerStepper::setMinPulseWidth(unsigned int microseconds) {
    _minPulseWidth = microseconds;
}

void TimerStepper::setMaxSpeed(float maxStepsPerSecond) {
    _maxSpeed = maxStepsPerSecond;
    // 정속 회전 모드에서는 현재 속도에 영향 주지 않음
}

void TimerStepper::startTimer() {
    if (_stepInterval > 0) {
        #if defined(ESP32)
            // ESP32: 마이크로초를 타이머 틱으로 변환
            unsigned long timerTicks = (unsigned long)_stepInterval;
            
            // ESP32 타이머 제한 확인 (32비트)
            if (timerTicks > 0xFFFFFFFF) {
                timerTicks = 0xFFFFFFFF;
            }
            
            timerWrite(_timer, timerTicks);
            timerAlarm(_timer, timerTicks, true, 0);  // 자동 리로드 활성화
            timerStart(_timer);
        #elif defined(__AVR__)
            // AVR: 마이크로초를 타이머 틱으로 변환
            // 1 tick = 0.5마이크로초 (2MHz)
            unsigned long timerTicks = _stepInterval * 2;
            
            // 16비트 타이머 제한 확인
            if (timerTicks > 65535) {
                timerTicks = 65535;
            }
            
            OCR1A = (uint16_t)timerTicks;
            TCNT1 = 0;  // 타이머 카운터 리셋
            // 타이머 시작
            TCCR1B |= (1 << CS11);  // 프리스케일러 활성화
        #endif
    }
}

void TimerStepper::stopTimer() {
    #if defined(ESP32)
        timerStop(_timer);
    #elif defined(__AVR__)
    // 타이머 정지
    TCCR1B &= ~(1 << CS11);  // 프리스케일러 비활성화
    TCNT1 = 0;  // 타이머 카운터 리셋
    #endif
}


// 정적 ISR 함수
#if defined(ESP32)
void IRAM_ATTR TimerStepper::timerISR() {
    if (_instance != nullptr) {
        _instance->doStep();
    }
}
#else
void TimerStepper::timerISR() {
    if (_instance != nullptr) {
        _instance->doStep();
    }
}
#endif

// 실제 스텝 실행 함수
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doStep() {
#else
void TimerStepper::doStep() {
#endif
    if (!_isRunning) {
        return;
    }
    
    // 타이머 모드에 따라 다른 동작
    if (_timerMode == TIMER_MODE_POSITION && _positionModeActive) {
        // 위치 기반 제어 모드
        doPositionStep();
    } else {
        // 정속 회전 모드
        doSpeedStep();
    }
}

// 정속 회전 모드용 스텝 실행
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doSpeedStep() {
#else
void TimerStepper::doSpeedStep() {
#endif
    unsigned long currentTime = micros();
    
    if (!_pulseState) {
        // 펄스 시작
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 1);
        #else
    digitalWrite(_stepPin, HIGH);
        #endif
        _pulseState = true;
        _pulseStartTime = currentTime;
    } else if (currentTime - _pulseStartTime >= _minPulseWidth) {
        // 펄스 종료
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 0);
        #else
    digitalWrite(_stepPin, LOW);
        #endif
        _pulseState = false;
    
    // 스텝 카운트 업데이트
    if (_direction) {
        _stepCount++;
    } else {
        _stepCount--;
    }
    
        _lastStepTime = currentTime;
    }
}

// 위치 기반 제어 모드용 스텝 실행 (가속/감속 기반)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::doPositionStep() {
#else
void TimerStepper::doPositionStep() {
#endif
    // 목표 도달 확인
    if (_currentPos == _targetPos) {
        stop();
        _positionModeActive = false;
        return;
    }
    
    // 펄스 상태 머신 처리 (최적화된 버전)
    if (!_pulseState) {
        // 단순 카운트만 수행
        _intervalCounter++;
        
        if (_intervalCounter >= _lastPulseInterval) {
            // 펄스 시작
            #if defined(ESP32)
                gpio_set_level((gpio_num_t)_stepPin, 1);
            #else
                digitalWrite(_stepPin, HIGH);
            #endif
            _pulseState = true;
            _pulseStartTime = 0;  // 타이머 기반
            _intervalCounter = 0;
            
            // 펄스 생성 직후에만 다음 간격 계산
            calculateNextInterval();
        }
    } else {
        // 펄스 종료 조건: 타이머 틱 기반으로 변경
        // _minPulseWidth를 타이머 틱으로 변환 (100마이크로초 기준)
        unsigned long minPulseTicks = _minPulseWidth / 100;
        if (minPulseTicks < 1) minPulseTicks = 1;
        
        _intervalCounter++;  // 펄스 종료를 위한 카운트도 증가
        
        if (_intervalCounter >= minPulseTicks) {
            // 펄스 종료
            #if defined(ESP32)
                gpio_set_level((gpio_num_t)_stepPin, 0);
            #else
                digitalWrite(_stepPin, LOW);
            #endif
            _pulseState = false;
            
            // 위치 업데이트
            if (_direction) {
                _currentPos++;
            } else {
                _currentPos--;
            }
            
            // 위치 업데이트 후 목표 도달 확인
            if (_currentPos == _targetPos) {
                stop();
                _positionModeActive = false;
            }
        }
    }
}


// 펄스 생성 함수 (인터럽트용)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::generateStepPulse() {
#else
void TimerStepper::generateStepPulse() {
#endif
    unsigned long currentTime = micros();
    
    if (!_pulseState) {
        // 펄스 시작
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 1);
        #else
            digitalWrite(_stepPin, HIGH);
        #endif
        _pulseState = true;
        _pulseStartTime = currentTime;
    } else if (currentTime - _pulseStartTime >= _minPulseWidth) {
        // 펄스 종료
        #if defined(ESP32)
            gpio_set_level((gpio_num_t)_stepPin, 0);
        #else
            digitalWrite(_stepPin, LOW);
        #endif
        _pulseState = false;
        
        // 위치 업데이트
        if (_direction) {
            _currentPos++;
        } else {
            _currentPos--;
        }
        
        _lastStepTime = currentTime;
    }
}



// 플랫폼별 ISR 정의
#if defined(__AVR__)
// AVR용 ISR 정의 (Timer1 Compare Match A)
ISR(TIMER1_COMPA_vect) {
    TimerStepper::timerISR();
}
#endif

// ===== AccelStepper 호환 함수들 =====

void TimerStepper::setAcceleration(float acceleration) {
    if (acceleration == 0.0) {
        return;
    }
    _acceleration = acceleration;
}


void TimerStepper::setCurrentPosition(long position) {
    _currentPos = position;
    _targetPos = position;
    _stepInterval = 0;
    _speed = 0.0;
    _isAccelerating = false;
    _isDecelerating = false;
}

void TimerStepper::moveTo(long absolute) {
    if (_targetPos != absolute) {
 
        
        // 위치 모드로 전환
        switchToPositionMode();
        
        _targetPos = absolute;
        
        // 초기 방향 설정
        long distanceTo = _targetPos - _currentPos;
        if (distanceTo > 0) {
            _direction = true;
        } else {
            _direction = false;
        }
        setOutputPins(_direction);
        
        // 펄스 상태 및 구간 전환 상태 리셋
        _pulseState = false;
        _pulseStartTime = 0;
        _wasAccelerating = false;
        _wasConstantSpeed = false;
        _fractionalAccumulator = 0.0;  // 소수점 누적 변수 초기화
        
        // 궤적 미리 계산 (가속/정속/감속 구간과 타겟 포인트들)
        // 주의: _scaledPulseInterval은 calculateTrajectory()에서 초기화됨
        calculateTrajectory();
        
        // 위치 제어용 타이머 간격 설정 (100마이크로초)
        _stepInterval = 100;
        
        // 타이머 기반으로 시작 (인터럽트에서 단순 실행만)
        _positionModeActive = true;
        _isRunning = true;
        startTimer();
    }
}



void TimerStepper::move(long relative) {
    moveTo(_currentPos + relative);
}


// 타이머 기반 모드에서는 run() 루프가 불필요
// moveTo() 호출 즉시 인터럽트에서 모든 계산 처리

long TimerStepper::currentPosition() {
    // 인터럽트 안전을 위해 변수 복사
    long currentPos = _currentPos;
    return currentPos;
}

long TimerStepper::targetPosition() {
    // 인터럽트 안전을 위해 변수 복사
    long targetPos = _targetPos;
    return targetPos;
}

float TimerStepper::speed() {
    // getCurrentSpeed()와 동일한 로직 사용
    return getCurrentSpeed();
}

float TimerStepper::maxSpeed() {
    return _maxSpeed;
}

float TimerStepper::acceleration() {
    return _acceleration;
}

long TimerStepper::distanceToGo() {
    // 인터럽트 안전을 위해 인터럽트 비활성화
    #if defined(ESP32)
        portENTER_CRITICAL_ISR(&_timerMux);
        long result = _targetPos - _currentPos;
        portEXIT_CRITICAL_ISR(&_timerMux);
        return result;
    #else
        noInterrupts();
        long result = _targetPos - _currentPos;
        interrupts();
        return result;
    #endif
}

bool TimerStepper::isAccelerating() {
    // 인터럽트 안전을 위해 변수 복사
    bool isAccel = _isAccelerating;
    return isAccel;
}

bool TimerStepper::isDecelerating() {
    // 인터럽트 안전을 위해 변수 복사
    bool isDecel = _isDecelerating;
    return isDecel;
}



// 궤적 계산 함수 (moveTo에서 호출)
void TimerStepper::calculateTrajectory() {
    // 시작 위치 저장
    _startPos = _currentPos;
    
    long distanceTo = _targetPos - _currentPos;
    if (distanceTo == 0) return;
    
    // 거리의 절댓값
    long absDistance = abs(distanceTo);
    
    // 가속 구간에서 최고속도에 도달하는데 필요한 거리
    float accelDistance = (_maxSpeed * _maxSpeed) / (2.0 * _acceleration);
    
    // 감속 구간에서 최고속도에서 정지하는데 필요한 거리
    float decelDistance = accelDistance;  // 대칭이므로 같음
    
    // 정속 구간이 있는지 확인
    if (absDistance >= (accelDistance + decelDistance)) {
        // 정속 구간이 있음: 가속 → 정속 → 감속 (사다리꼴 프로파일)
        _target1 = _currentPos + (distanceTo > 0 ? accelDistance : -accelDistance);
        _target2 = _targetPos - (distanceTo > 0 ? decelDistance : -decelDistance);
        
        // 정속 구간에서의 인터럽트 간격 (최고속도 기준)
        // 100마이크로초 타이머 기준으로 계산
        // _maxSpeed = 1000이면 1000스텝/초 = 1스텝/1ms = 1스텝/10*100us
        _constantSpeedIntervalCount = (unsigned long)(1000000.0 / _maxSpeed / 100.0);
        if (_constantSpeedIntervalCount < 1) _constantSpeedIntervalCount = 1;  // 최소 1
        
        // 디버그 출력
        Serial.print("사다리꼴 프로파일: 거리="); Serial.print(absDistance);
        Serial.print(", 가속거리="); Serial.print(accelDistance);
        Serial.print(", 타겟1="); Serial.print(_target1);
        Serial.print(", 타겟2="); Serial.print(_target2);
        Serial.print(", 정속간격="); Serial.println(_constantSpeedIntervalCount);
    } else {
        // 정속 구간이 없음: 가속 → 감속 (삼각형 프로파일)
        float halfDistance = absDistance / 2.0;
        _target1 = _currentPos + (distanceTo > 0 ? halfDistance : -halfDistance);
        _target2 = _target1;  // 정속 구간 없음
        
        // 최고속도가 제한됨
        float limitedMaxSpeed = sqrt(_acceleration * absDistance);
        // 100마이크로초 타이머 기준으로 계산
        _constantSpeedIntervalCount = (unsigned long)(1000000.0 / limitedMaxSpeed / 100.0);
        if (_constantSpeedIntervalCount < 1) _constantSpeedIntervalCount = 1;  // 최소 1
        
        // 디버그 출력
        Serial.print("삼각형 프로파일: 거리="); Serial.print(absDistance);
        Serial.print(", 타겟1="); Serial.print(_target1);
        Serial.print(", 제한속도="); Serial.print(limitedMaxSpeed);
        Serial.print(", 정속간격="); Serial.println(_constantSpeedIntervalCount);
    }
    
    // 가속/감속 구간 스텝 수 계산
    long accelSteps = abs(_target1 - _currentPos);
    long decelSteps = abs(_targetPos - _target2);
    
    if (accelSteps > 0) {
        // 초기 펄스간격 (가속 시작점)
        // 가속 구간에서의 최종 속도 계산
        float finalSpeedInAccel = sqrt(2.0 * _acceleration * accelSteps);
        
        // 초기 속도 설정 (적절한 시작 속도)
        float initialSpeed = 10.0;  // 10 스텝/초 (적절한 시작 속도)
        
        // 최소 초기 속도 보장 (너무 느린 시작 방지)
        float minInitialSpeed = 5.0;  // 5 스텝/초
        if (initialSpeed < minInitialSpeed) initialSpeed = minInitialSpeed;
        
        float initialInterval = 1000000.0 / initialSpeed / 100.0;  // 100마이크로초 타이머 기준
        
        // 초기 펄스 인터벌 설정
        _lastPulseInterval = (unsigned long)initialInterval;
        if (_lastPulseInterval < 1) _lastPulseInterval = 1;
        // 최대 제한: 정속 간격의 10배 이내
        if (_lastPulseInterval > _constantSpeedIntervalCount * 10) {
            _lastPulseInterval = _constantSpeedIntervalCount * 10;
        }
        
        // 초기 간격 저장 (감속 최대 제한용)
        _initialInterval = _lastPulseInterval;
        
        // 펄스 간격 증가치 계산 (스케일링 없이 직접 계산)
        // 가속: 초기 간격 → 정속 간격 (간격 감소)
        // 대칭이므로 가속/감속 같은 증가치 사용
        float totalChange = (float)_lastPulseInterval - (float)_constantSpeedIntervalCount;
        _pulseIntervalIncrement = totalChange / (float)accelSteps;
        
        // 디버그 출력
        Serial.print("선형 가속/감속: 가속스텝="); Serial.print(accelSteps);
        Serial.print(", 감속스텝="); Serial.print(decelSteps);
        Serial.print(", 초기속도="); Serial.print(initialSpeed, 1);
        Serial.print(", 최종속도="); Serial.print(finalSpeedInAccel, 1);
        Serial.print(", 초기간격="); Serial.print(_lastPulseInterval);
        Serial.print(", 최고속도간격="); Serial.print(_constantSpeedIntervalCount);
        Serial.print(", 증가치="); Serial.print(_pulseIntervalIncrement, 3);
        Serial.print(", target1="); Serial.print(_target1);
        Serial.print(", target2="); Serial.println(_target2);
    } else {
        // 가속구간이 없는 경우
        _pulseIntervalIncrement = 0;
        _lastPulseInterval = _constantSpeedIntervalCount;
    }
    
    // 인터벌 카운터 초기화
    _intervalCounter = 0;
}

void TimerStepper::setOutputPins(uint8_t direction) {
    #if defined(ESP32)
        gpio_set_level((gpio_num_t)_dirPin, direction ? 1 : 0);
    #else
        digitalWrite(_dirPin, direction ? HIGH : LOW);
    #endif
}

// 다음 펄스 간격 계산 (펄스 생성 직후에만 호출)
#if defined(ESP32)
void IRAM_ATTR TimerStepper::calculateNextInterval() {
#else
void TimerStepper::calculateNextInterval() {
#endif
    // 현재 구간 판단 (방향 고려)
    bool isAccelerating, isConstantSpeed, isDecelerating;
    
    if (_direction) {
        // 정방향 이동 (currentPos -> targetPos, currentPos 증가)
        isAccelerating = _currentPos < _target1;
        isConstantSpeed = (_target1 != _target2) && (_currentPos >= _target1 && _currentPos < _target2);
        isDecelerating = _currentPos >= _target2;
    } else {
        // 역방향 이동 (currentPos -> targetPos, currentPos 감소)
        isAccelerating = _currentPos > _target1;
        isConstantSpeed = (_target1 != _target2) && (_currentPos <= _target1 && _currentPos > _target2);
        isDecelerating = _currentPos <= _target2;
    }
    
    // 구간에 따른 펄스 간격 계산 (소수점 누적 방식 사용)
    if (isAccelerating) {
        // 가속: 간격 감소 (속도 증가)
        // 소수점 누적 방식으로 정확한 간격 계산
        _fractionalAccumulator += _pulseIntervalIncrement;
        int intervalChange = (int)_fractionalAccumulator;  // 정수 부분 추출
        _fractionalAccumulator -= (float)intervalChange;  // 정수 부분을 누적치에서 제거
        
        _lastPulseInterval = _lastPulseInterval - intervalChange;
        if (_lastPulseInterval < 1) _lastPulseInterval = 1;
        // 최소값 제한: 정속 간격보다 작아지지 않도록
        if (_lastPulseInterval < _constantSpeedIntervalCount) {
            _lastPulseInterval = _constantSpeedIntervalCount;
            _fractionalAccumulator = 0.0;  // 정속 구간 진입 시 누적치 리셋
        }
    } else if (isConstantSpeed) {
        // 정속: 일정한 간격 유지
        _lastPulseInterval = _constantSpeedIntervalCount;
        _fractionalAccumulator = 0.0;  // 정속 구간에서는 누적치 리셋
    } else if (isDecelerating) {
        // 감속 구간 진입 감지: 이전 구간이 가속 또는 정속이었다면 처음 진입
        bool justEnteredDecel = (_wasAccelerating || _wasConstantSpeed);
        
        if (justEnteredDecel) {
            // 감속 구간 진입 시: _lastPulseInterval을 정속 간격으로 초기화
            _lastPulseInterval = _constantSpeedIntervalCount;
            _fractionalAccumulator = 0.0;  // 감속 구간 진입 시 누적치 리셋
        } else {
            // 감속: 간격 증가 (속도 감소)
            // 소수점 누적 방식으로 정확한 간격 계산
            _fractionalAccumulator += _pulseIntervalIncrement;
            int intervalChange = (int)_fractionalAccumulator;  // 정수 부분 추출
            _fractionalAccumulator -= (float)intervalChange;  // 정수 부분을 누적치에서 제거
            
            _lastPulseInterval = _lastPulseInterval + intervalChange;
            if (_lastPulseInterval < 1) _lastPulseInterval = 1;
            // 최대 제한: 초기 간격까지 (대칭)
            if (_lastPulseInterval > _initialInterval) _lastPulseInterval = _initialInterval;
        }
    } else {
        _lastPulseInterval = 5;
    }
    
    // 구간 전환 상태 업데이트 (다음 사이클에서 구간 전환 감지용)
    _wasAccelerating = isAccelerating;
    _wasConstantSpeed = isConstantSpeed;
}




