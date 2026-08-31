#include "ParticleBehavior.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "ParticleEmitModules.h"
using namespace GameEngine;

ParticleBehavior::ParticleBehavior(const std::string& name, uint32_t maxNum, TextureManager* textureManager, Model* model, Camera* camera) {
    maxNumInstance_ = maxNum;
    name_ = name;
    camera_ = camera;
    model_ = model;

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
    debugParame_->Register("LifeTime", main_.lifeTime, index++, subGroup);
    debugParame_->Register("IsLoop", main_.isLoop, index++, subGroup);
    debugParame_->Register("IsBillBoard", main_.isBillBoard, index++, subGroup);
    subGroup += "/Defalut";
    debugParame_->Register("EmittePos", main_.emitterPos, index++, subGroup);
    debugParame_->Register("Rotate", main_.rotate, index++, subGroup);
    debugParame_->Register("Scale", main_.scale, index++, subGroup);

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

    // 移動処理
    Move(camera_->GetWorldMatrix());
}

void ParticleBehavior::Draw() {
    renderQueue_->SubmitInstancing(model_, currentNumInstance_, *worldTransforms_, 0.0f, BlendMode::kBlendModeAdd);
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
    tmpParticleData.color = { 1.0f,1.0f,1.0f,1.0f };
    tmpParticleData.startColor = tmpParticleData.color;
    tmpParticleData.startSize =  main_.scale;
    tmpParticleData.startSpeed = tmpParticleData.velocity;
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

        // worldTransformsの更新
        if (main_.isBillBoard) {
            // ビルボードを適応する
            if (isRotateVelocity) {
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeDirectionalBillboardMatrix(particle.transform.scale, particle.transform.translate, cameraMatrix, camera_->GetViewMatrix(), particle.velocity, particle.transform.rotate.z);
            } else {
                worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeBillboardMatrix(particle.transform.scale, particle.transform.translate, particle.transform.translate.z,cameraMatrix);
            }
        } else {
            worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
        }

        worldTransforms_->transformDatas_[currentNumInstance_].color = particle.color;
        worldTransforms_->transformDatas_[currentNumInstance_].textureHandle = particle.textureHandle;
        currentNumInstance_++;
    }

    // 行列の更新処理
    if (!main_.isBillBoard) {
        worldTransforms_->UpdateTransformMatrix(currentNumInstance_);
    }
}
