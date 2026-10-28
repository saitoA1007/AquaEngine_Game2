#include "pch.h"
#include "RandomGenerator.h"

std::mt19937 RandomGenerator::randomEngine_;

void RandomGenerator::Initialize() {
    std::random_device rd;
    randomEngine_ = std::mt19937(rd());
}

// Vector3 専用
Vector3 RandomGenerator::GetVector3(float min, float max) {
    return Vector3(
        Get<float>(min, max),
        Get<float>(min, max),
        Get<float>(min, max)
    );
}

Vector3 RandomGenerator::GetVector3(Vector3 min, Vector3 max) {
    return Vector3(
        Get<float>(min.x, max.x),
        Get<float>(min.y, max.y),
        Get<float>(min.z, max.z)
    );
}