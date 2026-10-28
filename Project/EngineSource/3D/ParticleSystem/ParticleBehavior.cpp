#include "pch.h"
#include "ParticleBehavior.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "ParticleEmitModules.h"
#include <algorithm>
using namespace GameEngine;

namespace {

    /// <summary>
    /// 位置に行列を掛ける
    /// </summary>
    Vector3 TransformPosition(const Vector3& v, const Matrix4x4& m) {
        Vector3 result{};
        result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
        result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
        result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
        float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
        if (w != 0.0f) {
            result.x /= w;
            result.y /= w;
            result.z /= w;
        }
        return result;
    }

    /// <summary>
    /// 方向ベクトルに行列を掛ける
    /// </summary>
    Vector3 TransformDirection(const Vector3& v, const Matrix4x4& m) {
        return {
            v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
            v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
            v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
        };
    }
}

ParticleBehavior::ParticleBehavior(const std::string& name, uint32_t maxNum, TextureManager* textureManager, Model* model) {
    maxNumInstance_ = maxNum;
    name_ = name;
    model_ = model;
    camera_ = &renderQueue_->GetMainCamera();

    // パーティクル配列を確保
    particles_.resize(maxNumInstance_);

    // WorldTransformsを初期化
    worldTransforms_ = std::make_unique<WorldTransforms>();
    Transform defaultTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    worldTransforms_->Initialize(maxNumInstance_, defaultTransform);

    // 全パーティクルを非アクティブ化
    for (auto& particle : particles_) {
        particle.currentTime = 1.0f;
        particle.lifeTime = 1.0f;
    }

    // パラメータ機能
    debugParame_ = std::make_unique<DebugParameter>(name);
    modulesControl_ = std::make_unique<ModulesControl>(debugParame_.get());
    // 登録
    int index = 0;
    std::string subGroup = "Emitter";
    debugParame_->Register("SpawnMaxCount", main_.spawnMaxCount, index++, subGroup);
    debugParame_->Register("SpawnCoolTime", main_.spawnCoolTime, index++, subGroup);
    debugParame_->Register("IsLoop", main_.isLoop, index++, subGroup);
    debugParame_->Register("IsBillBoard", main_.isBillBoard, index++, subGroup);
    debugParame_->Register("IsActiveBlendAdd", main_.isActiveBlendAdd_, index++, subGroup);
    debugParame_->Register("IsEmitByDistance", main_.isEmitByDistance, index++, subGroup);
    debugParame_->Register("SpawnDistance", main_.spawnDistance, index++, subGroup);
    subGroup += "/Defalut";
    debugParame_->Register("LifeTime", main_.lifeTime, index++, subGroup);
    debugParame_->Register("EmittePos", main_.emitterPos, index++, subGroup);
    debugParame_->Register("Rotate", main_.rotate, index++, subGroup);
    debugParame_->Register("Scale", main_.scale, index++, subGroup);
    debugParame_->Register("Color", main_.color, index++, subGroup);

    // 出現範囲を抑える
    if (maxNumInstance_ <= main_.spawnMaxCount) {
        main_.spawnMaxCount = maxNumInstance_;
    }

    // 値の適応
    debugParame_->Apply();
    modulesControl_->Update();
    debugParame_->Apply();

    // テクスチャの設定
    if (auto* textureModule = modulesControl_->GetModule<TextureModule>("TextureEmit")) {
        textureModule->SetTexture(textureManager);
    }
}

void ParticleBehavior::Initialize() {



}

void ParticleBehavior::Update() {
    Update(FpsCounter::deltaTime);
}

void ParticleBehavior::Update(float deltaTime) {
    // 値の適応
    debugParame_->ApplyIfDirty();

    // モジュールの更新
    modulesControl_->Update();

    // 出現範囲を抑える
    if (maxNumInstance_ <= main_.spawnMaxCount) {
        main_.spawnMaxCount = maxNumInstance_;
    }

    // パーティクルの発生を管理する
    const bool canSpawn = main_.isLoop && isEmitting_;
    if (main_.isEmitByDistance) {
        UpdateDistanceEmit(canSpawn);
    } else if (canSpawn) {
        Create(deltaTime);
    }

    Matrix4x4 cameraMatrix = camera_->GetWorldMatrix();
    if (renderQueue_->GetUseDebugCamera()) {
        cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();
    }
    // 移動処理
    Move(cameraMatrix, deltaTime);
}

void ParticleBehavior::Draw() {

    BlendMode blendMode = main_.isActiveBlendAdd_ ? BlendMode::kBlendModeAdd : BlendMode::kBlendModeNormal;
    renderQueue_->SubmitInstancing(model_, currentNumInstance_, *worldTransforms_, 0.0f, blendMode, nullptr, "WBOITAccumulatePass");

    // トレイル
    if (trailTransforms_ && trailNumInstance_ > 0) {
        renderQueue_->SubmitInstancing(model_, trailNumInstance_, *trailTransforms_, 0.0f, blendMode, nullptr, "WBOITAccumulatePass");
    }
}

void ParticleBehavior::Emit(const Vector3& pos) {
    emitterPos_ = pos;

    if (!main_.isLoop) {
        spawnTimer_ = main_.spawnCoolTime;
        // 生成する
        Create(0.0f);
    }
}

bool ParticleBehavior::HasAliveParticles() const {
    for (const auto& particle : particles_) {
        // ParticleData::IsAlive は寿命を過ぎているとtrueになる
        if (!particle.IsAlive()) {
            return true;
        }
    }
    return false;
}

void ParticleBehavior::Clear() {
    // 全パーティクルを非アクティブ化
    for (auto& particle : particles_) {
        particle.currentTime = 1.0f;
    }
    currentNumInstance_ = 0;
    trailNumInstance_ = 0;
    spawnTimer_ = 0.0f;
    ResetEmitHistory();
}

ParticleData ParticleBehavior::MakeNewParticle(const Vector3& offset) {

    ParticleData tmpParticleData;
    tmpParticleData.transform.translate = main_.emitterPos;
    tmpParticleData.transform.scale = main_.scale;
    tmpParticleData.transform.rotate = main_.rotate;
    tmpParticleData.velocity = { 0.0f,0.0f,0.0f };
    tmpParticleData.color = main_.color;
    tmpParticleData.rotateVelocity = { 0.0f,0.0f,0.0f };
    tmpParticleData.dir = { 0.0f,0.0f,0.0f };
    // 生存時間
    tmpParticleData.currentTime = 0.0f;
    tmpParticleData.lifeTime = main_.lifeTime;

    // モジュールを適応
    modulesControl_->ParticleCreate(tmpParticleData);

    // 初期値を取得する
    tmpParticleData.startColor = tmpParticleData.color;
    tmpParticleData.startSize = tmpParticleData.transform.scale;
    tmpParticleData.startSpeed = tmpParticleData.velocity;

    ColorOverLifeTimeModule* module = modulesControl_->GetModule<ColorOverLifeTimeModule>("ColorOverLifeTime");
    if (module != nullptr) {
        module->Create(tmpParticleData);
    }

    if (isSetEmitPos_) {
        tmpParticleData.transform.translate += emitterPos_;
    }
    // 距離発生時の補間位置ぶんずらす
    tmpParticleData.transform.translate += offset;

    return tmpParticleData;
}

void ParticleBehavior::Create(float deltaTime) {

    // 経過時間を加算
    spawnTimer_ += deltaTime;

    if (spawnTimer_ >= main_.spawnCoolTime) {
        uint32_t spawnCount = 0;
        for (uint32_t i = 0; i < maxNumInstance_; ++i) {
            // 時間が過ぎていれば新しく生成する
            if (1.0f <= particles_[i].currentTime) {
                particles_[i] = MakeNewParticle();
                spawnCount++;
            }
            // 指定した数発生させたら終了
            if (spawnCount >= main_.spawnMaxCount || spawnCount >= maxNumInstance_) {
                break;
            }
        }
        spawnTimer_ = 0.0f;
    }
}

void ParticleBehavior::Move(const Matrix4x4& cameraMatrix, float deltaTime) {
    currentNumInstance_ = 0;
    RotationByVelocityModule* module = modulesControl_->GetModule<RotationByVelocityModule>("RotationByVelocity");
    bool isRotateVelocity = false;
    if (module != nullptr) {
        isRotateVelocity = true;
    }

    // 常に親に追従するか
    const bool isFollowParent = IsFollowParent();

    // トレイル
    trailNumInstance_ = 0;
    const TrailModule* trailModule = modulesControl_->GetModule<TrailModule>("Trail");
    if (trailModule != nullptr && !trailTransforms_) {
        // 初めて有効になった時に確保する
        trailTransforms_ = std::make_unique<WorldTransforms>();
        Transform defaultTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        trailTransforms_->Initialize(maxNumInstance_ * kMaxTrailLength, defaultTransform);
    }

    for (uint32_t i = 0; i < maxNumInstance_; ++i) {
        ParticleData& particle = particles_[i];

        // 生存期間を過ぎたら描画対象にしない
        if (particle.IsAlive()) {
            continue;
        }

        // 更新
        modulesControl_->ParticleUpdate(particle, deltaTime);

        // 経過時間を加算
        particle.currentTime += deltaTime / particle.lifeTime;
        // 速度を追加
        particle.transform.translate += particle.velocity * deltaTime;
        // 回転速度
        particle.transform.rotate += particle.rotateVelocity * deltaTime;

        // worldTransformsの更新
        if (main_.isBillBoard) {

            Vector3 worldPos = particle.transform.translate;
            Vector3 worldVelocity = particle.velocity;
            if (isFollowParent) {
                worldPos = TransformPosition(worldPos, *parentMatrix_);
                worldVelocity = TransformDirection(worldVelocity, *parentMatrix_);
            }

            // ビルボードを適応する
            if (isRotateVelocity) {
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeDirectionalBillboardMatrix(particle.transform.scale, particle.transform.translate, cameraMatrix, camera_->GetViewMatrix(), particle.velocity, particle.transform.rotate.z);
            } else {
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeBillboardMatrix(particle.transform.scale, particle.transform.translate, particle.transform.rotate.z,cameraMatrix);
            }

            // ペアレント
            if (parentMatrix_ != nullptr) {
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix *= *parentMatrix_;
            }
        } else {
            if (isFollowParent) {
                // ローカル行列を作ってから親のワールド行列を掛ける
                Matrix4x4 localMatrix = Matrix4x4::MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = localMatrix * (*parentMatrix_);
                // 後段のUpdateTransformMatrixで上書きされないように、transformも同期しておく
                worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
            } else {
                worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
            }
        }

        worldTransforms_->transformDatas_[currentNumInstance_].color = particle.color;
        worldTransforms_->transformDatas_[currentNumInstance_].textureHandle = particle.textureHandle;
        currentNumInstance_++;

        // トレイルを追加
        if (trailModule != nullptr) {
            AddTrail(particle, *trailModule, cameraMatrix, isRotateVelocity, isFollowParent);
        }
    }

    // 行列の更新処理
    if (!main_.isBillBoard && !isFollowParent) {
        worldTransforms_->UpdateTransformMatrix(currentNumInstance_);
    }
}

void ParticleBehavior::AddTrail(const ParticleData& particle, const TrailModule& trailModule, const Matrix4x4& cameraMatrix, bool isRotateVelocity, bool isFollowParent) {

    const uint32_t count = (std::min)(particle.trailCount, trailModule.GetTrailLength());

    for (uint32_t index = 0; index < count; ++index) {
        WorldTransforms::TransformData& data = trailTransforms_->transformDatas_[trailNumInstance_];

        const Vector3& position = particle.GetTrailPosition(index);
        Vector3 scale{};
        Vector4 color{};
        trailModule.CalcTrailPoint(particle, index, scale, color);

        // パーティクル本体と同じ方法で行列を作る
        if (main_.isBillBoard) {
            if (isRotateVelocity) {
                data.worldMatrix = Math::MakeDirectionalBillboardMatrix(scale, position, cameraMatrix, camera_->GetViewMatrix(), particle.velocity, particle.transform.rotate.z);
            } else {
                data.worldMatrix = Math::MakeBillboardMatrix(scale, position, particle.transform.rotate.z, cameraMatrix);
            }

            // ペアレント
            if (parentMatrix_ != nullptr) {
                data.worldMatrix *= *parentMatrix_;
            }
        } else {
            if (isFollowParent) {
                data.worldMatrix = Matrix4x4::MakeAffineMatrix(scale, particle.transform.rotate, position) * (*parentMatrix_);
            } else {
                data.worldMatrix = Math::MakeWorldMatrixFromEulerRotation(position, particle.transform.rotate, scale);
            }
        }

        data.transform = { scale, particle.transform.rotate, position };
        data.color = color;
        data.textureHandle = particle.textureHandle;
        trailNumInstance_++;
    }
}

Vector3 ParticleBehavior::GetEmitterWorldPos() const {
    Vector3 pos = main_.emitterPos;
    if (isSetEmitPos_) {
        pos += emitterPos_;
    }
    // 親がいる場合は、親の動きもエミッターとしてカウントする
    if (parentMatrix_ != nullptr) {
        pos = TransformPosition(pos, *parentMatrix_);
    }
    return pos;
}

uint32_t ParticleBehavior::SpawnParticles(uint32_t count, const Vector3& offset) {
    uint32_t spawned = 0;
    for (uint32_t i = 0; i < maxNumInstance_ && spawned < count; ++i) {
        if (1.0f <= particles_[i].currentTime) {
            particles_[i] = MakeNewParticle(offset);
            ++spawned;
        }
    }
    return spawned;
}

void ParticleBehavior::UpdateDistanceEmit(bool canSpawn) {
    const Vector3 current = GetEmitterWorldPos();

    // 初回、または発生停止中は位置だけ追従する
    if (!hasPrevEmitPos_ || !canSpawn) {
        prevEmitWorldPos_ = current;
        hasPrevEmitPos_ = true;
        return;
    }

    const float interval = (std::max)(main_.spawnDistance, 0.001f);
    const Vector3 delta = current - prevEmitWorldPos_;
    const float moved = delta.Length();
    if (moved <= 0.0f) {
        return;
    }

    const float travelled = distanceAccum_ + moved;
    uint32_t count = static_cast<uint32_t>(travelled / interval);

    // 瞬間移動などで大量発生しないように上限をかける
    count = (std::min)(count, maxNumInstance_);

    for (uint32_t k = 1; k <= count; ++k) {
        // 前回位置から数えて、この発生点が経路上のどこにあるか
        const float t = (static_cast<float>(k) * interval - distanceAccum_) / moved;
        // 親に追従する場合、粒は親と一緒に動くので補間は不要
        Vector3 offset{};
        if (!IsFollowParent()) {
            // 現在位置から経路を遡った分
            offset = delta * (t - 1.0f); 
        }
        SpawnParticles(main_.spawnMaxCount, offset);
    }

    distanceAccum_ = travelled - static_cast<float>(count) * interval;
    prevEmitWorldPos_ = current;
}
