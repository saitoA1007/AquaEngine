#include"ParticleBehavior.h"
#include"FPSCounter.h"
#include"RandomGenerator.h"
#include"GameParamEditor.h"
#include"MyMath.h"
using namespace GameEngine;

void ParticleBehavior::Initialize(const std::string& name,uint32_t maxNum, uint32_t textureHandle) {
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
    // 登録
    int index = 0;
    std::string subGroup = "Emitter";
    debugParame_->Register("SpawnMaxCount", particleEmitter_.spawnMaxCount, index++, subGroup);
    debugParame_->Register("SpawnCoolTime", particleEmitter_.spawnCoolTime, index++, subGroup);
    debugParame_->Register("IsLoop", particleEmitter_.isLoop, index++, subGroup);
    debugParame_->Register("IsBillBoard", particleEmitter_.isBillBoard, index++, subGroup);
    subGroup = "Particle";
    index = 0;
    debugParame_->Register("LifeTime", particleEmitter_.lifeTime, index++, subGroup);
    debugParame_->Register("FieldAcceleration", particleEmitter_.fieldAcceleration, index++, subGroup);
    debugParame_->Register("VelocityRange", particleEmitter_.velocityRange, index++, subGroup);
    debugParame_->Register("SpawnRange", particleEmitter_.posRange, index++, subGroup);
    debugParame_->Register("ScaleRange", particleEmitter_.scaleRange, index++, subGroup);
    debugParame_->Register("ColorRange", particleEmitter_.colorRange, index++, subGroup);

    // 出現範囲を抑える
    if (maxNumInstance_ <= particleEmitter_.spawnMaxCount) {
        particleEmitter_.spawnMaxCount = maxNumInstance_;
    }

    // 値の適応
    debugParame_->Apply();
}

void ParticleBehavior::Update(const Matrix4x4& cameraMatrix) {
    // 値の適応
    debugParame_->ApplyIfDirty();

    // 出現範囲を抑える
    if (maxNumInstance_ <= particleEmitter_.spawnMaxCount) {
        particleEmitter_.spawnMaxCount = maxNumInstance_;
    }

    // パーティクルの発生を管理する
    if (particleEmitter_.isLoop) {
        Create();
    }

    // 移動処理
    Move(cameraMatrix);
}

void ParticleBehavior::Emit(const Vector3& pos) {
    emitterPos_ = pos;

    if (!particleEmitter_.isLoop) {
        spawnTimer_ = particleEmitter_.spawnCoolTime;
        // 生成する
        Create();
    }
}

ParticleData ParticleBehavior::MakeNewParticle() {

    ParticleData tmpParticleData;
    // srtを設定
    float scale = RandomGenerator::Get(particleEmitter_.scaleRange.min.x, particleEmitter_.scaleRange.max.x);
    tmpParticleData.transform.scale = { scale ,scale ,scale };
    tmpParticleData.transform.rotate = {0.0f,0.0f,0.0f};
    tmpParticleData.transform.translate = {
    RandomGenerator::Get(particleEmitter_.posRange.min.x, particleEmitter_.posRange.max.x),
    RandomGenerator::Get(particleEmitter_.posRange.min.y, particleEmitter_.posRange.max.y), 
    RandomGenerator::Get(particleEmitter_.posRange.min.z, particleEmitter_.posRange.max.z),
    };
    tmpParticleData.transform.translate += emitterPos_;
    // 速度
    tmpParticleData.velocity = {
    RandomGenerator::Get(particleEmitter_.velocityRange.min.x, particleEmitter_.velocityRange.max.x),
    RandomGenerator::Get(particleEmitter_.velocityRange.min.y, particleEmitter_.velocityRange.max.y),
    RandomGenerator::Get(particleEmitter_.velocityRange.min.z, particleEmitter_.velocityRange.max.z), 
    };
    // 色
    tmpParticleData.color = { 
    RandomGenerator::Get(particleEmitter_.colorRange.min.x, particleEmitter_.colorRange.max.x),
    RandomGenerator::Get(particleEmitter_.colorRange.min.y, particleEmitter_.colorRange.max.y),
    RandomGenerator::Get(particleEmitter_.colorRange.min.z, particleEmitter_.colorRange.max.z), 
    RandomGenerator::Get(particleEmitter_.colorRange.min.w, particleEmitter_.colorRange.max.w),
    };
    // 生存時間
    tmpParticleData.currentTime = 0.0f;
    tmpParticleData.lifeTime = particleEmitter_.lifeTime;
    return tmpParticleData;
}

void ParticleBehavior::Create() {

    // 経過時間を加算
    spawnTimer_ += FpsCounter::deltaTime;

    if (spawnTimer_ >= particleEmitter_.spawnCoolTime) {
        uint32_t spawnCount = 0;
        for (uint32_t i = 0; i < maxNumInstance_; ++i) {
            // 時間が過ぎていれば新しく生成する
            if (particles_[i].lifeTime <= particles_[i].currentTime) {
                particles_[i] = MakeNewParticle();
                spawnCount++;
            }
            // 指定した数発生させたら終了
            if (spawnCount >= particleEmitter_.spawnMaxCount || spawnCount >= maxNumInstance_) {
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
        if (particle.lifeTime <= particle.currentTime) {
            continue;
        }
        // 経過時間を加算
        particle.currentTime += FpsCounter::deltaTime;
        // 速度を追加
        particle.velocity += particleEmitter_.fieldAcceleration * FpsCounter::deltaTime;
        particle.transform.translate += particle.velocity * FpsCounter::deltaTime;

        // worldTransformsの更新
        if (particleEmitter_.isBillBoard) {
            // ビルボードを適応する
            worldTransforms_->transformDatas_[currentNumInstance_].worldMatrix = Math::MakeBillboardMatrix(particles_[i].transform.scale, particles_[i].transform.translate, cameraMatrix);
        } else {
            worldTransforms_->transformDatas_[currentNumInstance_].transform = particles_[i].transform;
        }

        worldTransforms_->transformDatas_[currentNumInstance_].color = particles_[i].color;
        worldTransforms_->transformDatas_[currentNumInstance_].textureHandle = textureHandle_;
        currentNumInstance_++;
    }

    // 行列の更新処理
    if (!particleEmitter_.isBillBoard) {
        worldTransforms_->UpdateTransformMatrix(currentNumInstance_);
    }
}