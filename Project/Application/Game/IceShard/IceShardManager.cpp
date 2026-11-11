#include "pch.h"
#include "IceShardManager.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include "MyMath.h"
#include "TLAS.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
using namespace GameEngine;

IceShardManager::IceShardManager(Model* sphereModel, Model* planeModel, TextureManager* textureManager, EnemyManager* enemyManager)
    : enemyManager_(enemyManager), model_(sphereModel) {
    if (model_ == nullptr || planeModel == nullptr || enemyManager_ == nullptr) {
        throw std::runtime_error("IceShardManager: model or enemy manager is null");
    }

    RegisterDebugParameters();

    // 氷塊は事前に確保して使い回す
    shards_.resize(kMaxShards);

    CreateVisual();
    CreateParticles(planeModel, textureManager);

    drawTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    iceMaterial_.GetMaterialData()->color = shardColor_;
}

void IceShardManager::RegisterDebugParameters() {
    // サブグループ名。ここを書き換えれば登録の全てに反映される
    const std::string kCollectGroup = "Collect";
    const std::string kOrbitGroup = "Orbit";
    const std::string kChainGroup = "Chain";
    const std::string kVisualGroup = "Visual";

    debugParam_ = std::make_unique<DebugParameter>("IceShard");

    // 回収
    debugParam_->Register("maxHeldCount", maxHeldCount_, 0, kCollectGroup);
    debugParam_->Register("attractRadius", attractRadius_, 1, kCollectGroup);
    debugParam_->Register("attractSpeed", attractSpeed_, 2, kCollectGroup);
    debugParam_->Register("collectRadius", collectRadius_, 3, kCollectGroup);
    debugParam_->Register("groundLifetime", groundLifetime_, 4, kCollectGroup);
    debugParam_->Register("dropScatter", dropScatter_, 5, kCollectGroup);

    // 所持の見た目
    debugParam_->Register("orbitRadius", orbitRadius_, 0, kOrbitGroup);
    debugParam_->Register("orbitHeight", orbitHeight_, 1, kOrbitGroup);
    debugParam_->Register("orbitSpeed", orbitSpeed_, 2, kOrbitGroup);
    debugParam_->Register("orbitSpeedVariation", orbitSpeedVariation_, 3, kOrbitGroup);
    debugParam_->Register("orbitSpinSpeed", orbitSpinSpeed_, 4, kOrbitGroup);
    debugParam_->Register("orbitMinRadiusRatio", orbitMinRadiusRatio_, 5, kOrbitGroup);

    // 連鎖
    debugParam_->Register("aimHalfAngleDegrees", aimHalfAngleDegrees_, 0, kChainGroup);
    debugParam_->Register("launchInterval", launchInterval_, 1, kChainGroup);
    debugParam_->Register("maxLaunchSpan", maxLaunchSpan_, 2, kChainGroup);
    debugParam_->Register("hopTime", hopTime_, 3, kChainGroup);
    debugParam_->Register("maxHopSpan", maxHopSpan_, 4, kChainGroup);
    debugParam_->Register("hopArcHeight", hopArcHeight_, 5, kChainGroup);
    debugParam_->Register("hitHeight", hitHeight_, 6, kChainGroup);
    debugParam_->Register("shardIcePower", shardIcePower_, 7, kChainGroup);

    // 氷塊の見た目
    debugParam_->Register("shardScale", shardScale_, 0, kVisualGroup);
    debugParam_->Register("shardColor", shardColor_, 1, kVisualGroup);
    debugParam_->Register("hitEffectHeight", hitEffectHeight_, 2, kVisualGroup);

    debugParam_->Apply();
}

void IceShardManager::CreateVisual() {
    // 球を1メッシュのまま、氷マテリアルと氷用のヒットグループで描く（敵の氷の殻と同じ方式）
    refBuffers_.resize(model_->GetMeshes().size());
    for (RefBuffer& refBuffer : refBuffers_) {
        refBuffer.Create();
        refBuffer.SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), iceMaterial_.GetMaterialSrvIndex());
        refBuffer.SetHitGroupIndex(1);
        refBuffer.SetInstanceMask(static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));
    }
}

void IceShardManager::CreateParticles(Model* planeModel, TextureManager* textureManager) {
    // 軌跡と着弾は、氷塊の位置からEmitで発生させるため、ループは無効にする（見た目はJSONで決める）
    trailParticle_ = std::make_unique<ParticleBehavior>(kTrailParticleName, kMaxTrailParticles, textureManager, planeModel);
    trailParticle_->Initialize();
    trailParticle_->SetIsLoop(false);

    hitParticle_ = std::make_unique<ParticleBehavior>(kHitParticleName, kMaxHitParticles, textureManager, planeModel);
    hitParticle_->Initialize();
    hitParticle_->SetIsLoop(false);
}

void IceShardManager::Update() {
    // 調整値の反映と、粒の更新を行う。氷塊の更新はUpdateShardsで行う
    if (debugParam_->ApplyIfDirty()) {
        iceMaterial_.GetMaterialData()->color = shardColor_;
    }

    trailParticle_->Update();
    hitParticle_->Update();
}

int32_t IceShardManager::GetHeldCount() const {
    int32_t count = 0;
    for (const Shard& shard : shards_) {
        if (shard.state == ShardState::kHeld) {
            ++count;
        }
    }
    return count;
}

IceShardManager::Shard* IceShardManager::AcquireShard() {
    // 空きを優先し、なければ一番古い地面の氷塊を再利用する
    Shard* oldest = nullptr;
    for (Shard& shard : shards_) {
        if (shard.state == ShardState::kInactive) {
            return &shard;
        }
        if (shard.state == ShardState::kGround && (oldest == nullptr || shard.age > oldest->age)) {
            oldest = &shard;
        }
    }
    return oldest;
}

int32_t IceShardManager::AcquireVolley() {
    for (size_t i = 0; i < volleys_.size(); ++i) {
        if (volleys_[i].shardCount == 0) {
            return static_cast<int32_t>(i);
        }
    }
    volleys_.emplace_back();
    return static_cast<int32_t>(volleys_.size() - 1);
}

void IceShardManager::Drop(const Vector3& position, int32_t count) {
    for (int32_t i = 0; i < count; ++i) {
        Shard* shard = AcquireShard();
        if (shard == nullptr) {
            return;
        }

        // 撃破した位置の周りにばらして落とす
        float angle = RandomGenerator::Get<float>(0.0f, 2.0f * std::numbers::pi_v<float>);
        float distance = std::sqrt(RandomGenerator::Get<float>(0.0f, 1.0f)) * dropScatter_;

        *shard = Shard();
        shard->state = ShardState::kGround;
        shard->position = { position.x + std::cos(angle) * distance, shardScale_, position.z + std::sin(angle) * distance };
    }
}

void IceShardManager::UpdateShards(const IceShardInput& input) {
    float deltaTime = FpsCounter::GetGameDeltaTime();
    playerPosition_ = input.playerPosition;
    orbitPhase_ += orbitSpeed_ * deltaTime;
    orbitSpin_ += orbitSpinSpeed_ * deltaTime;

    // 所持と引き寄せ中の数を数える。上限を超えて引き寄せないために使う
    int32_t carryCount = 0;
    for (const Shard& shard : shards_) {
        if (shard.state == ShardState::kHeld || shard.state == ShardState::kPulled) {
            ++carryCount;
        }
    }


    for (Shard& shard : shards_) {
        switch (shard.state) {
        case ShardState::kGround:
            UpdateGroundShard(shard, deltaTime, input, carryCount);
            break;
        case ShardState::kPulled:
            UpdatePulledShard(shard, deltaTime, input);
            break;
        case ShardState::kHeld:
        case ShardState::kQueued:
            UpdateOrbitShard(shard, deltaTime);
            break;
        case ShardState::kFlying:
            UpdateFlyingShard(shard, deltaTime);
            break;
        default:
            break;
        }
    }
}

void IceShardManager::UpdateGroundShard(Shard& shard, float deltaTime, const IceShardInput& input, int32_t& carryCount) {
    // 落ちてから時間が経ったら消える
    shard.age += deltaTime;
    if (shard.age >= groundLifetime_) {
        shard.state = ShardState::kInactive;
        return;
    }

    // チャージ中で、持てる数に余裕があり、範囲内なら引き寄せを始める
    if (!input.isCharging || carryCount >= maxHeldCount_) {
        return;
    }
    float dx = shard.position.x - input.playerPosition.x;
    float dz = shard.position.z - input.playerPosition.z;
    if (dx * dx + dz * dz > attractRadius_ * attractRadius_) {
        return;
    }

    shard.state = ShardState::kPulled;
    shard.orbitAngle = static_cast<float>(orbitCounter_++) * kGoldenAngle;
    AssignOrbit(shard);
    ++carryCount;
}

void IceShardManager::UpdatePulledShard(Shard& shard, float deltaTime, const IceShardInput& input) {
    // チャージをやめたら、その場の地面へ戻す（寿命は続きから）
    if (!input.isCharging) {
        shard.state = ShardState::kGround;
        shard.position.y = shardScale_;
        return;
    }

    // 周回の位置へ向かって一定の速さで近づく
    Vector3 goal = GetOrbitPosition(shard);
    float dx = goal.x - shard.position.x;
    float dy = goal.y - shard.position.y;
    float dz = goal.z - shard.position.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    float step = attractSpeed_ * deltaTime;
    if (distance <= collectRadius_ || distance <= step) {
        shard.state = ShardState::kHeld;
        shard.position = goal;
        return;
    }

    shard.position.x += dx / distance * step;
    shard.position.y += dy / distance * step;
    shard.position.z += dz / distance * step;
}

void IceShardManager::UpdateOrbitShard(Shard& shard, float deltaTime) {
    shard.position = GetOrbitPosition(shard);
    if (shard.state != ShardState::kQueued) {
        return;
    }

    // 順番が来たら飛び始める
    shard.launchDelay -= deltaTime;
    if (shard.launchDelay > 0.0f) {
        return;
    }
    if (!StartNextHop(shard)) {
        FinishShard(shard);
        return;
    }
    shard.state = ShardState::kFlying;
}

void IceShardManager::AssignOrbit(Shard& shard) {
    // 軌道の平面の法線を、球面上のランダムな向きに決める
    float normalY = RandomGenerator::Get<float>(-1.0f, 1.0f);
    float normalAngle = RandomGenerator::Get<float>(0.0f, 2.0f * std::numbers::pi_v<float>);
    float normalHorizontal = std::sqrt(1.0f - normalY * normalY);
    Vector3 normal = { normalHorizontal * std::cos(normalAngle), normalY, normalHorizontal * std::sin(normalAngle) };

    // 法線に垂直な2本の単位ベクトルで、軌道の平面を張る。法線が縦に近いときは横の軸を基準にする
    Vector3 reference = std::abs(normal.y) > 0.99f ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    shard.orbitAxisU = Vector3::Normalize(Vector3::Cross(normal, reference));
    shard.orbitAxisV = Vector3::Cross(normal, shard.orbitAxisU);

    // 速さと向きと軌道の半径を氷塊ごとに変える（半径は球の中に散らすため、下限から1の間）
    float speedScale = 1.0f + RandomGenerator::Get<float>(-orbitSpeedVariation_, orbitSpeedVariation_);
    bool isReverse = RandomGenerator::Get<float>(0.0f, 1.0f) < 0.5f;
    shard.orbitSpeedScale = isReverse ? -speedScale : speedScale;
    shard.orbitRadiusScale = RandomGenerator::Get<float>((std::min)(orbitMinRadiusRatio_, 1.0f), 1.0f);
}

Vector3 IceShardManager::GetOrbitPosition(const Shard& shard) const {
    // 氷塊ごとの傾いた軌道の上を回る
    float angle = shard.orbitAngle + orbitPhase_ * shard.orbitSpeedScale;
    float radius = orbitRadius_ * shard.orbitRadiusScale;
    float cosAngle = std::cos(angle);
    float sinAngle = std::sin(angle);
    Vector3 offset = {
        (shard.orbitAxisU.x * cosAngle + shard.orbitAxisV.x * sinAngle) * radius,
        (shard.orbitAxisU.y * cosAngle + shard.orbitAxisV.y * sinAngle) * radius,
        (shard.orbitAxisU.z * cosAngle + shard.orbitAxisV.z * sinAngle) * radius };

    // 塊全体を縦軸でゆっくり自転させる
    float cosSpin = std::cos(orbitSpin_);
    float sinSpin = std::sin(orbitSpin_);
    float spunX = offset.x * cosSpin + offset.z * sinSpin;
    float spunZ = -offset.x * sinSpin + offset.z * cosSpin;

    return { playerPosition_.x + spunX, orbitHeight_ + offset.y, playerPosition_.z + spunZ };
}

void IceShardManager::EmitTrail(const Shard& shard) {
    trailParticle_->SetEmitterPos(shard.position);
    trailParticle_->Emit(shard.position);
}

void IceShardManager::EmitHit(const Vector3& position) {
    // 敵の中心から出すと体と殻に隠れるため、殻の上側の高さから出す
    Vector3 emitPosition = { position.x, hitEffectHeight_, position.z };
    hitParticle_->SetEmitterPos(emitPosition);
    hitParticle_->Emit(emitPosition);
}

bool IceShardManager::StartNextHop(Shard& shard) {
    const Volley& volley = volleys_[static_cast<size_t>(shard.volleyIndex)];

    // 倒された敵は飛ばして、次の生きている標的を探す
    while (shard.nextTarget < volley.order.size()) {
        const EnemyTarget& candidate = volley.order[shard.nextTarget];
        ++shard.nextTarget;

        Vector3 targetPosition;
        if (!enemyManager_->GetTargetPosition(candidate, targetPosition)) {
            continue;
        }

        shard.target = candidate;
        shard.hopStart = shard.position;
        shard.hopEnd = { targetPosition.x, hitHeight_, targetPosition.z };
        shard.hopTimer = 0.0f;
        return true;
    }
    return false;
}

void IceShardManager::UpdateFlyingShard(Shard& shard, float deltaTime) {
    // 標的が動いていれば到着点を追う。倒されていたら、最後の位置へそのまま向かう
    Vector3 targetPosition;
    bool isTargetAlive = enemyManager_->GetTargetPosition(shard.target, targetPosition);
    if (isTargetAlive) {
        shard.hopEnd = { targetPosition.x, hitHeight_, targetPosition.z };
    }

    float hopTime = (std::max)(volleys_[static_cast<size_t>(shard.volleyIndex)].hopTime, kMinHopTime);
    shard.hopTimer += deltaTime;
    float t = (std::min)(shard.hopTimer / hopTime, 1.0f);

    // 出発点から到着点へ直線で進み、途中で少し持ち上げる
    shard.position.x = shard.hopStart.x + (shard.hopEnd.x - shard.hopStart.x) * t;
    shard.position.y = shard.hopStart.y + (shard.hopEnd.y - shard.hopStart.y) * t + std::sin(t * std::numbers::pi_v<float>) * hopArcHeight_;
    shard.position.z = shard.hopStart.z + (shard.hopEnd.z - shard.hopStart.z) * t;

    // 飛んでいる間、毎フレーム位置から軌跡の粒を1個出す
    EmitTrail(shard);
    if (t < 1.0f) {
        return;
    }

    // 到着。着弾の粒を弾けさせ、着弾を処理して、次の標的へ向かう。全員に当たったら砕けて消える
    shard.position = shard.hopEnd;
    EmitHit(shard.position);
    enemyManager_->ApplyShardHit(shard.target, shardIcePower_, volleys_[static_cast<size_t>(shard.volleyIndex)].chainId);

    // 飛んでいる間に倒された敵への到着は、命中に数えない
    if (isTargetAlive && hitCallback_) {
        hitCallback_();
    }
    if (!StartNextHop(shard)) {
        FinishShard(shard);
    }
}

void IceShardManager::FinishShard(Shard& shard) {
    if (shard.volleyIndex >= 0) {
        --volleys_[static_cast<size_t>(shard.volleyIndex)].shardCount;
    }
    shard.state = ShardState::kInactive;
    shard.volleyIndex = -1;
}

void IceShardManager::Launch(const IceMistInfo& info) {
    // 持っている氷塊がなければ何もしない
    int32_t heldCount = GetHeldCount();
    if (heldCount == 0) {
        return;
    }

    // 氷霧の範囲内に敵がいなければ、氷塊は消費しない
    enemyManager_->CollectTargets(info.center, info.radius, candidates_);
    if (candidates_.empty()) {
        return;
    }

    int32_t volleyIndex = AcquireVolley();
    Volley& volley = volleys_[static_cast<size_t>(volleyIndex)];
    volley.chainId = info.id;
    BuildChainOrder(info, volley.order);
    if (volley.order.empty()) {
        return;
    }

    // 敵の数が多いときは、1個が全敵を回る時間が上限に収まるよう1区間を短くする
    float hopTime = (std::min)(hopTime_, maxHopSpan_ / static_cast<float>(volley.order.size()));
    volley.hopTime = (std::max)(hopTime, kMinHopTime);

    // 氷塊の数が多いときは、全氷塊が飛び始めるまでの時間が上限に収まるよう間隔を詰める
    float interval = (std::min)(launchInterval_, maxLaunchSpan_ / static_cast<float>(heldCount));

    // 持っている全部を、順番に飛び始めるよう並べる
    int32_t order = 0;
    for (Shard& shard : shards_) {
        if (shard.state != ShardState::kHeld) {
            continue;
        }

        shard.state = ShardState::kQueued;
        shard.volleyIndex = volleyIndex;
        shard.launchDelay = interval * static_cast<float>(order);
        shard.nextTarget = 0;
        ++order;
    }
    volley.shardCount = order;
}

void IceShardManager::BuildChainOrder(const IceMistInfo& info, std::vector<EnemyTarget>& order) {
    order.clear();

    // 現在位置を取り直し、倒された敵を除いた未訪問の一覧にする
    remainingTargets_.clear();
    remainingPositions_.clear();
    for (const EnemyTarget& candidate : candidates_) {
        Vector3 position;
        if (!enemyManager_->GetTargetPosition(candidate, position)) {
            continue;
        }
        remainingTargets_.push_back(candidate);
        remainingPositions_.push_back(position);
    }
    if (remainingTargets_.empty()) {
        return;
    }

    // 1個目を決め、以降は直前の敵の最寄りを選んでいく
    order.reserve(remainingTargets_.size());
    size_t index = PickFirstIndex(info);
    while (true) {
        order.push_back(remainingTargets_[index]);
        Vector3 current = remainingPositions_[index];

        // 順番を保たなくてよいので、末尾と入れ替えて取り除く
        remainingTargets_[index] = remainingTargets_.back();
        remainingPositions_[index] = remainingPositions_.back();
        remainingTargets_.pop_back();
        remainingPositions_.pop_back();
        if (remainingTargets_.empty()) {
            break;
        }

        float bestDistance = 0.0f;
        index = 0;
        for (size_t i = 0; i < remainingPositions_.size(); ++i) {
            float dx = remainingPositions_[i].x - current.x;
            float dz = remainingPositions_[i].z - current.z;
            float distance = dx * dx + dz * dz;
            if (i == 0 || distance < bestDistance) {
                bestDistance = distance;
                index = i;
            }
        }
    }
}

size_t IceShardManager::PickFirstIndex(const IceMistInfo& info) const {
    float directionLength = std::sqrt(info.moveDirection.x * info.moveDirection.x + info.moveDirection.z * info.moveDirection.z);
    bool isMoving = directionLength > kMoveEpsilon;
    float cosLimit = std::cos(aimHalfAngleDegrees_ * std::numbers::pi_v<float> / 180.0f);

    // 全体の最寄りと、移動方向の円錐内の最寄りを同時に探す
    size_t nearestAll = 0;
    float nearestAllDistance = 0.0f;
    bool hasCone = false;
    size_t nearestCone = 0;
    float nearestConeDistance = 0.0f;
    for (size_t i = 0; i < remainingPositions_.size(); ++i) {
        float dx = remainingPositions_[i].x - info.center.x;
        float dz = remainingPositions_[i].z - info.center.z;
        float distanceSquared = dx * dx + dz * dz;
        if (i == 0 || distanceSquared < nearestAllDistance) {
            nearestAllDistance = distanceSquared;
            nearestAll = i;
        }
        if (!isMoving) {
            continue;
        }

        // 真上に重なっている敵は向きが決まらないので、円錐内として扱う
        float distance = std::sqrt(distanceSquared);
        bool isInCone = distance < 0.0001f || (dx * info.moveDirection.x + dz * info.moveDirection.z) / (distance * directionLength) >= cosLimit;
        if (isInCone && (!hasCone || distanceSquared < nearestConeDistance)) {
            hasCone = true;
            nearestConeDistance = distanceSquared;
            nearestCone = i;
        }
    }

    return hasCone ? nearestCone : nearestAll;
}

void IceShardManager::Draw() {
    for (const Shard& shard : shards_) {
        if (shard.state != ShardState::kInactive) {
            DrawShard(shard);
        }
    }

    trailParticle_->Draw();
    hitParticle_->Draw();
}

void IceShardManager::DrawShard(const Shard& shard) {
    // 地面の氷塊は、消える直前に縮める
    float scale = shardScale_;
    if (shard.state == ShardState::kGround) {
        float remaining = groundLifetime_ - shard.age;
        if (remaining < kFadeTime) {
            scale *= (std::max)(remaining / kFadeTime, 0.0f);
        }
    }
    scale = (std::max)(scale, 0.001f);

    drawTransform_.SetWorldMatrix(Math::MakeWorldMatrixFromEulerRotation(shard.position, { 0.0f, 0.0f, 0.0f }, { scale, scale, scale }));
    renderQueue_->SubmitRaytracingModel(model_, drawTransform_, &refBuffers_);
}
