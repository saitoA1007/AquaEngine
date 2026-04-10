#pragma once
#include"Vector4.h"
#include"Vector3.h"
#include"Vector2.h"
#include<cstdint>

// ヴィネット
struct alignas(16) VignettingData {
    float intensity; // ぼかさない円の範囲
    float time; // ぼかしぐわい
    uint32_t textureHandle; // 加工する画像
    float padding;
};

// スキャンライン
struct alignas(16) ScanLineData {
    float interval; // 間隔
    float time; // 時間
    float speed; // 速度
    float pad;
    Vector3 lineColor; // 線の色
    uint32_t textureHandle; // 加工する画像
};

// ラジアルブラー
struct alignas(16) RadialBlurData {
    Vector2 centerPos; // 中心点
    int32_t numSamles; // サンプリング数。大きい程滑らか
    float blurWidth; // ぼかしの幅
    uint32_t textureHandle; // 加工する画像
    float padding[3];
};