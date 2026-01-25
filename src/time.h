#pragma once
#include <chrono>

class Time
{
public:
    void start()
    {
        lastTime = Clock::now();
        startTime = Clock::now();
        paused = false;
        delta = 0.0f;
    }
    void pause()
    {
        if(!paused)
        {
            pausedTime = Clock::now();
            paused = true;
        }
    }
    void resume()
    {
        if(paused)
        {
            auto now = Clock::now();
            lastTime += now - pausedTime;
            paused = false;
        }
    }
    float elapsedTime()
    {
        if(paused) 
            return std::chrono::duration<float>(pausedTime - startTime).count();
        else
            return std::chrono::duration<float>(Clock::now() - startTime).count();
    }

private:
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point lastTime;
    Clock::time_point startTime;
    Clock::time_point pausedTime;

    float delta = 0.0f;
    bool paused = false;
};