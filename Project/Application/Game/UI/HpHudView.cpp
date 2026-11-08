#include "pch.h"
#include "HpHudView.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
using namespace GameEngine;

HpHudView::HpHudView(const Camera* camera, const HpHudModels& models, uint32_t whiteTextureHandle)
    : camera_(camera), models_(models), whiteTextureHandle_(whiteTextureHandle) {
    // モデルが1つでも足りなければ、表示が欠けるので例外にする
    if (camera_ == nullptr || models_.frame == nullptr || models_.core == nullptr || models_.shard == nullptr) {
        throw std::runtime_error("HpHudView: camera or model is missing");
    }

    RegisterDebugParameters();

    // 白い材質。色は、インスタンスごとの色で決める
    material_.Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, false);

    // インスタンスの領域を、アイコンの数の上限だけ確保する
    Transform identity = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    frameTransforms_.Initialize(kMaxIcons, identity);
    coreTransforms_.Initialize(kMaxIcons, identity);

    SetUpShards();
}

void HpHudView::SetUpShards() {
    // 破壊グループが1つの破片モデルだけを扱う（ShatterEffectManagerと同じ条件）
    const std::unordered_map<std::string, std::vector<FractureChunkEntry>>& groups = models_.shard->GetFractureChunks();
    if (groups.size() != 1 || groups.begin()->second.empty()) {
        throw std::runtime_error("HpHudView: shard model must have exactly one fracture group");
    }
    const std::string& groupName = groups.begin()->first;
    const std::vector<FractureChunkEntry>& chunks = groups.begin()->second;

    // 欠片の番号・割れる前の中心・大きさを、読み込み時に求めた値から取る
    std::vector<uint32_t> chunkIds;
    for (const FractureChunkEntry& chunk : chunks) {
        chunkIds.push_back(chunk.info.chunkId);
        Vector3 halfSize = (chunk.info.aabb.max - chunk.info.aabb.min) * 0.5f;
        shardShapes_.push_back({ chunk.info.centroid, halfSize.Length() });
    }

    // 破片の描画はモデル自身のマテリアルを使う。UIは平らな色で見せたいので、光を切る
    // （Fracture.PS.hlslがenableLightingを見るようになるまでは、陰影が付いたまま描かれる。docs/ai/FeatureGaps.md）
    shardMaterial_ = models_.shard->GetMaterial(chunks.front().materialName);
    if (shardMaterial_ == nullptr) {
        throw std::runtime_error("HpHudView: shard model material is missing");
    }
    shardMaterial_->SetEnableLighting(false);

    // アイコンごとに、破片の領域を一度だけ確保する。回転は各欠片の中心を軸にする
    const PackedGeometryBuffer& buffer = *models_.shard->GetFractureBuffers().at(groupName);
    for (FractureInstance& instance : shardInstances_) {
        instance.Initialize(chunkIds, buffer);
        for (size_t c = 0; c < shardShapes_.size(); ++c) {
            instance.SetPivot(static_cast<uint32_t>(c), shardShapes_[c].restCenter, 0.0f);
        }
    }
}

void HpHudView::RegisterDebugParameters() {
    const std::string kLayoutGroup = "Layout";
    const std::string kColorGroup = "Color";
    const std::string kShardGroup = "Shard";

    debugParam_ = std::make_unique<DebugParameter>(kDebugName);
    debugParam_->Register("origin", origin_, 0, kLayoutGroup);
    debugParam_->Register("iconSize", iconSize_, 1, kLayoutGroup);
    debugParam_->Register("spacing", spacing_, 2, kLayoutGroup);
    debugParam_->Register("distance", distance_, 3, kLayoutGroup);
    debugParam_->Register("spinSpeed", spinSpeed_, 4, kLayoutGroup);

    debugParam_->Register("coreColor", coreColor_, 0, kColorGroup);
    debugParam_->Register("frameColor", frameColor_, 1, kColorGroup);
    debugParam_->Register("lostCoreColor", lostCoreColor_, 2, kColorGroup);
    debugParam_->Register("lostFrameColor", lostFrameColor_, 3, kColorGroup);
    debugParam_->Register("shatterTime", shatterTime_, 4, kColorGroup);

    debugParam_->Register("gravity", pileSettings_.gravity, 0, kShardGroup);
    debugParam_->Register("burstSpeed", pileSettings_.burstSpeed, 1, kShardGroup);
    debugParam_->Register("burstUp", pileSettings_.burstUp, 2, kShardGroup);
    debugParam_->Register("maxAngularSpeed", pileSettings_.maxAngularSpeed, 3, kShardGroup);
    debugParam_->Register("angularDamping", pileSettings_.angularDamping, 4, kShardGroup);
    debugParam_->Register("restitution", pileSettings_.restitution, 5, kShardGroup);
    debugParam_->Register("friction", pileSettings_.friction, 6, kShardGroup);
    debugParam_->Register("radiusScale", pileSettings_.radiusScale, 7, kShardGroup);
    debugParam_->Register("depthSettle", pileSettings_.depthSettle, 8, kShardGroup);
    debugParam_->Register("sleepSpeed", pileSettings_.sleepSpeed, 9, kShardGroup);
    debugParam_->Apply();
}

void HpHudView::ResizeIcons(int32_t count) {
    // 新しいアイコンは核が残った状態で足す
    size_t target = static_cast<size_t>((std::clamp)(count, 0, kMaxIcons));
    if (icons_.size() != target) {
        icons_.resize(target);
    }
}

void HpHudView::SetHp(int32_t hp, int32_t maxHp) {
    ResizeIcons(maxHp);
    int32_t count = static_cast<int32_t>(icons_.size());
    int32_t clampedHp = (std::clamp)(hp, 0, count);

    if (shownHp_ < 0) {
        // 初回は演出せず、失った分は暗い枠だけにする
        for (int32_t i = 0; i < count; ++i) {
            icons_[i].isAlive = i < clampedHp;
            icons_[i].lostTime = shatterTime_;
            icons_[i].pile.Clear();
        }
    } else if (clampedHp < shownHp_) {
        // 減った分の核を砕く。欠片は枠の中に積もって、失った状態の見た目になる
        float innerApothem = kFrameInnerRadius * std::cos(std::numbers::pi_v<float> / 6.0f);
        int32_t end = (std::min)(shownHp_, count);
        for (int32_t i = clampedHp; i < end; ++i) {
            Icon& icon = icons_[i];
            if (!icon.isAlive) {
                continue;
            }
            icon.isAlive = false;
            icon.lostTime = isDamagePending_ ? 0.0f : shatterTime_;
            icon.pile.Start(shardShapes_, spinAngle_, innerApothem, pileSettings_);
        }
    } else if (clampedHp > shownHp_) {
        // 増えた分は核を戻し、欠片を消す
        for (int32_t i = shownHp_; i < clampedHp; ++i) {
            icons_[i].isAlive = true;
            icons_[i].pile.Clear();
        }
    }

    isDamagePending_ = false;
    shownHp_ = clampedHp;
}

void HpHudView::Update(float deltaTime) {
    debugParam_->ApplyIfDirty();

    // 核の回転は、大きくなりすぎないよう1周で戻す
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    spinAngle_ = std::fmod(spinAngle_ + spinSpeed_ * deltaTime, kTwoPi);

    for (Icon& icon : icons_) {
        if (icon.isAlive) {
            continue;
        }
        icon.lostTime += deltaTime;
        icon.pile.Update(deltaTime, pileSettings_);
    }
}

Vector3 HpHudView::ScreenToWorld(const Vector2& screen) const {
    // 画面の点を、ニアとファーの2点へ戻して視線の向きを求める
    Matrix4x4 inverseVP = Matrix4x4::Inverse(camera_->GetVPMatrix());
    float ndcX = screen.x / kScreenWidth * 2.0f - 1.0f;
    float ndcY = 1.0f - screen.y / kScreenHeight * 2.0f;
    Vector3 nearPoint = Matrix4x4::Transform({ ndcX, ndcY, 0.0f }, inverseVP);
    Vector3 farPoint = Matrix4x4::Transform({ ndcX, ndcY, 1.0f }, inverseVP);
    Vector3 direction = Vector3::Normalize(farPoint - nearPoint);

    // カメラの前向きにdistanceだけ進んだ面と、視線の交わる点を返す
    Matrix4x4 cameraWorld = Matrix4x4::Inverse(camera_->GetViewMatrix());
    Vector3 cameraPosition = { cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2] };
    Vector3 forward = Vector3::Normalize({ cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2] });
    float along = (std::max)(Vector3::Dot(direction, forward), 0.0001f);
    return cameraPosition + direction * (distance_ / along);
}

Vector4 HpHudView::LerpLostColor(const Vector4& aliveColor, const Vector4& lostColor, float lostTime) const {
    float t = (std::clamp)(lostTime / (std::max)(shatterTime_, 0.01f), 0.0f, 1.0f);
    return {
        aliveColor.x + (lostColor.x - aliveColor.x) * t,
        aliveColor.y + (lostColor.y - aliveColor.y) * t,
        aliveColor.z + (lostColor.z - aliveColor.z) * t,
        aliveColor.w + (lostColor.w - aliveColor.w) * t };
}

void HpHudView::Draw(RenderQueue* renderQueue) {
    if (icons_.empty()) {
        return;
    }

    // 画面と平行に置くため、カメラの右・上・前の向きを、そのままアイコンの軸にする
    Matrix4x4 cameraWorld = Matrix4x4::Inverse(camera_->GetViewMatrix());
    Matrix4x4 basis = Matrix4x4::MakeIdentity();
    for (int32_t row = 0; row < 3; ++row) {
        Vector3 axis = Vector3::Normalize({ cameraWorld.m[row][0], cameraWorld.m[row][1], cameraWorld.m[row][2] });
        basis.m[row][0] = axis.x;
        basis.m[row][1] = axis.y;
        basis.m[row][2] = axis.z;
    }

    // 枠の幅は高さのcos30度倍（頂点が上の六角形）
    float iconWidth = iconSize_ * std::cos(std::numbers::pi_v<float> / 6.0f);
    Matrix4x4 spin = Matrix4x4::MakeRotateYMatrix(spinAngle_);

    uint32_t frameCount = 0;
    uint32_t coreCount = 0;
    float newestLostTime = -1.0f;   // 欠片があるアイコンのうち、一番新しく砕けたものの経過時間（-1は欠片なし）
    for (size_t i = 0; i < icons_.size(); ++i) {
        const Icon& icon = icons_[i];

        // 画面の中心と、アイコンの高さ分だけ下の点をワールドへ戻し、その距離から大きさを決める
        Vector2 center = {
            origin_.x + iconWidth * 0.5f + static_cast<float>(i) * (iconWidth + spacing_),
            origin_.y + iconSize_ * 0.5f };
        Vector3 worldCenter = ScreenToWorld(center);
        Vector3 worldBottom = ScreenToWorld({ center.x, center.y + iconSize_ });
        float scale = Vector3::Distance(worldCenter, worldBottom) * 2.0f / kFrameHeight;
        Matrix4x4 place = Matrix4x4::MakeScaleMatrix({ scale, scale, scale }) * basis * Matrix4x4::MakeTranslateMatrix(worldCenter);

        // 枠
        WorldTransforms::TransformData& frame = frameTransforms_.transformDatas_[frameCount++];
        frame.worldMatrix = place;
        frame.color = icon.isAlive ? frameColor_ : LerpLostColor(frameColor_, lostFrameColor_, icon.lostTime);
        frame.textureHandle = whiteTextureHandle_;

        // 核（画面の縦の軸まわりに回す）
        if (icon.isAlive) {
            WorldTransforms::TransformData& core = coreTransforms_.transformDatas_[coreCount++];
            core.worldMatrix = spin * place;
            core.color = coreColor_;
            core.textureHandle = whiteTextureHandle_;
            continue;
        }

        // 砕けた欠片。欠片の頂点は核のモデル座標のままなので、中心を軸に回してから、物理の位置へ動かす
        if (!icon.pile.IsActive()) {
            continue;
        }
        FractureInstance& instance = shardInstances_[i];
        instance.SetParentWorldMatrix(place);
        std::vector<FractureChunkState>& states = instance.GetTransformDatas();
        size_t shardCount = (std::min)(states.size(), icon.pile.GetCount());
        for (size_t c = 0; c < shardCount; ++c) {
            HpShardPile::ShardPose pose = icon.pile.GetPose(c);
            FractureChunkState& state = states[c];
            state.transform.scale = { 1.0f, 1.0f, 1.0f };
            state.transform.rotate = pose.rotate;
            state.transform.translate = pose.position - state.pivot;
        }
        instance.Update();
        renderQueue->SubmitFracture(models_.shard, instance);

        if (newestLostTime < 0.0f || icon.lostTime < newestLostTime) {
            newestLostTime = icon.lostTime;
        }
    }

    // 破片の描画はモデル自身のマテリアルを使い、色は全アイコンで共通になる。直近に砕けた核に合わせて暗くしていく
    if (newestLostTime >= 0.0f) {
        shardMaterial_->SetColor(LerpLostColor(coreColor_, lostCoreColor_, newestLostTime));
    }

    // 枠と核は、モデルごとに1回のインスタンス描画として登録する
    const GpuResource* material = &material_.GetMaterialBuffer();
    renderQueue->SubmitInstancing(models_.frame, frameCount, frameTransforms_, 1.0f, BlendMode::kBlendModeNormal, material);
    if (coreCount > 0) {
        renderQueue->SubmitInstancing(models_.core, coreCount, coreTransforms_, 1.0f, BlendMode::kBlendModeNormal, material);
    }
}
