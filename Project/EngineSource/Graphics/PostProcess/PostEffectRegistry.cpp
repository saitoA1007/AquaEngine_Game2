#include "pch.h"
#include "PostEffectRegistry.h"
#include "LogManager.h"

using namespace GameEngine;

std::vector<std::string> PostEffectRegistry::GetTypeNames() const {
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (const auto& [type, factory] : factories_) { names.push_back(type); }
    std::ranges::sort(names);
    return names;
}

std::unique_ptr<IPostEffect> PostEffectRegistry::CreateInstance(const std::string& typeName) const {
    auto it = factories_.find(typeName);
    return it != factories_.end() ? it->second() : nullptr;
}

const DrawPsoData* PostEffectRegistry::FindPso(const std::string& psoName) const {
    auto it = psoList_.find(psoName);
    return it != psoList_.end() ? &it->second : nullptr;
}

IPostEffect* PostEffectRegistry::Create(const std::string& typeName, const std::string& baseName, std::string& outName) {
    auto fx = CreateInstance(typeName);
    if (!fx) {
        LogManager::GetInstance().Log("PostEffect: 未登録の種類です: " + typeName + "\n");
        return nullptr;
    }
    outName = MakeUniqueName(baseName.empty() ? typeName : baseName, effects_);
    IPostEffect* raw = fx.get();
    effects_[outName] = std::move(fx);
    return raw;
}

IPostEffect* PostEffectRegistry::Find(const std::string& name) const {
    auto it = effects_.find(name);
    return it != effects_.end() ? it->second.get() : nullptr;
}

bool PostEffectRegistry::Owns(const std::string& name, const IPostEffect* fx) const {
    auto it = effects_.find(name);
    return it != effects_.end() && it->second.get() == fx;
}

void PostEffectRegistry::Destroy(const std::string& name) {
    auto it = effects_.find(name);
    if (it == effects_.end()) { return; }
    retired_.push_back({ std::move(it->second), kRetireFrames });
    effects_.erase(it);
}

void PostEffectRegistry::RetireAll() {
    for (auto& [key, fx] : effects_) { retired_.push_back({ std::move(fx), kRetireFrames }); }
    effects_.clear();
}

void PostEffectRegistry::Replace(std::unordered_map<std::string, std::unique_ptr<IPostEffect>>&& newEffects) {
    RetireAll();
    effects_ = std::move(newEffects);
}

void PostEffectRegistry::Tick() {
    for (auto& r : retired_) { --r.framesLeft; }
    std::erase_if(retired_, [](const Retired& r) { return r.framesLeft < 0; });
}