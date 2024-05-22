#pragma once

#include <chrono>

class HighResolutionClock
{
public:
    HighResolutionClock();

    // 고해상도 시계를 tick합니다.
    // 델타 시간을 처음 읽기 전에 시계를 tick합니다.
    // 프레임당 한 번만 시계를 tick합니다.
    // Get* 함수를 사용하여 tick 간의 경과 시간을 반환합니다.
    void Tick();

    // 시계를 재설정 합니다.
    void Reset();

    double GetDeltaNanoseconds() const;
    double GetDeltaMicroseconds() const;
    double GetDeltaMilliseconds() const;
    double GetDeltaSeconds() const;

    double GetTotalNanoseconds() const;
    double GetTotalMicroseconds() const;
    double GetTotalMilliSeconds() const;
    double GetTotalSeconds() const;

private:
    // 초기화 시점
    std::chrono::high_resolution_clock::time_point m_t0;
    // 마지막 tick이 실행된 이후 흐른 시간
    std::chrono::high_resolution_clock::duration m_deltaTime;
    std::chrono::high_resolution_clock::duration m_totalTime;
};