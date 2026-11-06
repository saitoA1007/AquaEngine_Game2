#include "pch.h"
#include "EnemyGrid.h"
#include <algorithm>
#include <cmath>

void EnemyGrid::Initialize(float halfExtent, float cellSize) {
    halfExtent_ = (std::max)(halfExtent, 1.0f);
    cellSize_ = (std::max)(cellSize, 0.1f);
    cellCount_ = (std::max)(static_cast<int32_t>(std::ceil(halfExtent_ * 2.0f / cellSize_)), 1);

    int32_t totalCells = cellCount_ * cellCount_;
    cellStart_.assign(static_cast<size_t>(totalCells) + 1, 0);
    cursor_.assign(static_cast<size_t>(totalCells), 0);
}

int32_t EnemyGrid::ToCellCoord(float value) const {
    int32_t coord = static_cast<int32_t>(std::floor((value + halfExtent_) / cellSize_));
    return (std::clamp)(coord, 0, cellCount_ - 1);
}

void EnemyGrid::Rebuild(const std::vector<Enemy*>& enemies) {
    int32_t totalCells = cellCount_ * cellCount_;
    std::fill(cellStart_.begin(), cellStart_.end(), 0);
    entryCell_.resize(enemies.size());
    sorted_.resize(enemies.size());

    // セルごとの個数を数える
    for (size_t i = 0; i < enemies.size(); ++i) {
        const Vector3& position = enemies[i]->GetPosition();
        int32_t cell = ToCellCoord(position.z) * cellCount_ + ToCellCoord(position.x);
        entryCell_[i] = cell;
        ++cellStart_[static_cast<size_t>(cell) + 1];
    }

    // 個数を累積して、セルごとの先頭位置にする
    for (int32_t cell = 0; cell < totalCells; ++cell) {
        cellStart_[static_cast<size_t>(cell) + 1] += cellStart_[cell];
        cursor_[cell] = cellStart_[cell];
    }

    // セル順に並べる
    for (size_t i = 0; i < enemies.size(); ++i) {
        sorted_[cursor_[entryCell_[i]]++] = enemies[i];
    }
}

void EnemyGrid::Query(const Vector3& center, float radius, std::vector<Enemy*>& out) const {
    out.clear();

    int32_t minX = ToCellCoord(center.x - radius);
    int32_t maxX = ToCellCoord(center.x + radius);
    int32_t minZ = ToCellCoord(center.z - radius);
    int32_t maxZ = ToCellCoord(center.z + radius);
    float radiusSquared = radius * radius;

    for (int32_t z = minZ; z <= maxZ; ++z) {
        for (int32_t x = minX; x <= maxX; ++x) {
            int32_t cell = z * cellCount_ + x;
            for (int32_t i = cellStart_[cell]; i < cellStart_[static_cast<size_t>(cell) + 1]; ++i) {
                Enemy* enemy = sorted_[i];
                float dx = enemy->GetPosition().x - center.x;
                float dz = enemy->GetPosition().z - center.z;
                if (dx * dx + dz * dz <= radiusSquared) {
                    out.push_back(enemy);
                }
            }
        }
    }
}
