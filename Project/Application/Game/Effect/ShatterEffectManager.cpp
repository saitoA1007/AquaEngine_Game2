#include "pch.h"
#include "ShatterEffectManager.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include "MyMath.h"
#include <algorithm>
#include <execution>
#include <cmath>
#include <fstream>
#include <numbers>
#include <stdexcept>
using namespace GameEngine;

ShatterEffectManager::ShatterEffectManager(Model* shellModel) {
    if (shellModel == nullptr) {
        throw std::runtime_error("shard model not found");
    }

    RegisterDebugParameters();

    SetUpShardSet(shellSet_, shellModel);
    shellSet_.material.GetMaterialData()->color = shellColor_;

    identityTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });

    AllocateSlots();
}

ShatterEffectManager::~ShatterEffectManager() {
    // 共有モデルが、解放済みのマテリアルを参照し続けないように元へ戻す
    if (shellSet_.model == nullptr) {
        return;
    }
    shellSet_.model->GetFractureBuffers().at(shellSet_.groupName)->SetBufferMaterial(
        shellSet_.originalSrvIndex, static_cast<uint32_t>(RayInstanceMask::kRayMaskOpaque));
}

void ShatterEffectManager::RegisterDebugParameters() {
    // サブグループ名。ここを書き換えれば登録の全てに反映される
    const std::string kMotionGroup = "Motion";
    const std::string kColorGroup = "Color";
    const std::string kBenchmarkGroup = "Benchmark";

    debugParam_ = std::make_unique<DebugParameter>("ShatterEffect");

    // 破片の動き
    debugParam_->Register("minLaunchSpeed", minLaunchSpeed_, 0, kMotionGroup);
    debugParam_->Register("maxLaunchSpeed", maxLaunchSpeed_, 1, kMotionGroup);
    debugParam_->Register("maxAngularSpeed", maxAngularSpeed_, 2, kMotionGroup);
    debugParam_->Register("lifetime", lifetime_, 3, kMotionGroup);
    debugParam_->Register("fadeStartRatio", fadeStartRatio_, 4, kMotionGroup);
    debugParam_->Register("shardSizeRatio", shardSizeRatio_, 5, kMotionGroup);
    debugParam_->Register("parallelThreshold", parallelThreshold_, 6, kMotionGroup);

    // 色
    debugParam_->Register("shellColor", shellColor_, 0, kColorGroup);

    // 計測
    debugParam_->Register("benchmarkCount", benchmarkCount_, 0, kBenchmarkGroup);
    debugParam_->Register("benchmarkSeconds", benchmarkSeconds_, 1, kBenchmarkGroup);
    debugParam_->Register("benchmarkSpread", benchmarkSpread_, 2, kBenchmarkGroup);

    debugParam_->Apply();
}

void ShatterEffectManager::SetUpShardSet(ShardSet& set, Model* model) {
    // 単一の破壊グループを持つモデルだけを扱う
    const std::unordered_map<std::string, std::vector<FractureChunkEntry>>& groups = model->GetFractureChunks();
    if (groups.size() != 1) {
        throw std::runtime_error("shard model must have exactly one fracture group: " + std::to_string(groups.size()));
    }
    const std::vector<FractureChunkEntry>& chunks = groups.begin()->second;
    if (chunks.empty()) {
        throw std::runtime_error("shard model has no fracture chunks");
    }

    set.model = model;
    set.groupName = groups.begin()->first;
    set.originalSrvIndex = model->GetMaterial(chunks.front().materialName)->GetMaterialSrvIndex();

    // 全破片のバウンディングから、中心と大きさを求める（破片の数を減らしても形の基準を変えない）
    Vector3 minPoint = chunks.front().info.aabb.min;
    Vector3 maxPoint = chunks.front().info.aabb.max;
    for (const FractureChunkEntry& chunk : chunks) {
        const AABB& aabb = chunk.info.aabb;
        minPoint = { (std::min)(minPoint.x, aabb.min.x), (std::min)(minPoint.y, aabb.min.y), (std::min)(minPoint.z, aabb.min.z) };
        maxPoint = { (std::max)(maxPoint.x, aabb.max.x), (std::max)(maxPoint.y, aabb.max.y), (std::max)(maxPoint.z, aabb.max.z) };
    }
    set.center = (minPoint + maxPoint) * 0.5f;
    Vector3 extent = (maxPoint - minPoint) * 0.5f;
    set.halfExtent = (std::max)({ extent.x, extent.y, extent.z, 0.0001f });

    // 破片は上限まで。多いモデルでは等間隔に選ぶ（15個のモデルならすべて使う）
    size_t count = (std::min)(chunks.size(), kMaxChunks);
    for (size_t i = 0; i < count; ++i) {
        const FractureChunkEntry& chunk = chunks[i * chunks.size() / count];
        set.chunkIds.push_back(chunk.info.chunkId);
        set.centroids.push_back(chunk.info.centroid);
    }

    // 破片の描画は氷用なので、氷マテリアルをモデルのバッファへ設定する
    PackedGeometryBuffer& buffer = *model->GetFractureBuffers().at(set.groupName);
    buffer.SetBufferMaterial(set.material.GetMaterialSrvIndex(), static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));
}

void ShatterEffectManager::AllocateSlots() {
    slots_.reserve(kSlotCount);
    for (int32_t i = 0; i < kSlotCount; ++i) {
        std::unique_ptr<Slot> slot = std::make_unique<Slot>();

        // バッファの確保はここで一度だけ行う。以後は状態だけを書き換えて使い回す
        slot->shell.Initialize(shellSet_.chunkIds, *shellSet_.model->GetFractureBuffers().at(shellSet_.groupName));

        // 回転は各破片の重心を中心に行う
        for (size_t c = 0; c < shellSet_.centroids.size(); ++c) {
            slot->shell.SetPivot(static_cast<uint32_t>(c), shellSet_.centroids[c], 0.0f);
        }
        slots_.push_back(std::move(slot));
    }
}

void ShatterEffectManager::LaunchShards(FractureInstance& instance, const ShardSet& set, const Vector3& position, float radius) {
    // モデルを大きさradiusの球に収めたうえで、破片のサイズ倍率を掛け、中心を砕けた位置へ合わせる
    float scale = radius * shardSizeRatio_ / set.halfExtent;
    Vector3 translate = { position.x - set.center.x * scale, radius - set.center.y * scale, position.z - set.center.z * scale };
    instance.SetParentWorldMatrix(Math::MakeWorldMatrixFromEulerRotation(translate, { 0.0f, 0.0f, 0.0f }, { scale, scale, scale }));

    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    for (FractureChunkState& state : instance.GetTransformDatas()) {
        // 球面上のランダムな方向。地面へ潜らないよう、上向き側に寄せる
        float vertical = std::abs(RandomGenerator::Get(-1.0f, 1.0f));
        float angle = RandomGenerator::Get(0.0f, kTwoPi);
        float horizontal = std::sqrt(1.0f - vertical * vertical);
        Vector3 direction = { horizontal * std::cos(angle), vertical, horizontal * std::sin(angle) };

        // 速度はワールド単位で決め、親行列の拡大を打ち消してモデル座標へ直す
        float speed = RandomGenerator::Get(minLaunchSpeed_, (std::max)(maxLaunchSpeed_, minLaunchSpeed_));
        state.velocity = direction * (speed / scale);
        state.transform.translate = { 0.0f, 0.0f, 0.0f };
        state.transform.rotate = { 0.0f, 0.0f, 0.0f };
        state.transform.scale = { 1.0f, 1.0f, 1.0f };
        state.angularVelocity = {
            RandomGenerator::Get(-maxAngularSpeed_, maxAngularSpeed_),
            RandomGenerator::Get(-maxAngularSpeed_, maxAngularSpeed_),
            RandomGenerator::Get(-maxAngularSpeed_, maxAngularSpeed_) };
    }
    instance.Update();
}

ShatterEffectManager::Slot& ShatterEffectManager::AcquireSlot() {
    Slot* oldest = slots_.front().get();
    for (std::unique_ptr<Slot>& slot : slots_) {
        if (!slot->isActive) {
            return *slot;
        }
        if (slot->age > oldest->age) {
            oldest = slot.get();
        }
    }
    return *oldest;
}

void ShatterEffectManager::Spawn(const Vector3& position, float shellRadius) {
    Slot& slot = AcquireSlot();
    slot.isActive = true;
    slot.age = 0.0f;

    LaunchShards(slot.shell, shellSet_, position, shellRadius);
}

void ShatterEffectManager::Update() {
    if (debugParam_->ApplyIfDirty()) {
        shellSet_.material.GetMaterialData()->color = shellColor_;
    }

    if (isBenchmarking_) {
        UpdateBenchmark();
    }

    // 計測中は、破片の更新にかかったCPU時間を集める
    std::chrono::steady_clock::time_point updateStart = std::chrono::steady_clock::now();
    float deltaTime = FpsCounter::GetGameDeltaTime();

    // 使用中のスロットを集める
    activeSlots_.clear();
    for (std::unique_ptr<Slot>& slot : slots_) {
        if (slot->isActive) {
            activeSlots_.push_back(slot.get());
        }
    }

    // 各スロットは自分の破片だけを書き換えるので、数が多いときは並列に更新する（少ないときは分配のコストが勝つので直列）
    if (parallelThreshold_ > 0 && static_cast<int32_t>(activeSlots_.size()) >= parallelThreshold_) {
        std::for_each(std::execution::par, activeSlots_.begin(), activeSlots_.end(), [this, deltaTime](Slot* slot) {
            UpdateSlot(*slot, deltaTime);
        });
    } else {
        for (Slot* slot : activeSlots_) {
            UpdateSlot(*slot, deltaTime);
        }
    }
    if (isBenchmarking_ && benchmarkTimer_ >= kBenchmarkWarmup) {
        benchmarkUpdateMs_.push_back(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - updateStart).count());
    }
}

void ShatterEffectManager::UpdateSlot(Slot& slot, float deltaTime) {
    slot.age += deltaTime;
    if (slot.age >= lifetime_) {
        slot.isActive = false;
        return;
    }

    // 寿命の後半は、イージングで縮めて消す
    float ratio = slot.age / (std::max)(lifetime_, 0.01f);
    float fadeScale = 1.0f;
    if (ratio > fadeStartRatio_) {
        float t = (ratio - fadeStartRatio_) / (std::max)(1.0f - fadeStartRatio_, 0.01f);
        fadeScale = (std::max)(1.0f - t * t, 0.001f);
    }

    UpdateShards(slot.shell, deltaTime, fadeScale);
}

void ShatterEffectManager::UpdateShards(FractureInstance& instance, float deltaTime, float fadeScale) {
    for (FractureChunkState& state : instance.GetTransformDatas()) {
        state.transform.translate += state.velocity * deltaTime;
        state.transform.rotate += state.angularVelocity * deltaTime;
        state.transform.scale = { fadeScale, fadeScale, fadeScale };
    }
    instance.Update();
}

void ShatterEffectManager::Draw() {
    // 計測中は、描画登録にかかったCPU時間を集める
    std::chrono::steady_clock::time_point drawStart = std::chrono::steady_clock::now();
    for (std::unique_ptr<Slot>& slot : slots_) {
        if (!slot->isActive) {
            continue;
        }
        renderQueue_->SubmitRaytracingFracture(shellSet_.model, slot->shell, identityTransform_);
    }
    if (isBenchmarking_ && benchmarkTimer_ >= kBenchmarkWarmup) {
        benchmarkDrawMs_.push_back(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - drawStart).count());
    }
}

void ShatterEffectManager::StartBenchmark(int32_t count) {
    if (isBenchmarking_) {
        return;
    }
    if (count > 0) {
        benchmarkCount_ = count;
    }
    isBenchmarking_ = true;
    benchmarkTimer_ = 0.0f;
    benchmarkFrames_.clear();
    benchmarkUpdateMs_.clear();
    benchmarkDrawMs_.clear();
    benchmarkSpawnMs_.clear();
    lastFrameTime_ = std::chrono::steady_clock::now();
}

void ShatterEffectManager::UpdateBenchmark() {
    // 実時間でフレームの間隔を測る
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    float realDeltaTime = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;

    benchmarkTimer_ += realDeltaTime;

    // 開始直後は読み込みなどで揺れるため集計しない
    if (benchmarkTimer_ >= kBenchmarkWarmup) {
        benchmarkFrames_.push_back(realDeltaTime * 1000.0f);
    }

    if (benchmarkTimer_ >= kBenchmarkWarmup + benchmarkSeconds_) {
        isBenchmarking_ = false;
        WriteBenchmarkLog();
        return;
    }

    // 指定数が常に砕けている状態を保つ（寿命が尽きたら同じ位置の範囲に補充する）
    int32_t activeCount = 0;
    for (std::unique_ptr<Slot>& slot : slots_) {
        if (slot->isActive) {
            ++activeCount;
        }
    }
    int32_t target = (std::min)(benchmarkCount_, kSlotCount);
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    std::chrono::steady_clock::time_point spawnStart = std::chrono::steady_clock::now();
    int32_t spawned = 0;
    for (; activeCount < target; ++activeCount, ++spawned) {
        float angle = RandomGenerator::Get(0.0f, kTwoPi);
        float distance = benchmarkSpread_ * std::sqrt(RandomGenerator::Get(0.0f, 1.0f));
        Spawn({ std::cos(angle) * distance, 0.0f, std::sin(angle) * distance }, 0.6f);
    }
    if (spawned > 0 && benchmarkTimer_ >= kBenchmarkWarmup) {
        benchmarkSpawnMs_.push_back(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - spawnStart).count());
    }
}

void ShatterEffectManager::WriteBenchmarkLog() const {
    if (benchmarkFrames_.empty()) {
        return;
    }

    float total = 0.0f;
    float worst = 0.0f;
    for (float frame : benchmarkFrames_) {
        total += frame;
        worst = (std::max)(worst, frame);
    }
    float average = total / static_cast<float>(benchmarkFrames_.size());

    // CPU内訳の平均（ミリ秒）
    float updateTotal = 0.0f;
    for (float ms : benchmarkUpdateMs_) { updateTotal += ms; }
    float drawTotal = 0.0f;
    for (float ms : benchmarkDrawMs_) { drawTotal += ms; }
    float spawnMax = 0.0f;
    for (float ms : benchmarkSpawnMs_) { spawnMax = (std::max)(spawnMax, ms); }
    float updateAverage = benchmarkUpdateMs_.empty() ? 0.0f : updateTotal / static_cast<float>(benchmarkUpdateMs_.size());
    float drawAverage = benchmarkDrawMs_.empty() ? 0.0f : drawTotal / static_cast<float>(benchmarkDrawMs_.size());

#ifdef _DEBUG
    const char* config = "Debug";
#else
    const char* config = "Release or Development (NDEBUG)";
#endif

    std::ofstream file(kBenchmarkLogPath, std::ios::app);
    file << "config=" << config
         << " shatterCount=" << (std::min)(benchmarkCount_, kSlotCount)
         << " chunksPerShell=" << shellSet_.chunkIds.size()
         << " frames=" << benchmarkFrames_.size()
         << " averageMs=" << average
         << " worstMs=" << worst
         << " averageFps=" << (1000.0f / average)
         << " cpuUpdateAvgMs=" << updateAverage
         << " cpuDrawAvgMs=" << drawAverage
         << " cpuSpawnMaxMs=" << spawnMax
         << " sizeRatio=" << shardSizeRatio_ << "\n";
}
