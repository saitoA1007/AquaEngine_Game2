#pragma once

class Timer {
public:

	void Start();

	void Stop();

	void Reset();

	void Update();

public:

	float GetTimer()const { return timer_; }

private:
	float timer_ = 0.0f;
	bool isActive_ = false;
};