#include "pch.h"
#include "CellFractureTest.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include <cassert>
#include <cmath>

using namespace GameEngine;

CellFractureTest::CellFractureTest(Model* model, InputCommand* inputCommand)
    : model_(model), inputCommand_(inputCommand) {
    assert(model_ != nullptr);
    assert(inputCommand_ != nullptr);
    assert(model_->GetFractureChunks().size() == 1);
}

CellFractureTest::~CellFractureTest() {
    // 専用モデルに設定した材質が、解放済みの個体を参照し続けないように戻す。
    const std::string& groupName = model_->GetFractureChunks().begin()->first;
    const std::vector<FractureChunkEntry>& chunks = model_->GetFractureChunks().begin()->second;
    Material* material = model_->GetMaterial(chunks.front().materialName);
    model_->GetFractureBuffers().at(groupName)->SetBufferMaterial(
        material->GetMaterialSrvIndex(), static_cast<uint32_t>(RayInstanceMask::kRayMaskOpaque));
}

void CellFractureTest::Initialize() {
    const std::string& groupName = model_->GetFractureChunks().begin()->first;
    const std::vector<FractureChunkEntry>& chunks = model_->GetFractureChunks().begin()->second;
    assert(!chunks.empty());

    // 全破片を登録し、アンカー判定を使わず元の配置で表示する。
    std::vector<uint32_t> chunkIds;
    chunkIds.reserve(chunks.size());
    for (const FractureChunkEntry& chunk : chunks) {
        chunkIds.push_back(chunk.info.chunkId);
    }
    PackedGeometryBuffer& buffer = *model_->GetFractureBuffers().at(groupName);
    fractureInstance_.Initialize(chunkIds, buffer);

    // 既存の破片描画は氷向けなので、対応する材質を専用モデルへ設定する。
    buffer.SetBufferMaterial(iceMaterial_.GetMaterialSrvIndex(),
        static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));

    // 再初期化でも速度と破壊済み状態を残さない。
    for (FractureChunkState& state : fractureInstance_.GetTransformDatas()) {
        state.velocity = { 0.0f, 0.0f, 0.0f };
    }
    isBroken_ = false;
    worldTransform_.UpdateTransformMatrix();
    fractureInstance_.Update();
}

void CellFractureTest::Update() {
    // 入力を先に処理し、同じフレームから全破片を動かす。
    if (inputCommand_->IsCommandActive("BreakCellFractureCube")) {
        Break();
    }

    if (!isBroken_) {
        return;
    }

    // 重力や追加の分割は行わず、それぞれの初速度で直線的に移動する。
    for (FractureChunkState& state : fractureInstance_.GetTransformDatas()) {
        state.transform.translate += state.velocity * FpsCounter::gameDeltaTime;
    }
    fractureInstance_.Update();
}

void CellFractureTest::Draw() {
    renderQueue_->SubmitRaytracingFracture(model_, fractureInstance_, worldTransform_);
}

void CellFractureTest::Break() {
    if (isBroken_) {
        return;
    }
    assert(minLaunchSpeed_ > 0.0f && maxLaunchSpeed_ >= minLaunchSpeed_);

    // 球面上から方向を選び、軸方向へ偏らないランダムな初速度を作る。
    constexpr float kTwoPi = 6.28318530718f;
    for (FractureChunkState& state : fractureInstance_.GetTransformDatas()) {
        float vertical = RandomGenerator::Get(-1.0f, 1.0f);
        float angle = RandomGenerator::Get(0.0f, kTwoPi);
        float horizontal = std::sqrt(1.0f - vertical * vertical);
        Vector3 direction = {
            horizontal * std::cos(angle), vertical, horizontal * std::sin(angle)
        };
        float speed = RandomGenerator::Get(minLaunchSpeed_, maxLaunchSpeed_);
        state.velocity = direction * speed;
    }
    isBroken_ = true;
}
