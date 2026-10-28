#include "pch.h"
#include"PerformanceWindow.h"
#include"FPSCounter.h"

using namespace GameEngine;

void PerformanceWindow::Draw() {
    ImGui::Begin("Performance", &isActive);
	ImGui::Text("DeltaTime : %f", FpsCounter::deltaTime);
	ImGui::Text("FpsCount : %d", FpsCounter::frameCount_);
	ImGui::Text("MaxFpsCount : %.0f", FpsCounter::maxFrameCount);
    ImGui::End();
}