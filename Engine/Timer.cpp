#include "pch.h"
#include "Timer.h"

namespace JEngine {

Timer::Timer() 
    : secondsPerCount_(0.0), deltaTime_(0.0), baseTime_(0), 
    pausedTime_(0), stopTime_(0), prevTime_(0), 
    curTime_(0), stopped_(false), frameCount_(0), timeElapsed_(0.f), fps_(0.f) {

    LARGE_INTEGER countsPerSec;
    // Get Frequency of Performance Counter
    ::QueryPerformanceFrequency(&countsPerSec);
    
    // Tick per duration (1 / Frequency)
    secondsPerCount_ = 1.0 / static_cast<double>(countsPerSec.QuadPart);

    LogInfo("Timer Initialized.");
}

float Timer::TotalTime() const {
    // 타이머가 정지된 경우:
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------------*------> time
    //  baseTime        stopTime         startTime      stopTime    (현재)
    //
    // 총 시간 = (정지 시점 - 시작 시점) - 누적 일시정지 시간
    if (stopped_) {
        return static_cast<float>(((stopTime_ - baseTime_) - pausedTime_) * secondsPerCount_);
    } 
    // 타이머가 실행 중인 경우:
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------> time
    //  baseTime        stopTime         startTime      curTime
    //
    // 총 시간 = (현재 시점 - 시작 시점) - 누적 일시정지 시간
    else {
        return static_cast<float>(((curTime_ - baseTime_) - pausedTime_) * secondsPerCount_);
    }
} 

float Timer::DeltaTime() const {
    return static_cast<float>(deltaTime_);
}

float Timer::FrameRate() const {
    return fps_;
}

void Timer::Reset() {
    LARGE_INTEGER curTime;
    // 현재 성능 카운터 값을 가져옵니다
    ::QueryPerformanceCounter(&curTime);

    // 모든 시간 값을 현재 시점으로 초기화
    baseTime_ = curTime.QuadPart;
    prevTime_ = curTime.QuadPart;
    stopTime_ = 0;
    pausedTime_ = 0;
    stopped_ = false;
    frameCount_ = 0;
    timeElapsed_ = 0.0f;
    fps_ = 0.0f;

    LogInfo("Timer reset.");
}

void Timer::Start() {
    LARGE_INTEGER startTime;
    ::QueryPerformanceCounter(&startTime);
    
    // 일시정지 상태에서 재개 가능
    // Stop()과 Start() 사이의 시간을 누적합니다
    //
    //                     |<-------d------->|
    // ----*---------------*-----------------*------------> time
    //  baseTime        stopTime          startTime
    //
    // d = 일시정지된 시간 간격
    if (stopped_) {
        // 이번 일시정지 구간의 시간을 누적 일시정지 시간에 추가
        pausedTime_ += (startTime.QuadPart - stopTime_);
        
        // 이전 프레임 시간을 재개 시점으로 설정 (델타타임 계산용)
        prevTime_ = startTime.QuadPart;
        
        // 정지 시점 초기화 및 플래그 해제
        stopTime_ = 0;
        stopped_ = false;

        LogInfo("Timer resumed.");
    } else
        LogInfo("Timer started.");
}

void Timer::Stop() {
    // 이미 정지된 상태가 아닐 때만 정지
    if (!stopped_) {
        LARGE_INTEGER stopTime;
        ::QueryPerformanceCounter(&stopTime);
        stopTime_ = stopTime.QuadPart;
        
        stopped_ = true;
        LogInfo("Timer stopped.");
    }
}

void Timer::Tick() {
    if (stopped_) {
        deltaTime_ = 0.0;
        return;
    }

    LARGE_INTEGER curTime;
    ::QueryPerformanceCounter(&curTime);
    curTime_ = curTime.QuadPart;

    // 이전 프레임과 현재 프레임 사이의 시간 차이를 계산 (초 단위)
    deltaTime_ = (curTime_ - prevTime_) * secondsPerCount_;
    
    // 다음 프레임을 위해 현재 시간을 저장
    prevTime_ = curTime_;

    // 예외 상황 처리:
    // GPU 전원 절약 모드로 인한 DeltaTime 음수 방지
    if (deltaTime_ < 0.0) {
        deltaTime_ = 0.0;
    }

    // FPS 계산
    ++frameCount_;
    timeElapsed_ += static_cast<float>(deltaTime_);

    if (timeElapsed_ >= 1.0f) {
        fps_ = static_cast<float>(frameCount_) / timeElapsed_;
        frameCount_ = 0;
        timeElapsed_ = 0.f;
    }
}

} // namespace JEngine