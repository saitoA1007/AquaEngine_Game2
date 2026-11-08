#include "pch.h"
#include "HpShardPile.h"
#include "RandomGenerator.h"
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <numbers>

void HpShardPile::Start(const std::vector<ShardShape>& shapes, float spinAngle, float innerApothem, const Settings& settings) {
    innerApothem_ = innerApothem;
    isActive_ = true;
    shards_.resize(shapes.size());

    // 核と同じ向きから始めるため、割れる前の位置を核の回転で回す（回転はY軸まわりだけなので、オイラー角のYと同じ）
    Matrix4x4 spin = Matrix4x4::MakeRotateYMatrix(spinAngle);
    for (size_t i = 0; i < shapes.size(); ++i) {
        const ShardShape& shape = shapes[i];
        Shard& shard = shards_[i];
        shard.position = Matrix4x4::Transform(shape.restCenter, spin);
        shard.rotate = { 0.0f, spinAngle, 0.0f };
        shard.radius = shape.size * settings.radiusScale;
        shard.isTouching = false;

        // 枠の面の上で、中心から外へ向かって飛ばし、少し上へ跳ねさせる
        Vector3 outward = { shard.position.x, shard.position.y, 0.0f };
        float length = outward.Length();
        if (length > 0.0001f) {
            outward = outward / length;
        } else {
            outward = { 0.0f, 1.0f, 0.0f };
        }
        shard.velocity = { outward.x * settings.burstSpeed, outward.y * settings.burstSpeed + settings.burstUp, 0.0f };

        shard.angularVelocity = {
            RandomGenerator::Get(-settings.maxAngularSpeed, settings.maxAngularSpeed),
            RandomGenerator::Get(-settings.maxAngularSpeed, settings.maxAngularSpeed),
            RandomGenerator::Get(-settings.maxAngularSpeed, settings.maxAngularSpeed) };
    }
}

void HpShardPile::Update(float deltaTime, const Settings& settings) {
    if (!isActive_ || deltaTime <= 0.0f) {
        return;
    }

    // 引っかかったフレームで一気に進めると枠をすり抜けるので、1回で進める時間を抑えてから分ける
    float clamped = (std::min)(deltaTime, kMaxDeltaTime);
    float subDelta = clamped / static_cast<float>(kSubSteps);
    for (int32_t step = 0; step < kSubSteps; ++step) {
        Step(subDelta, settings);
    }
}

void HpShardPile::Step(float deltaTime, const Settings& settings) {
    // 重力で落とし、奥行きを枠の面へ寄せ、回転を進める
    float depthRate = (std::min)(settings.depthSettle * deltaTime, 1.0f);
    float angularKeep = std::exp(-settings.angularDamping * deltaTime);
    for (Shard& shard : shards_) {
        shard.isTouching = false;
        shard.velocity.y -= settings.gravity * deltaTime;
        shard.position.x += shard.velocity.x * deltaTime;
        shard.position.y += shard.velocity.y * deltaTime;
        shard.position.z += (0.0f - shard.position.z) * depthRate;

        // 回転は見た目だけなので、オイラー角を一定の速さで進める（減衰で止まる）
        shard.angularVelocity = shard.angularVelocity * angularKeep;
        shard.rotate = shard.rotate + shard.angularVelocity * deltaTime;
    }

    // 積もったときに重なりが残らないよう、辺と欠片どうしの当たりを数回くり返して解く
    for (int32_t iteration = 0; iteration < kSolverIterations; ++iteration) {
        for (Shard& shard : shards_) {
            SolveWalls(shard, settings);
        }
        SolvePairs(settings);
    }

    // 何かに触れていて十分に遅い欠片は止め、細かい震えをなくす
    for (Shard& shard : shards_) {
        if (!shard.isTouching) {
            continue;
        }
        float speed = std::sqrt(shard.velocity.x * shard.velocity.x + shard.velocity.y * shard.velocity.y);
        if (speed < settings.sleepSpeed) {
            shard.velocity = { 0.0f, 0.0f, 0.0f };
            shard.angularVelocity = shard.angularVelocity * angularKeep;
        }
    }
}

void HpShardPile::SolveWalls(Shard& shard, const Settings& settings) const {
    // 頂点が上の六角形なので、辺の外向きの向きは0度から60度ごと
    constexpr float kStep = std::numbers::pi_v<float> / 3.0f;
    for (int32_t k = 0; k < 6; ++k) {
        float angle = kStep * static_cast<float>(k);
        float nx = std::cos(angle);
        float ny = std::sin(angle);

        // 円が辺からはみ出した分だけ押し戻す
        float penetration = shard.position.x * nx + shard.position.y * ny + shard.radius - innerApothem_;
        if (penetration <= 0.0f) {
            continue;
        }
        shard.position.x -= nx * penetration;
        shard.position.y -= ny * penetration;
        shard.isTouching = true;

        // 辺へ向かう速さは跳ね返し、辺に沿った速さは摩擦で減らす
        float normalSpeed = shard.velocity.x * nx + shard.velocity.y * ny;
        if (normalSpeed <= 0.0f) {
            continue;
        }
        float tangentX = shard.velocity.x - nx * normalSpeed;
        float tangentY = shard.velocity.y - ny * normalSpeed;
        float keep = 1.0f - (std::clamp)(settings.friction, 0.0f, 1.0f);
        shard.velocity.x = tangentX * keep - nx * normalSpeed * settings.restitution;
        shard.velocity.y = tangentY * keep - ny * normalSpeed * settings.restitution;
    }
}

void HpShardPile::SolvePairs(const Settings& settings) {
    for (size_t i = 0; i < shards_.size(); ++i) {
        for (size_t j = i + 1; j < shards_.size(); ++j) {
            Shard& a = shards_[i];
            Shard& b = shards_[j];
            float dx = b.position.x - a.position.x;
            float dy = b.position.y - a.position.y;
            float distanceSq = dx * dx + dy * dy;
            float minDistance = a.radius + b.radius;
            if (distanceSq >= minDistance * minDistance) {
                continue;
            }

            // 中心が重なったときは、向きを決められないので上下に分ける
            float distance = std::sqrt(distanceSq);
            float nx = 0.0f;
            float ny = 1.0f;
            if (distance > 0.0001f) {
                nx = dx / distance;
                ny = dy / distance;
            }

            // 重なりを半分ずつ押し戻す（欠片の重さは同じとみなす）
            float overlap = minDistance - distance;
            a.position.x -= nx * overlap * 0.5f;
            a.position.y -= ny * overlap * 0.5f;
            b.position.x += nx * overlap * 0.5f;
            b.position.y += ny * overlap * 0.5f;
            a.isTouching = true;
            b.isTouching = true;

            // 近づく速さだけを跳ね返す
            float approach = (b.velocity.x - a.velocity.x) * nx + (b.velocity.y - a.velocity.y) * ny;
            if (approach >= 0.0f) {
                continue;
            }
            float impulse = -(1.0f + settings.restitution) * approach * 0.5f;
            a.velocity.x -= nx * impulse;
            a.velocity.y -= ny * impulse;
            b.velocity.x += nx * impulse;
            b.velocity.y += ny * impulse;
        }
    }
}
