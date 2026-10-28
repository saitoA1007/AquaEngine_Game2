#include "pch.h"
#include "Timer.h"
#include "FPSCounter.h"
using namespace GameEngine;

void Timer::Start() {
	isActive_ = true;
}

void Timer::Stop() {
	isActive_ = false;
}

void Timer::Reset() {
	timer_ = 0.0f;
}

void Timer::Update() {
	if (!isActive_) { return; }
	timer_ += FpsCounter::deltaTime;
}