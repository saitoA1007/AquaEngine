#include "ParticleBehavior.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "ParticleEmitModules.h"
using namespace GameEngine;

namespace {

    /// <summary>
    /// 位置に行列を掛ける（平行移動あり）
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
    /// 方向ベクトルに行列を掛ける（平行移動なし）
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
    // 値の適応
    debugParame_->ApplyIfDirty();

    // モジュールの更新
    modulesControl_->Update();

    // 出現範囲を抑える
    if (maxNumInstance_ <= main_.spawnMaxCount) {
        main_.spawnMaxCount = maxNumInstance_;
    }

    // パーティクルの発生を管理する
    if (main_.isLoop) {
        Create();
    }

    Matrix4x4 cameraMatrix = camera_->GetWorldMatrix();
    if (renderQueue_->GetUseDebugCamera()) {
        cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();
    }
    // 移動処理
    Move(cameraMatrix);
}

void ParticleBehavior::Draw() {

    if (main_.isActiveBlendAdd_) {
        renderQueue_->SubmitInstancing(model_, currentNumInstance_, *worldTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, "WBOITAccumulatePass");
    } else {
        renderQueue_->SubmitInstancing(model_, currentNumInstance_, *worldTransforms_, 0.0f, BlendMode::kBlendModeNormal, nullptr, "WBOITAccumulatePass");
    }
}

void ParticleBehavior::Emit(const Vector3& pos) {
    emitterPos_ = pos;

    if (!main_.isLoop) {
        spawnTimer_ = main_.spawnCoolTime;
        // 生成する
        Create();
    }
}

ParticleData ParticleBehavior::MakeNewParticle() {

    ParticleData tmpParticleData;
    tmpParticleData.transform.translate = main_.emitterPos;
    tmpParticleData.transform.scale = main_.scale;
    tmpParticleData.transform.rotate = main_.rotate;
    tmpParticleData.velocity = { 0.0f,0.0f,0.0f };
    tmpParticleData.color = main_.color;
    tmpParticleData.startColor = tmpParticleData.color;
    tmpParticleData.startSize =  main_.scale;
    tmpParticleData.startSpeed = tmpParticleData.velocity;
    tmpParticleData.rotateVelocity = { 0.0f,0.0f,0.0f };
    tmpParticleData.dir = { 0.0f,0.0f,0.0f };

    // 生存時間
    tmpParticleData.currentTime = 0.0f;
    tmpParticleData.lifeTime = main_.lifeTime;

    // モジュールを適応
    modulesControl_->ParticleCreate(tmpParticleData);

    if (isSetEmitPos_) {
        tmpParticleData.transform.translate += emitterPos_;
    }

    return tmpParticleData;
}

void ParticleBehavior::Create() {

    // 経過時間を加算
    spawnTimer_ += FpsCounter::deltaTime;

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

void ParticleBehavior::Move(const Matrix4x4& cameraMatrix) {
    currentNumInstance_ = 0;
    RotationByVelocityModule* module = modulesControl_->GetModule<RotationByVelocityModule>("RotationByVelocity");
    bool isRotateVelocity = false;
    if (module != nullptr) {
        isRotateVelocity = true;
    }

    // 常に親に追従するか（ローカル空間のときのみ毎フレーム親行列を掛ける）
    const bool isFollowParent = IsFollowParent();

    for (uint32_t i = 0; i < maxNumInstance_; ++i) {
        ParticleData& particle = particles_[i];

        // 生存期間を過ぎたら描画対象にしない
        if (particle.IsAlive()) {
            continue;
        }

        // 更新
        modulesControl_->ParticleUpdate(particle, FpsCounter::deltaTime);

        // 経過時間を加算
        particle.currentTime += FpsCounter::deltaTime / particle.lifeTime;
        // 速度を追加
        particle.transform.translate += particle.velocity * FpsCounter::deltaTime;
        // 回転速度
        particle.transform.rotate += particle.rotateVelocity * FpsCounter::deltaTime;

        // worldTransformsの更新
        if (main_.isBillBoard) {

            // ビルボードは親の回転を掛けてしまうとカメラを向かなくなるので
           // 「位置（と進行方向）だけ」を親空間からワールドへ変換して使う
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
                Matrix4x4 localMatrix = Math::MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = localMatrix * (*parentMatrix_);
                // 後段のUpdateTransformMatrixで上書きされないように、transformも同期しておく
                worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
            } else {
                worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
            }
            //worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
        }

        worldTransforms_->transformDatas_[currentNumInstance_].color = particle.color;
        worldTransforms_->transformDatas_[currentNumInstance_].textureHandle = particle.textureHandle;
        currentNumInstance_++;
    }

    // 行列の更新処理
    if (!main_.isBillBoard && !isFollowParent) {
        worldTransforms_->UpdateTransformMatrix(currentNumInstance_);
    }
}
