#include "pch.h"
#include "GameObjectManager.h"
#include <algorithm>
using namespace GameEngine;

GameObjectManager::GameObjectManager() {
    // あらかじめメモリを確保
    objects_.reserve(1000);
}

void GameObjectManager::InitializeAll() {
    for (auto& obj : objects_) {
        obj->Initialize();
    }
}

void GameObjectManager::UpdateAll() {

    // オブジェクトの更新処理をおこなう
    isIterating_ = true;
    for (auto& obj : objects_) {
        if (obj->IsActive() && !obj->IsDead()) {
            obj->Update();
        }
    }
    isIterating_ = false;

    // 更新中に追加されたオブジェクトを反映
    FlushPendingObjects();

    // 削除フラグが立ったオブジェクトを削除
    std::erase_if(objects_, [](const std::unique_ptr<IGameObject>& obj) {
        return obj->IsDead();
        });

    // ソートをおこなう
    if (needsSort_) {
        std::stable_sort(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<IGameObject>& a, const std::unique_ptr<IGameObject>& b) {
                return a->GetUpdateOrder() < b->GetUpdateOrder();
            });
        needsSort_ = false;
    }
}

void GameObjectManager::DrawAll() {
    for (auto& obj : objects_) {
        if (obj->IsActive() && !obj->IsDead()) {
            obj->Draw();
        }
    }
}

void GameObjectManager::DebugUpdateAll() {
    for (auto& obj : objects_) {
        if (!obj->IsDead()) {
            obj->DebugUpdate();
        }
    }
}

void GameObjectManager::FlushPendingObjects() {
    if (pendingObjects_.empty()) { return; }
    for (auto& obj : pendingObjects_) {
        objects_.push_back(std::move(obj));
    }
    pendingObjects_.clear();
    needsSort_ = true;
}