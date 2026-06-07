#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"

namespace GameEngine {
    // 前方宣言
    class DebugParameter;
}

// ボスの戦い中の状態
enum class BossBattleState {
    kNormal, // 状態の切り替えを管理

    kRushAttack,    // 突進攻撃
    kWindAttack,    // 風の発射
    kIceFallAttack, // 氷柱を落とす攻撃

    kWait,          // その場で留まる。攻撃と攻撃の小休憩
    kRotateMove,    // 回転して回る動き
    kCrossMove,     // 横断する動き

    kInMove,        // 最初の時に取る行動

    kMaxCount
};

// 戦いで使用する共通データ
struct BossBattleStateCommonData {
    Transform transform = { {1,1,1},{0,0,0},{0,0,0} };

    // 速度
    Vector3 velocity = { 0.0f,0.0f,0.0f };
    // 現在向いている方向
    Vector3 currentDir = { 0.0f,0.0f,1.0f };
    // 最終的に向く方向
    Vector3 targetDir = { 0.0f, 0.0f, 1.0f };
    // 現在の方向
    float currentYaw = 0.0f;

    // プレイヤーの位置
    const Vector3* playerPos;

    // ステージの半径
    float stageRadius = 10.0f;

    // 状態
    BossBattleState state = BossBattleState::kNormal;
};

/// <summary>
/// ボスのバトル中の行動の基底クラス
/// </summary>
class IBossBattleAction {
public:
    IBossBattleAction(BossBattleStateCommonData& commonData) : commonData_(commonData) {}
    virtual ~IBossBattleAction() = default;

    /// <summary>
    /// 攻撃の初期化
    /// </summary>
    virtual void Initialize() = 0;

    /// <summary>
    /// 攻撃の更新処理
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 攻撃の終了処理
    /// </summary>
    virtual void Finalize() = 0;

    /// <summary>
    /// 値を登録する
    /// </summary>
    virtual void RegisterParameter([[maybe_unused]] GameEngine::DebugParameter* param) {};

    /// <summary>
    /// 攻撃が完了したかどうか
    /// </summary>
    bool IsFinished() const { return isFinished_; }

protected:
    // 共通データ
    BossBattleStateCommonData& commonData_;

    bool isFinished_ = false;
};