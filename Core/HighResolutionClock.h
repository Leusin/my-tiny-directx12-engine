#pragma once

#include <chrono>

class HighResolutionClock
{
public:
    HighResolutionClock();

    // ���ػ� �ð踦 tick�մϴ�.
    // ��Ÿ �ð��� ó�� �б� ���� �ð踦 tick�մϴ�.
    // �����Ӵ� �� ���� �ð踦 tick�մϴ�.
    // Get* �Լ��� ����Ͽ� tick ���� ��� �ð��� ��ȯ�մϴ�.
    void Tick();

    // �ð踦 �缳�� �մϴ�.
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
    // �ʱ�ȭ ����
    std::chrono::high_resolution_clock::time_point m_t0;
    // ������ tick�� ����� ���� �帥 �ð�
    std::chrono::high_resolution_clock::duration m_deltaTime;
    std::chrono::high_resolution_clock::duration m_totalTime;
};