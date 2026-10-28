#include "pch.h"
#include "RenderTextureManager.h"

using namespace GameEngine;

void RenderTextureManager::Initialize(RtvManager* rtvManager,DsvManager* dsvManager, ID3D12Device5* device) {
    device_ = device;
    RenderTexture::StaticInitialize(rtvManager, dsvManager);
}

void RenderTextureManager::Create(const std::string& name, uint32_t width, uint32_t height, RenderTextureMode mode, DXGI_FORMAT colorFormat, Vector4 clearColor) {
    auto renderTexture = std::make_unique<RenderTexture>();
    renderTexture->Create(width, height, mode, colorFormat, clearColor);

    // PIX上でリソース名が読めるように名前を設定する
    renderTexture->SetDebugNames(name);

    // 同名が既存ならデストラクタ経由で古いリソースが解放される
    renderTextures_[name] = std::move(renderTexture);
}

[[nodiscard]]
RenderTexture* RenderTextureManager::GetRenderTexture(const std::string& name) const {
    auto it = renderTextures_.find(name);
    if (it != renderTextures_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void RenderTextureManager::Release(const std::string& name) {
    renderTextures_.erase(name);
}

void RenderTextureManager::ReleaseAll() {
    renderTextures_.clear();
}