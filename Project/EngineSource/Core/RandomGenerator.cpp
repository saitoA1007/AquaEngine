#include "RandomGenerator.h"

std::mt19937 RandomGenerator::randomEngine_;

void RandomGenerator::Initialize() {
    std::random_device rd;
    randomEngine_ = std::mt19937(rd());
}

// 汎用テンプレート
template <typename T>
T RandomGenerator::Get(T min, T max) {
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(randomEngine_);
    } else if constexpr (std::is_floating_point_v<T>) {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(randomEngine_);
    } else {
        static_assert(std::is_arithmetic_v<T>, "RandomGenerator::Get は数値型のみ対応しています");
    }
}

// Vector3 専用
Vector3 RandomGenerator::GetVector3(float min, float max) {
    return Vector3(
        Get<float>(min, max),
        Get<float>(min, max),
        Get<float>(min, max)
    );
}