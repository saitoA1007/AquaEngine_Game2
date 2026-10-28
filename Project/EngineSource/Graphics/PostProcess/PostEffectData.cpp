#include "pch.h"
#include "PostEffectData.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
using namespace GameEngine;

ColorGrading::ColorGrading() {
    // 作成
    buffer_.Create();
    auto* data = buffer_.GetData();
    
    data->enableGrayscale = 0;
    data->enableSepia = 0;
    data->enableRandom = 1;
    data->enableVignetting = 1;

    data->vignettingIntensity = 16.0f;
    data->vignettingTime = 0.15f;

    data->randomIntensity = 0.02f;
    data->randomTime = 1.0f;

    isActive_ = true;
}

void ColorGrading::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void ColorGrading::DrawParamUI() {
#ifdef USE_IMGUI
    auto* d = buffer_.GetData();
    bool b;
    b = d->enableGrayscale != 0;  
    if (ImGui::Checkbox("Grayscale", &b)) { d->enableGrayscale = b; }
    b = d->enableSepia != 0;
    if (ImGui::Checkbox("Sepia", &b)) { d->enableSepia = b; }
    b = d->enableRandom != 0;
    if (ImGui::Checkbox("Random", &b)) { d->enableRandom = b; }
    ImGui::DragFloat("Random Intensity", &d->randomIntensity, 0.001f, 0.0f, 1.0f);
    b = d->enableVignetting != 0; 
    if (ImGui::Checkbox("Vignetting", &b)) { d->enableVignetting = b; }
    ImGui::DragFloat("Vignette Intensity", &d->vignettingIntensity, 0.1f, 0.0f, 64.0f);
    ImGui::DragFloat("Vignette Range", &d->vignettingTime, 0.01f, 0.0f, 2.0f);
#endif 
}

void ColorGrading::SaveParams(nlohmann::json& j) {
    auto* d = buffer_.GetData();
    j["grayscale"] = d->enableGrayscale != 0;
    j["sepia"] = d->enableSepia != 0;
    j["random"] = d->enableRandom != 0;
    j["vignetting"] = d->enableVignetting != 0;
    j["vignettingIntensity"] = d->vignettingIntensity;
    j["vignettingTime"] = d->vignettingTime;
    j["randomIntensity"] = d->randomIntensity;
    j["randomTime"] = d->randomTime;
}

void ColorGrading::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->enableGrayscale = j.value("grayscale", d->enableGrayscale != 0) ? 1 : 0;
    d->enableSepia = j.value("sepia", d->enableSepia != 0) ? 1 : 0;
    d->enableRandom = j.value("random", d->enableRandom != 0) ? 1 : 0;
    d->enableVignetting = j.value("vignetting", d->enableVignetting != 0) ? 1 : 0;
    d->vignettingIntensity = j.value("vignettingIntensity", d->vignettingIntensity);
    d->vignettingTime = j.value("vignettingTime", d->vignettingTime);
    d->randomIntensity = j.value("randomIntensity", d->randomIntensity);
    d->randomTime = j.value("randomTime", d->randomTime);
}


RadialBlur::RadialBlur() {
    // 作成
    buffer_.Create();
    buffer_.GetData()->centerPos = { 0.5f,0.5f };
    buffer_.GetData()->numSamles = 3;
    buffer_.GetData()->blurWidth = 0.01f;
}

void RadialBlur::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

HighLumMask::HighLumMask() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->highLumMask = 0.8f;

    isActive_ = true;
}

void HighLumMask::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void HighLumMask::DrawParamUI() {
#ifdef USE_IMGUI
    ImGui::SliderFloat("Threshold", &buffer_.GetData()->highLumMask, 0.0f, 1.0f);
#endif 
}

void HighLumMask::SaveParams(nlohmann::json& j) { j["highLumMask"] = buffer_.GetData()->highLumMask; }
void HighLumMask::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->highLumMask = j.value("highLumMask", d->highLumMask);
}

GaussVertical::GaussVertical() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.0f;

    isActive_ = true;
}

void GaussVertical::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void GaussVertical::DrawParamUI() {
#ifdef USE_IMGUI
    ImGui::DragFloat("SD", &buffer_.GetData()->sd, 0.05f, 0.1f, 20.0f);
#endif 
}

void GaussVertical::SaveParams(nlohmann::json& j) { j["sd"] = buffer_.GetData()->sd; }
void GaussVertical::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->sd = j.value("sd", d->sd);
}

GaussHorizontal::GaussHorizontal() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.0f;

    isActive_ = true;
}

void GaussHorizontal::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void GaussHorizontal::DrawParamUI() {
#ifdef USE_IMGUI
    ImGui::DragFloat("SD", &buffer_.GetData()->sd, 0.05f, 0.1f, 20.0f);
#endif 
}

void GaussHorizontal::SaveParams(nlohmann::json& j) { j["sd"] = buffer_.GetData()->sd; }
void GaussHorizontal::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->sd = j.value("sd", d->sd);
}


Bloom::Bloom() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->intensity = 1.0f;

    isActive_ = true;
}

void Bloom::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void Bloom::DrawParamUI() {
#ifdef USE_IMGUI
    ImGui::DragFloat("Intensity", &buffer_.GetData()->intensity, 0.01f, 0.0f, 10.0f);
#endif   
}

void Bloom::SaveParams(nlohmann::json& j) { j["intensity"] = buffer_.GetData()->intensity; }
void Bloom::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->intensity = j.value("intensity", d->intensity);
}

Dissolve::Dissolve() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->threshold = 0.0f;

    isActive_ = true;
}

void Dissolve::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void Dissolve::DrawParamUI() {
#ifdef USE_IMGUI
    ImGui::SliderFloat("Threshold", &buffer_.GetData()->threshold, 0.0f, 1.0f);
#endif 
}

void Dissolve::SaveParams(nlohmann::json& j) { j["threshold"] = buffer_.GetData()->threshold; }
void Dissolve::LoadParams(const nlohmann::json& j) {
    if (!j.is_object()) { return; }
    auto* d = buffer_.GetData();
    d->threshold = j.value("threshold", d->threshold);
}

OutLine::OutLine() {
    // 作成
    buffer_.Create();
    // 標準偏差
    buffer_.GetData()->sd = 2.5f;
}

void OutLine::Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
    commandList->SetGraphicsRootDescriptorTable(0, srvManager->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    commandList->SetGraphicsRootConstantBufferView(1, buffer_.GetGpuVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
}