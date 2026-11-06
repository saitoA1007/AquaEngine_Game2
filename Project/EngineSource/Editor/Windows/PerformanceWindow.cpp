#include "pch.h"
#include"PerformanceWindow.h"
#include"FPSCounter.h"

using namespace GameEngine;

void PerformanceWindow::Draw() {
    ImGui::Begin("Performance", &isActive);
	ImGui::Text("DeltaTime : %f", FpsCounter::GetDeltaTime());
	ImGui::Text("FpsCount : %d", FpsCounter::GetFrameCount());
	ImGui::Text("MaxFpsCount : %.0f", FpsCounter::GetMaxFrameCount());
    ImGui::End();
}