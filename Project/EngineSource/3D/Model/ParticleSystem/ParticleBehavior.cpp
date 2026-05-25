#include "ParticleBehavior.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"
#include "MyMath.h"
using namespace GameEngine;

ParticleBehavior::ParticleBehavior(const std::string& name, uint32_t maxNum, uint32_t textureHandle) {
    maxNumInstance_ = maxNum;
    textureHandle_ = textureHandle;
    name_ = name;

    // パーティクル配列を確保
    particles_.resize(maxNumInstance_);

    // WorldTransformsを初期化
    worldTransforms_ = std::make_unique<WorldTransforms>();
    Transform defaultTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    worldTransforms_->Initialize(maxNumInstance_, defaultTransform);

    // 全パーティクルを非アクティブ化
    for (auto& particle : particles_) {
        particle.currentTime = 0.0f;
        particle.lifeTime = 0.0f;
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

    //subGroup = "Particle";
    //index = 0;

    // 出現範囲を抑える
    if (maxNumInstance_ <= main_.spawnMaxCount) {
        main_.spawnMaxCount = maxNumInstance_;
    }

    // 値の適応
    debugParame_->Apply();

}

void ParticleBehavior::Initialize() {


    
}

void ParticleBehavior::Update(const Matrix4x4& cameraMatrix) {
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
    Move(cameraMatrix);
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

    // 生存時間
    tmpParticleData.currentTime = 0.0f;
    tmpParticleData.lifeTime = main_.lifeTime;

    // モジュールを適応
    modulesControl_->ParticleCreate(tmpParticleData);

    return tmpParticleData;
}

void ParticleBehavior::Create() {

    // 経過時間を加算
    spawnTimer_ += FpsCounter::deltaTime;

    if (spawnTimer_ >= main_.spawnCoolTime) {
        uint32_t spawnCount = 0;
        for (uint32_t i = 0; i < maxNumInstance_; ++i) {
            // 時間が過ぎていれば新しく生成する
            if (particles_[i].lifeTime <= particles_[i].currentTime) {
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
    for (uint32_t i = 0; i < maxNumInstance_; ++i) {
        ParticleData& particle = particles_[i];

        // 生存期間を過ぎたら描画対象にしない
        if (particle.IsAlive()) {
            continue;
        }

        // 更新
        modulesControl_->ParticleUpdate(particle, FpsCounter::deltaTime);

        // 経過時間を加算
        particle.currentTime += FpsCounter::deltaTime;
        // 速度を追加
        //particle.velocity += particleEmitter_.fieldAcceleration * FpsCounter::deltaTime;
        particle.transform.translate += particle.velocity * FpsCounter::deltaTime;

        // worldTransformsの更新
        if (main_.isBillBoard) {
            // ビルボードを適応する
            worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeBillboardMatrix(particle.transform.scale, particle.transform.translate, cameraMatrix);
        } else {
            worldTransforms_->transformDatas_[currentNumInstance_].transform = particle.transform;
        }

        worldTransforms_->transformDatas_[currentNumInstance_].color = particle.color;
        worldTransforms_->transformDatas_[currentNumInstance_].textureHandle = textureHandle_;
        currentNumInstance_++;
    }

    // 行列の更新処理
    if (!main_.isBillBoard) {
        worldTransforms_->UpdateTransformMatrix(currentNumInstance_);
    }
}