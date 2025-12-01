#pragma once

namespace JEngine {

class Timer
{
  public:
    Timer();

    float TotalTime() const; // in seconds
    float DeltaTime() const; // in seconds
    float FrameRate() const;

    // Call before message loop
    void Reset(); 
    // Call when unpaused
    void Start(); 
    // Call when paused
    void Stop();
    // Call every frame
    void Tick();  

  private:
    double secondsPerCount_; // Tick Per Duration
    double deltaTime_; 

    __int64 baseTime_; // Time when Reset() is called (Program start time)
    __int64 pausedTime_; // Accumulated paused time
    __int64 stopTime_;   // Time when Stop() is called
    __int64 prevTime_;   // Time at previous Tick()
    __int64 curTime_;

    bool stopped_;

    int frameCount_;
    float timeElapsed_;
    float fps_;
};

} // namespace JEngine