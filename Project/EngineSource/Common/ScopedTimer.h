#pragma once
#include "LogManager.h"
#include <chrono>

namespace GameEngine {

    /// <summary>
    /// 時間を計測してログを表示する
    /// </summary>
    class ScopedTimer {
    public:
        explicit ScopedTimer(std::string name)
            : name_(std::move(name)), start_(Clock::now()) {}

        ~ScopedTimer() {
            const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
            LogManager::GetInstance().Log("[LoadTime] " + name_ + " : " + std::to_string(ms) + " ms\n");
        }

        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;

    private:
        using Clock = std::chrono::steady_clock;
        std::string name_;
        Clock::time_point start_;
    };
}