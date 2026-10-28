#include "pch.h"
#include "DebugRenderer.h"
#include <cassert>
#include "MyMath.h"
#include "LogManager.h"
using namespace GameEngine;

DebugRenderer::DebugRenderer() {
    Clear();

    // 大きな頂点バッファを事前に確保
    std::vector<VertexPosColor> vertices(maxVertices_);
    vertexBuffer_.Create(vertices);
    vertexData_ = vertexBuffer_.GetVertexData();
}

void DebugRenderer::Clear() {
    lines_.clear();
}

void DebugRenderer::AddLine(const Vector3& start, const Vector3& end, const Vector4& color) {
    lines_.push_back({ start, end, color });
}

void DebugRenderer::AddAABB(const AABB& aabb, const Vector4& color) {
    // 下面
    AddLine({ aabb.min.x, aabb.min.y, aabb.min.z }, { aabb.max.x, aabb.min.y, aabb.min.z }, color);
    AddLine({ aabb.max.x, aabb.min.y, aabb.min.z }, { aabb.max.x, aabb.min.y, aabb.max.z }, color);
    AddLine({ aabb.max.x, aabb.min.y, aabb.max.z }, { aabb.min.x, aabb.min.y, aabb.max.z }, color);
    AddLine({ aabb.min.x, aabb.min.y, aabb.max.z }, { aabb.min.x, aabb.min.y, aabb.min.z }, color);
    // 上面
    AddLine({ aabb.min.x, aabb.max.y, aabb.min.z }, { aabb.max.x, aabb.max.y, aabb.min.z }, color);
    AddLine({ aabb.max.x, aabb.max.y, aabb.min.z }, { aabb.max.x, aabb.max.y, aabb.max.z }, color);
    AddLine({ aabb.max.x, aabb.max.y, aabb.max.z }, { aabb.min.x, aabb.max.y, aabb.max.z }, color);
    AddLine({ aabb.min.x, aabb.max.y, aabb.max.z }, { aabb.min.x, aabb.max.y, aabb.min.z }, color);
    // 縦の線
    AddLine({ aabb.min.x, aabb.min.y, aabb.min.z }, { aabb.min.x, aabb.max.y, aabb.min.z }, color);
    AddLine({ aabb.max.x, aabb.min.y, aabb.min.z }, { aabb.max.x, aabb.max.y, aabb.min.z }, color);
    AddLine({ aabb.max.x, aabb.min.y, aabb.max.z }, { aabb.max.x, aabb.max.y, aabb.max.z }, color);
    AddLine({ aabb.min.x, aabb.min.y, aabb.max.z }, { aabb.min.x, aabb.max.y, aabb.max.z }, color);
}

void DebugRenderer::AddBox(const Vector3& centerPos, const Vector3& size, const Vector4& color) {
    Vector3 halfSize = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
    AABB resultAABB;
    resultAABB.min = { centerPos.x - halfSize.x, centerPos.y - halfSize.y, centerPos.z - halfSize.z };
    resultAABB.max = { centerPos.x + halfSize.x, centerPos.y + halfSize.y, centerPos.z + halfSize.z };
    AddAABB(resultAABB, color);
}

void DebugRenderer::AddOBB(const OBB& obb, const Vector4& color) {
    // スケールを適応した軸のベクトル
    Vector3 scaledX = {
        obb.orientations[0].x * obb.size.x,
        obb.orientations[0].y * obb.size.x,
        obb.orientations[0].z * obb.size.x
    };
    Vector3 scaledY = {
        obb.orientations[1].x * obb.size.y,
        obb.orientations[1].y * obb.size.y,
        obb.orientations[1].z * obb.size.y
    };
    Vector3 scaledZ = {
        obb.orientations[2].x * obb.size.z,
        obb.orientations[2].y * obb.size.z,
        obb.orientations[2].z * obb.size.z
    };

    // 頂点を計算
    Vector3 corners[8];
    // v0(-x, -y, -z)
    corners[0] = {
        obb.center.x - scaledX.x - scaledY.x - scaledZ.x,
        obb.center.y - scaledX.y - scaledY.y - scaledZ.y,
        obb.center.z - scaledX.z - scaledY.z - scaledZ.z
    };
    // v1(+x, -y, -z)
    corners[1] = {
        obb.center.x + scaledX.x - scaledY.x - scaledZ.x,
        obb.center.y + scaledX.y - scaledY.y - scaledZ.y,
        obb.center.z + scaledX.z - scaledY.z - scaledZ.z
    };
    // v2(-x, +y, -z)
    corners[2] = {
        obb.center.x - scaledX.x + scaledY.x - scaledZ.x,
        obb.center.y - scaledX.y + scaledY.y - scaledZ.y,
        obb.center.z - scaledX.z + scaledY.z - scaledZ.z
    };
    // v3(+x, +y, -z)
    corners[3] = {
        obb.center.x + scaledX.x + scaledY.x - scaledZ.x,
        obb.center.y + scaledX.y + scaledY.y - scaledZ.y,
        obb.center.z + scaledX.z + scaledY.z - scaledZ.z
    };
    // v4(-x, -y, +z)
    corners[4] = {
        obb.center.x - scaledX.x - scaledY.x + scaledZ.x,
        obb.center.y - scaledX.y - scaledY.y + scaledZ.y,
        obb.center.z - scaledX.z - scaledY.z + scaledZ.z
    };
    // v5(+x, -y, +z)
    corners[5] = {
        obb.center.x + scaledX.x - scaledY.x + scaledZ.x,
        obb.center.y + scaledX.y - scaledY.y + scaledZ.y,
        obb.center.z + scaledX.z - scaledY.z + scaledZ.z
    };
    // v6(-x, +y, +z)
    corners[6] = {
        obb.center.x - scaledX.x + scaledY.x + scaledZ.x,
        obb.center.y - scaledX.y + scaledY.y + scaledZ.y,
        obb.center.z - scaledX.z + scaledY.z + scaledZ.z
    };
    // v7(+x, +y, +z)
    corners[7] = {
        obb.center.x + scaledX.x + scaledY.x + scaledZ.x,
        obb.center.y + scaledX.y + scaledY.y + scaledZ.y,
        obb.center.z + scaledX.z + scaledY.z + scaledZ.z
    };

    // 下面
    AddLine(corners[0], corners[1], color);
    AddLine(corners[1], corners[5], color);
    AddLine(corners[5], corners[4], color);
    AddLine(corners[4], corners[0], color);
    // 上面
    AddLine(corners[2], corners[3], color);
    AddLine(corners[3], corners[7], color);
    AddLine(corners[7], corners[6], color);
    AddLine(corners[6], corners[2], color);
    // 縦の線
    AddLine(corners[0], corners[2], color);
    AddLine(corners[1], corners[3], color);
    AddLine(corners[5], corners[7], color);
    AddLine(corners[4], corners[6], color);
}

void DebugRenderer::AddSphere(const Sphere& sphere, const Vector4& color, int segments) {
    float angleStep = 2.0f * 3.14159265f / segments;

    // XY平面の円
    for (int i = 0; i < segments; ++i) {
        float angle1 = angleStep * i;
        float angle2 = angleStep * (i + 1);
        Vector3 p1 = {
            sphere.center.x + sphere.radius * std::cosf(angle1),
            sphere.center.y + sphere.radius * std::sinf(angle1),
            sphere.center.z
        };
        Vector3 p2 = {
            sphere.center.x + sphere.radius * std::cosf(angle2),
            sphere.center.y + sphere.radius * std::sinf(angle2),
            sphere.center.z
        };
        AddLine(p1, p2, color);
    }

    // YZ平面の円
    for (int i = 0; i < segments; ++i) {
        float angle1 = angleStep * i;
        float angle2 = angleStep * (i + 1);
        Vector3 p1 = {
            sphere.center.x,
            sphere.center.y + sphere.radius * std::cosf(angle1),
            sphere.center.z + sphere.radius * std::sinf(angle1)
        };
        Vector3 p2 = {
            sphere.center.x,
            sphere.center.y + sphere.radius * std::cosf(angle2),
            sphere.center.z + sphere.radius * std::sinf(angle2)
        };
        AddLine(p1, p2, color);
    }

    // XZ平面の円
    for (int i = 0; i < segments; ++i) {
        float angle1 = angleStep * i;
        float angle2 = angleStep * (i + 1);
        Vector3 p1 = {
            sphere.center.x + sphere.radius * std::cosf(angle1),
            sphere.center.y,
            sphere.center.z + sphere.radius * std::sinf(angle1)
        };
        Vector3 p2 = {
            sphere.center.x + sphere.radius * std::cosf(angle2),
            sphere.center.y,
            sphere.center.z + sphere.radius * std::sinf(angle2)
        };
        AddLine(p1, p2, color);
    }
}

void DebugRenderer::AddRay(const Segment segment, float length, const Vector4& color) {
    Vector3 end = {
        segment.origin.x + segment.diff.x * length,
        segment.origin.y + segment.diff.y * length,
        segment.origin.z + segment.diff.z * length
    };
    AddLine(segment.origin, end, color);
}

void DebugRenderer::AddCircle(const Vector3& centerPos, const Vector3& normal, float radius, const Vector4& color, int segments) {
    // 法線ベクトルから垂直な2つのベクトルを作成(簡易版)
    Vector3 tangent1, tangent2;
    if (std::abs(normal.y) < 0.9f) {
        tangent1 = { -normal.y, normal.x, 0 };
    } else {
        tangent1 = { 0, -normal.z, normal.y };
    }

    float len1 = std::sqrt(tangent1.x * tangent1.x + tangent1.y * tangent1.y + tangent1.z * tangent1.z);
    tangent1.x /= len1;
    tangent1.y /= len1;
    tangent1.z /= len1;

    // 外積でもう1つの垂直ベクトルを作成
    tangent2 = {
        normal.y * tangent1.z - normal.z * tangent1.y,
        normal.z * tangent1.x - normal.x * tangent1.z,
        normal.x * tangent1.y - normal.y * tangent1.x
    };

    float angleStep = 2.0f * 3.14159265f / segments;
    for (int i = 0; i < segments; ++i) {
        float angle1 = angleStep * i;
        float angle2 = angleStep * (i + 1);

        Vector3 p1 = {
            centerPos.x + (tangent1.x * std::cos(angle1) + tangent2.x * std::sin(angle1)) * radius,
            centerPos.y + (tangent1.y * std::cos(angle1) + tangent2.y * std::sin(angle1)) * radius,
            centerPos.z + (tangent1.z * std::cos(angle1) + tangent2.z * std::sin(angle1)) * radius
        };
        Vector3 p2 = {
            centerPos.x + (tangent1.x * std::cos(angle2) + tangent2.x * std::sin(angle2)) * radius,
            centerPos.y + (tangent1.y * std::cos(angle2) + tangent2.y * std::sin(angle2)) * radius,
            centerPos.z + (tangent1.z * std::cos(angle2) + tangent2.z * std::sin(angle2)) * radius
        };

        AddLine(p1, p2, color);
    }
}

void DebugRenderer::Update() {
    // 何も無ければ早期リターン
    if (!isEnabled_ || lines_.empty()) {
        return;
    }
    // すべての線の頂点データを1つのバッファにまとめる
    uint32_t vertexIndex = 0;
    uint32_t totalVertices = static_cast<uint32_t>(lines_.size() * 2); // 各線は2頂点

    // バッファサイズが足りない場合は警告
    if (totalVertices > maxVertices_) {
        assert(false);
        totalVertices = maxVertices_;
    }

    // すべての線の頂点をバッファに書き込む
    for (size_t i = 0; i < lines_.size() && vertexIndex < maxVertices_; ++i) {
        const auto& line = lines_[i];

        // 始点
        vertexData_[vertexIndex].pos = { line.start.x, line.start.y, line.start.z, 1.0f };
        vertexData_[vertexIndex].color = line.color;
        vertexIndex++;

        if (vertexIndex >= maxVertices_) break;

        // 終点
        vertexData_[vertexIndex].pos = { line.end.x, line.end.y, line.end.z, 1.0f };
        vertexData_[vertexIndex].color = line.color;
        vertexIndex++;
    }

    // 1回のDrawCallですべての線を描画
    totalVertices_ = static_cast<uint32_t>(lines_.size() * 2);
    if (totalVertices_ > maxVertices_) {
        totalVertices_ = maxVertices_;
    }
}