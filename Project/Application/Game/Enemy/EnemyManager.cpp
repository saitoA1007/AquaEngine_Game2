#include "pch.h"
#include "EnemyManager.h"
#include "Application/Game/Enemy/ChaseEnemy.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include "MyMath.h"
#include "TLAS.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
using namespace GameEngine;

namespace {
    /// <summary>
    /// 種類名から個体を作る。実装されていない種類は例外にする。
    /// </summary>
    std::unique_ptr<Enemy> CreateEnemyByKind(const std::string& kind) {
        if (kind == "Chase") {
            return std::make_unique<ChaseEnemy>();
        }
        throw std::runtime_error("unknown enemy kind: " + kind);
    }
}

EnemyManager::EnemyManager(ModelManager* modelManager, float fieldRadius, const Camera* camera, uint32_t whiteTextureHandle, ShatterEffectManager* shatterEffect)
    : shatterEffect_(shatterEffect) {
    // 敵一覧を読み込む（不正なら例外）
    definitions_ = EnemyDefinitionIO::Load(EnemyDefinitionIO::kEnemyListPath);

    for (const EnemyDefinition& def : definitions_) {
        maxEnemyRadius_ = (std::max)(maxEnemyRadius_, def.radius);
    }

    RegisterDebugParameters();

    grid_.Initialize(fieldRadius + kGridMargin, kGridCellSize);
    CreatePools(modelManager);
    CreateShell(modelManager);

    // 氷ゲージは、敵の最大数までのバーを描けるようにする。平面モデルが見つからなければ例外にする
    Model* planeModel = modelManager->GetNameByModel(kGaugeModelName);
    if (planeModel == nullptr) {
        throw std::runtime_error(std::string("gauge model not found: ") + kGaugeModelName);
    }
    size_t totalEnemies = 0;
    for (const EnemyPool& pool : pools_) {
        totalEnemies += pool.enemies.size();
    }
    gaugeView_ = std::make_unique<EnemyGaugeView>(camera, planeModel, whiteTextureHandle, static_cast<uint32_t>(totalEnemies));

    drawTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    iceMaterial_.GetMaterialData()->color = shellColor_;

#ifdef USE_IMGUI
    listEditor_ = std::make_unique<EnemyListEditor>(EnemyDefinitionIO::kEnemyListPath);
#endif
}

void EnemyManager::RegisterDebugParameters() {
    // サブグループ名。ここを書き換えれば登録の全てに反映される
    const std::string kSpawnGroup = "Spawn";
    const std::string kIceGroup = "Ice";
    const std::string kWallGroup = "Wall";
    const std::string kGaugeGroup = "Gauge";

    debugParam_ = std::make_unique<DebugParameter>("Enemy");

    // スポーン
    debugParam_->Register("spawnInterval", spawnInterval_, 0, kSpawnGroup);
    debugParam_->Register("spawnPerInterval", spawnPerInterval_, 1, kSpawnGroup);
    debugParam_->Register("spawnDistance", spawnDistance_, 2, kSpawnGroup);

    // 氷
    debugParam_->Register("freezeRate", freezeRate_, 0, kIceGroup);
    debugParam_->Register("decayRate", decayRate_, 1, kIceGroup);
    debugParam_->Register("frozenDuration", frozenDuration_, 2, kIceGroup);
    debugParam_->Register("shellScale", shellScale_, 3, kIceGroup);
    debugParam_->Register("shellColor", shellColor_, 4, kIceGroup);
    debugParam_->Register("meltTime", meltTime_, 5, kIceGroup);

    // 壁
    debugParam_->Register("enableEnemyWall", enableEnemyWall_, 0, kWallGroup);
    debugParam_->Register("enableWallPushPlayer", enableWallPushPlayer_, 1, kWallGroup);
    debugParam_->Register("slideStrength", slideStrength_, 2, kWallGroup);

    // 氷ゲージの表示
    debugParam_->Register("gaugeVisible", gaugeSettings_.isVisible, 0, kGaugeGroup);
    debugParam_->Register("gaugeMaxCount", gaugeSettings_.maxCount, 1, kGaugeGroup);
    debugParam_->Register("gaugeWidth", gaugeSettings_.width, 2, kGaugeGroup);
    debugParam_->Register("gaugeHeight", gaugeSettings_.height, 3, kGaugeGroup);
    debugParam_->Register("gaugeBorder", gaugeSettings_.border, 4, kGaugeGroup);
    debugParam_->Register("gaugeHeadOffset", gaugeSettings_.headOffset, 5, kGaugeGroup);
    debugParam_->Register("gaugeBackColor", gaugeSettings_.backColor, 6, kGaugeGroup);
    debugParam_->Register("gaugeFillColor", gaugeSettings_.fillColor, 7, kGaugeGroup);

    debugParam_->Apply();
}

void EnemyManager::CreatePools(ModelManager* modelManager) {
    pools_.reserve(definitions_.size());

    for (const EnemyDefinition& def : definitions_) {
        EnemyPool pool;

        // 種類ごとの見た目。モデルが見つからなければ例外にする
        pool.visual = std::make_unique<EnemyVisual>();
        pool.visual->model = modelManager->GetNameByModel(def.modelName);
        if (pool.visual->model == nullptr) {
            throw std::runtime_error("enemy model not found: " + def.modelName);
        }

        // モデルのマテリアルを元に、種類ごとの色を持つマテリアルと参照バッファを作る
        const std::vector<std::unique_ptr<Mesh>>& meshes = pool.visual->model->GetMeshes();
        pool.visual->materials.resize(meshes.size());
        pool.visual->refBuffers.resize(meshes.size());
        for (size_t i = 0; i < meshes.size(); ++i) {
            Material* source = pool.visual->model->GetMaterial(meshes[i]->GetMaterialName());
            Material::MaterialData* sourceData = source->GetMaterialData();

            pool.visual->materials[i].Initialize(def.color, { 1.0f, 1.0f, 1.0f }, sourceData->shininess, true);
            Material::MaterialData* data = pool.visual->materials[i].GetMaterialData();
            data->textureHandle = sourceData->textureHandle;
            data->roughness = sourceData->roughness;
            data->metallic = sourceData->metallic;

            pool.visual->refBuffers[i].Create();
            pool.visual->refBuffers[i].SetBufferMaterial(
                static_cast<uint32_t>(BufferType::kDefalutMaterial), pool.visual->materials[i].GetMaterialSrvIndex());
        }

        // 個体を上限数だけ事前に確保する
        pool.enemies.reserve(static_cast<size_t>(def.poolSize));
        for (int32_t i = 0; i < def.poolSize; ++i) {
            pool.enemies.push_back(CreateEnemyByKind(def.kind));
        }

        pools_.push_back(std::move(pool));
    }

    // 使用中の個体を集める配列は、最大数を確保しておく
    size_t total = 0;
    for (const EnemyPool& pool : pools_) {
        total += pool.enemies.size();
    }
    active_.reserve(total);
}

void EnemyManager::CreateShell(ModelManager* modelManager) {
    shellModel_ = modelManager->GetNameByModel(kShellModelName);
    if (shellModel_ == nullptr) {
        throw std::runtime_error(std::string("shell model not found: ") + kShellModelName);
    }

    // 球を1メッシュのまま、氷マテリアルと氷用のヒットグループで描く
    shellRefBuffers_.resize(shellModel_->GetMeshes().size());
    for (RefBuffer& refBuffer : shellRefBuffers_) {
        refBuffer.Create();
        refBuffer.SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), iceMaterial_.GetMaterialSrvIndex());
        refBuffer.SetHitGroupIndex(1);
        refBuffer.SetInstanceMask(static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));
    }
}

void EnemyManager::Update() {
    // 調整値の反映だけを行う。敵の更新はUpdateEnemiesで行う
    if (debugParam_->ApplyIfDirty()) {
        iceMaterial_.GetMaterialData()->color = shellColor_;
    }
}

void EnemyManager::DebugUpdate() {
#ifdef USE_IMGUI
    listEditor_->Draw();
#endif
}

Enemy* EnemyManager::AcquireEnemy(EnemyPool& pool) {
    size_t count = pool.enemies.size();
    for (size_t i = 0; i < count; ++i) {
        size_t index = (pool.searchStart + i) % count;
        if (!pool.enemies[index]->IsActive()) {
            pool.searchStart = index + 1;
            return pool.enemies[index].get();
        }
    }
    return nullptr;
}

EnemyOutput EnemyManager::UpdateEnemies(const EnemyInput& input) {
    float deltaTime = FpsCounter::GetGameDeltaTime();
    lastPlayerPosition_ = input.playerPosition;

    // 出現させてから使用中の個体を集め、その位置でグリッドを作る
    SpawnEnemies(input, deltaTime);
    CollectActiveEnemies();
    grid_.Rebuild(active_);

    UpdateEnemyBehaviors(input, deltaTime);
    EmitReleaseShatters();

    if (enableEnemyWall_) {
        ResolveWalls(input, deltaTime);
    }
    ClampEnemiesToField(input.fieldRadius);

    EnemyOutput output;
    ResolvePlayerContact(input, output);
    return output;
}

void EnemyManager::SpawnEnemies(const EnemyInput& input, float deltaTime) {
    if (pools_.empty()) {
        return;
    }

    // 間隔が極端に短くならないようにする
    float interval = (std::max)(spawnInterval_, 0.05f);
    // 読み込み直後などで経過時間が極端に長くても、溜まった分を一度に出さない（1フレームに1回まで）
    spawnTimer_ = (std::min)(spawnTimer_ + deltaTime, interval);
    while (spawnTimer_ >= interval) {
        spawnTimer_ -= interval;
        for (int32_t i = 0; i < spawnPerInterval_; ++i) {
            SpawnOne(input);
        }
    }
}

void EnemyManager::SpawnOne(const EnemyInput& input) {
    // 空きのある種類を、ランダムな種類から順に探す
    size_t poolCount = pools_.size();
    size_t start = RandomGenerator::Get<size_t>(0, poolCount - 1);
    for (size_t i = 0; i < poolCount; ++i) {
        size_t poolIndex = (start + i) % poolCount;
        Enemy* enemy = AcquireEnemy(pools_[poolIndex]);
        if (enemy == nullptr) {
            continue;
        }

        // プレイヤーの周囲の円周上のランダムな方向に置き、フィールドの外なら円内へ収める
        float angle = RandomGenerator::Get<float>(0.0f, 2.0f * std::numbers::pi_v<float>);
        float x = input.playerPosition.x + std::cos(angle) * spawnDistance_;
        float z = input.playerPosition.z + std::sin(angle) * spawnDistance_;
        float limit = (std::max)(input.fieldRadius - definitions_[poolIndex].radius, 0.0f);
        float distance = std::sqrt(x * x + z * z);
        if (distance > limit && distance > 0.0f) {
            x *= limit / distance;
            z *= limit / distance;
        }

        enemy->Spawn(&definitions_[poolIndex], { x, 0.0f, z });
        return;
    }
}

void EnemyManager::CollectActiveEnemies() {
    active_.clear();
    for (EnemyPool& pool : pools_) {
        for (std::unique_ptr<Enemy>& enemy : pool.enemies) {
            if (enemy->IsActive()) {
                active_.push_back(enemy.get());
            }
        }
    }
}

void EnemyManager::UpdateEnemyBehaviors(const EnemyInput& input, float deltaTime) {
    EnemyContext context;
    context.deltaTime = deltaTime;
    context.playerPosition = input.playerPosition;
    context.iceAreaRadius = input.iceAreaRadius;
    context.freezeRate = freezeRate_;
    context.decayRate = decayRate_;
    context.frozenDuration = frozenDuration_;

    for (Enemy* enemy : active_) {
        enemy->Update(context);
    }
}

void EnemyManager::EmitReleaseShatters() {
    for (Enemy* enemy : active_) {
        if (!enemy->ConsumeReleasedEvent()) {
            continue;
        }

        // 解放では氷の殻だけが砕ける
        float scale = enemy->GetVisualScale();
        shatterEffect_->Spawn(enemy->GetPosition(), scale * shellScale_);
    }
}

void EnemyManager::ApplyIceMist(const IceMistInfo& info) {
    // グリッドは直前のUpdateEnemiesで作ったもの。まだ一度も作っていなければ敵はいない
    if (active_.empty()) {
        return;
    }

    // グリッドは候補を絞るためだけに使い、範囲との重なりは現在の位置で判定する
    grid_.Query(info.center, info.radius + maxEnemyRadius_ + kGridLag, neighbors_);
    for (Enemy* enemy : neighbors_) {
        if (!enemy->IsActive()) {
            continue;
        }

        float dx = enemy->GetPosition().x - info.center.x;
        float dz = enemy->GetPosition().z - info.center.z;
        float reach = info.radius + enemy->GetRadius();
        if (dx * dx + dz * dz > reach * reach) {
            continue;
        }

        // 凍っていない敵は凍結を進める。凍った敵は、別の氷霧で凍っていたときだけ砕く
        if (!enemy->IsFrozen()) {
            enemy->ApplyIceDamage(info.power, info.id, frozenDuration_);
            continue;
        }
        if (!enemy->CanShatter(info.id)) {
            continue;
        }

        // 撃破でも氷の殻の破片だけを飛ばす。解放（砕く回数が足りない）は、次のEmitReleaseShattersが殻だけを飛ばす
        float scale = enemy->GetVisualScale();
        Vector3 position = enemy->GetPosition();
        if (enemy->Shatter()) {
            shatterEffect_->Spawn(position, scale * shellScale_);
        }
    }
}

void EnemyManager::ResolveWalls(const EnemyInput& input, float deltaTime) {
    for (int32_t iteration = 0; iteration < kResolveIterations; ++iteration) {
        for (Enemy* enemy : active_) {
            if (enemy->IsFrozen()) {
                continue;
            }

            grid_.Query(enemy->GetPosition(), enemy->GetRadius() + maxEnemyRadius_ + kGridLag, neighbors_);
            for (Enemy* wall : neighbors_) {
                if (wall == enemy || !wall->IsFrozen()) {
                    continue;
                }

                float dx = enemy->GetPosition().x - wall->GetPosition().x;
                float dz = enemy->GetPosition().z - wall->GetPosition().z;
                float distance = std::sqrt(dx * dx + dz * dz);
                float minDistance = enemy->GetRadius() + wall->GetRadius();
                if (distance >= minDistance) {
                    continue;
                }

                // 壁から離れる向き。ぴったり重なっているときは決まらないので、X軸の向きにする
                float normalX = 1.0f;
                float normalZ = 0.0f;
                if (distance > 0.0001f) {
                    normalX = dx / distance;
                    normalZ = dz / distance;
                }

                // 重なった分だけ押し出す
                float overlap = minDistance - distance;
                enemy->AddPosition({ normalX * overlap, 0.0f, normalZ * overlap });

                // 最初の繰り返しだけ、壁に正面からぶつかった分を横滑りに変える
                if (iteration != 0) {
                    continue;
                }
                const Vector3& moveDirection = enemy->GetMoveDirection();
                float blocked = -(moveDirection.x * normalX + moveDirection.z * normalZ);
                if (blocked <= 0.0f) {
                    continue;
                }

                // 壁の接線方向。回り込む向きは、初めてぶつかったときにプレイヤー側の向きで決め、以後は固定する
                float tangentX = -normalZ;
                float tangentZ = normalX;
                if (enemy->GetWallSense() == 0) {
                    float toPlayerX = input.playerPosition.x - enemy->GetPosition().x;
                    float toPlayerZ = input.playerPosition.z - enemy->GetPosition().z;
                    float side = tangentX * toPlayerX + tangentZ * toPlayerZ;
                    enemy->SetWallSense(side >= 0.0f ? 1 : -1);
                }
                float sense = static_cast<float>(enemy->GetWallSense());
                float slide = enemy->GetDefinition()->moveSpeed * deltaTime * blocked * slideStrength_;
                enemy->AddPosition({ tangentX * sense * slide, 0.0f, tangentZ * sense * slide });
            }
        }
    }
}

void EnemyManager::ClampEnemiesToField(float fieldRadius) {
    for (Enemy* enemy : active_) {
        if (enemy->IsFrozen()) {
            continue;
        }

        float limit = (std::max)(fieldRadius - enemy->GetRadius(), 0.0f);
        float x = enemy->GetPosition().x;
        float z = enemy->GetPosition().z;
        float distance = std::sqrt(x * x + z * z);
        if (distance > limit && distance > 0.0f) {
            enemy->SetPosition({ x * limit / distance, 0.0f, z * limit / distance });
        }
    }
}

void EnemyManager::ResolvePlayerContact(const EnemyInput& input, EnemyOutput& output) {
    Vector3 playerPosition = input.playerPosition;
    grid_.Query(playerPosition, input.playerRadius + maxEnemyRadius_ + kGridLag, neighbors_);

    // 凍った敵は壁なので、重なった分だけプレイヤーを押し戻す
    if (enableWallPushPlayer_) {
        for (Enemy* enemy : neighbors_) {
            if (!enemy->IsFrozen()) {
                continue;
            }

            float dx = playerPosition.x - enemy->GetPosition().x;
            float dz = playerPosition.z - enemy->GetPosition().z;
            float distance = std::sqrt(dx * dx + dz * dz);
            float minDistance = input.playerRadius + enemy->GetRadius();
            if (distance >= minDistance) {
                continue;
            }

            // ぴったり重なっているときは向きが決まらないので、X軸の向きにする
            float normalX = 1.0f;
            float normalZ = 0.0f;
            if (distance > 0.0001f) {
                normalX = dx / distance;
                normalZ = dz / distance;
            }
            float overlap = minDistance - distance;
            playerPosition.x += normalX * overlap;
            playerPosition.z += normalZ * overlap;
            output.pushback.x += normalX * overlap;
            output.pushback.z += normalZ * overlap;
        }
    }

    // 動く敵との接触はダメージだけ。凍った敵との接触では受けない
    for (Enemy* enemy : neighbors_) {
        if (enemy->IsFrozen()) {
            continue;
        }

        float dx = playerPosition.x - enemy->GetPosition().x;
        float dz = playerPosition.z - enemy->GetPosition().z;
        float minDistance = input.playerRadius + enemy->GetRadius();
        if (dx * dx + dz * dz < minDistance * minDistance) {
            output.damage = (std::max)(output.damage, enemy->GetDefinition()->contactDamage);
        }
    }
}

void EnemyManager::Draw() {
    for (EnemyPool& pool : pools_) {
        for (std::unique_ptr<Enemy>& enemy : pool.enemies) {
            if (!enemy->IsActive()) {
                continue;
            }

            // 球が地面に接するよう、見た目の大きさに合わせて高さを決める
            float scale = enemy->GetVisualScale();
            Vector3 position = { enemy->GetPosition().x, scale, enemy->GetPosition().z };
            drawTransform_.SetWorldMatrix(Math::MakeWorldMatrixFromEulerRotation(position, { 0.0f, 0.0f, 0.0f }, { scale, scale, scale }));
            renderQueue_->SubmitRaytracingModel(pool.visual->model, drawTransform_, &pool.visual->refBuffers);

            // 凍結中は氷の殻を重ねる
            if (enemy->IsFrozen()) {
                // 解放の直前は、殻を縮めて溶けるように見せる（残り時間が短いほど小さく、イーズアウト）
                float melt = 1.0f;
                float remaining = enemy->GetFrozenTimeRemaining();
                if (meltTime_ > 0.0f && remaining < meltTime_) {
                    float t = (std::max)(remaining / meltTime_, 0.0f);
                    melt = t * (2.0f - t);
                }
                float shellSize = (std::max)(scale * shellScale_ * melt, 0.001f);
                drawTransform_.SetWorldMatrix(Math::MakeWorldMatrixFromEulerRotation(position, { 0.0f, 0.0f, 0.0f }, { shellSize, shellSize, shellSize }));
                renderQueue_->SubmitRaytracingModel(shellModel_, drawTransform_, &shellRefBuffers_);
            }
        }
    }

    // 凍りかけの敵の頭に氷ゲージを出す
    gaugeView_->Draw(renderQueue_, gaugeSettings_, active_, lastPlayerPosition_);
}
