#define NOMINMAX
#include "DestructibleObject.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "LogManager.h"
using namespace GameEngine;

DestructibleObject::DestructibleObject(Model* model, uint32_t colliderId, uint32_t colliderAttribute) {

	model_ = model;

    collider_.SetCollisionAttribute(colliderAttribute);
    collider_.SetCollisionMask(~colliderAttribute);

	collider_.SetSize(Vector3(2.0f,2.0f,2.0f));
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
	collider_.SetAnchorPoint(Vector3(0.5f,0.5f,0.5f));

    UserData userData;
    userData.typeID = colliderId;
    userData.object = this;
    collider_.SetUserData(userData);

	// コールバック関数を登録
    collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
        OnCollisionEnter(result);
    });
}

void DestructibleObject::Initialize() {

	const auto& fractureChunks = model_->GetFractureChunks();
	// 破壊データを持たないモデルだった場合、飛ばす
	if (fractureChunks.empty()) {
		return;
	}
	groupName_ = fractureChunks.begin()->first;
	const auto& chunks = fractureChunks.begin()->second;

	// chunkIdから検索できるようにマップ化
	for (const auto& entry : chunks) {
		chunksById_[entry.info.chunkId] = &entry;
	}

	// 全チャンクの重心平均を、ひび押し出しの基準点として保持
	Vector3 sum = { 0.0f, 0.0f, 0.0f };
	for (const auto& entry : chunks) {
		sum += entry.info.centroid;
	}
	modelCenter_ = sum / static_cast<float>(chunks.size());

	// 全チャンクが無傷の状態に初期化
	std::vector<uint32_t> allIds;
	allIds.reserve(chunks.size());
	for (const auto& entry : chunks) {
		allIds.push_back(entry.info.chunkId);
	}

	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();
	intactInstance_.Initialize(allIds, *buffer);
	hasIntact_ = true;

	RebuildIntactIndexMap(allIds);
}

void DestructibleObject::Update() {

	// 更新
	worldTransform_.UpdateTransformMatrix();
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());

	if (hasMacroDebris_) {
		SimulateFallingDebris(macroDebrisInstance_, FpsCounter::gameDeltaTime);
	}
	if (hasMicroDebris_) {
		SimulateFallingDebris(microDebrisInstance_, FpsCounter::gameDeltaTime);
	}
}

void DestructibleObject::Draw() {

	if (hasIntact_) {
		renderQueue_->SubmitFracture(model_, intactInstance_);
	}
	if (hasMacroDebris_) {
		renderQueue_->SubmitFracture(model_, macroDebrisInstance_);
	}
	// ランタイムで分割された破片を描画
	if (hasMicroDebris_) {
		const auto& chunk = model_->GetFractureChunks().begin();
		Material* drawMaterial = model_->GetMaterial(chunk->second[0].materialName);
		renderQueue_->SubmitRuntimeCutFragments(microDebrisInstance_, &drawMaterial->GetMaterialBuffer());
	}
}

void DestructibleObject::OnCollisionEnter(const GameEngine::CollisionResult& result) {
    //ApplyDamage(result.contactPosition, 1.0f);
	//result.penetrationDepth * 2.0f + 0.5f
	ApplyChipDamage(result.contactPosition, 2.0f);
}

void DestructibleObject::ApplyDamage(const Vector3& impactPos, float damageRadius) {
	// 破壊データがなければ飛ばす
	if (chunksById_.empty()) {
		return;
	}

	// まだ壊れていない一番近いチャンクをシードとして特定
	std::optional<uint32_t> seedChunkId = FindNearestChunk(impactPos);
	// 壊せるチャンクが残っていないければ飛ばす
	if (!seedChunkId.has_value()) {
		return;
	}

	// フラッドフィルで切り離すチャンク範囲を選ぶ
	std::vector<uint32_t> detachedIds = SelectDetachedChunks(seedChunkId.value(), damageRadius, impactPos);
	if (detachedIds.empty()) {
		return;
	}

	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();

	// シード以外は事前分割のまま切り離す
	std::vector<uint32_t> macroIds;
	macroIds.reserve(detachedIds.size());
	for (uint32_t id : detachedIds) {
		if (id != seedChunkId.value()) {
			macroIds.push_back(id);
		}
	}
	if (!macroIds.empty()) {
		macroDebrisInstance_.Initialize(macroIds, *buffer);
		hasMacroDebris_ = true;

		// 各破片に、衝撃点からの距離に応じた爆発の初速を与える
		ApplyExplosionImpulse(macroDebrisInstance_, macroIds, impactPos, damageRadius);
	}

	// チャンクを取得
	Fragment seedFragment = buffer->ExtractChunk(seedChunkId.value());
	Log("seed tris=" + std::to_string(seedFragment.indices.size() / 3));
	// シードチャンクだけランタイムカット
	microDebrisInstance_.ApplyRuntimeCut(seedFragment, impactPos, 8);
	hasMicroDebris_ = true;

	// シード由来の破片に爆発の初速を与える
	ApplyExplosionImpulseUniform(microDebrisInstance_, impactPos, 8.0f);

	Log("microDebris numInstance = " + std::to_string(microDebrisInstance_.GetNumInstance()));

	// 切り離したチャンクを記録し、intactInstance_を残っているチャンクだけで作り直す
	for (uint32_t id : detachedIds) {
		destroyedChunkIds_.insert(id);
	}

	std::vector<uint32_t> remainingIds;
	for (const auto& [chunkId, entry] : chunksById_) {
		if (destroyedChunkIds_.find(chunkId) == destroyedChunkIds_.end()) {
			remainingIds.push_back(chunkId);
		}
	}

	if (!remainingIds.empty()) {
		intactInstance_.Initialize(remainingIds, *buffer);
		hasIntact_ = true;
		RebuildIntactIndexMap(remainingIds);
	} else {
		// 全チャンクが破壊された
		hasIntact_ = false;
	}

	// 破壊されたチャンクの蓄積情報は不要になるので削除
	for (uint32_t id : detachedIds) {
		chunkDamage_.erase(id);
	}
}

std::vector<uint32_t> DestructibleObject::SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const {
	std::queue<uint32_t> queue;
	std::unordered_set<uint32_t> visited;
	std::vector<uint32_t> result;

	queue.push(seedChunkId);
	visited.insert(seedChunkId);

	while (!queue.empty()) {
		uint32_t id = queue.front();
		queue.pop();

		auto it = chunksById_.find(id);
		if (it == chunksById_.end()) {
			continue;
		}
		const FractureChunkInfo& info = it->second->info;

		// アンカーチャンクは境界として残し、切り離さない
		if (info.isAnchored) {
			continue;
		}

		result.push_back(id);

		for (uint32_t neighborId : info.neighborChunkIds) {
			if (visited.count(neighborId)) {
				continue;
			}
			// 既に壊れているチャンクは選ばない
			if (destroyedChunkIds_.count(neighborId)) {
				continue;
			}
			auto neighborIt = chunksById_.find(neighborId);
			if (neighborIt == chunksById_.end()) {
				continue;
			}
			// ダメージ半径の外
			float dist = Math::Length(neighborIt->second->info.centroid - impactPos);
			if (dist > damageRadius) {
				continue;
			}
			visited.insert(neighborId);
			queue.push(neighborId);
		}
	}

	return result;
}

std::optional<uint32_t> DestructibleObject::FindNearestChunk(const Vector3& impactPos) const {
	std::optional<uint32_t> nearestId;
	float nearestDistSq = FLT_MAX;

	for (const auto& [chunkId, entry] : chunksById_) {
		// 既に壊れているチャンクは飛ばす
		if (destroyedChunkIds_.find(chunkId) != destroyedChunkIds_.end()) {
			continue;
		}
		Vector3 diff = entry->info.centroid - impactPos;
		float distSq = Math::Dot(diff, diff);
		if (distSq < nearestDistSq) {
			nearestDistSq = distSq;
			nearestId = chunkId;
		}
	}
	return nearestId;
}

void DestructibleObject::SimulateFallingDebris(FractureInstance& instance, float deltaTime) {
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		state.velocity.y -= 9.8f * deltaTime;
		state.transform.translate += state.velocity * deltaTime;
		state.transform.rotate.x += 1.0f * deltaTime;
		state.transform.rotate.y += 0.6f * deltaTime;
	}
	instance.Update();
}

void DestructibleObject::ApplyExplosionImpulse(FractureInstance& instance,
	const std::vector<uint32_t>& chunkIds, const Vector3& impactPos, float damageRadius) {

	auto& transforms = instance.GetTransformDatas();
	for (size_t i = 0; i < chunkIds.size(); ++i) {
		const auto& entry = chunksById_.at(chunkIds[i]);
		Vector3 dir = entry->info.centroid - impactPos;
		float dist = Math::Length(dir);
		if (dist < 1e-4f) {
			// 中心とほぼ同じ位置なら上方向にフォールバック
			dir = Vector3(0.0f, 1.0f, 0.0f);
			dist = 1e-4f;
		} else {
			// 正規化
			dir = dir / dist;
		}

		// 中心に近いほど強く、遠いほど弱い
		float falloff = std::max(0.0f, 1.0f - dist / damageRadius);
		// 最低速度3、中心付近で最大12程度
		float speed = 3.0f + falloff * 9.0f;

		transforms[i].velocity = dir * speed;
		transforms[i].velocity += RandomGenerator::GetVector3(-0.5f, 0.5f);
	}
}

void DestructibleObject::ApplyExplosionImpulseUniform(FractureInstance& instance,
	const Vector3& impactPos, float strength) {

	// ランタイムカット破片には個別の重心情報がないので、破片ごとにランダム方向へ均等に飛ばす
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		Vector3 randomDir = RandomGenerator::GetVector3(-1.0f, 1.0f);
		randomDir.Normalize();
		state.velocity = randomDir * strength + Vector3(0.0f, strength * 0.3f, 0.0f);
	}
}

void DestructibleObject::ApplyChipDamage(const Vector3& impactPos, float damageAmount) {
	if (chunksById_.empty()) {
		return;
	}

	std::optional<uint32_t> targetChunkId = FindNearestChunk(impactPos);
	if (!targetChunkId.has_value()) {
		return;
	}
	uint32_t chunkId = targetChunkId.value();

	float& accumulated = chunkDamage_[chunkId];
	accumulated += damageAmount;

	float ratio = std::min(accumulated / kBreakThreshold_, 1.0f);
	UpdateCrackVisual(chunkId, ratio);

	// 隣接チャンクにも弱めのひびを波及させる
	if (kNeighborCrackFactor_ > 0.0f) {
		auto it = chunksById_.find(chunkId);
		if (it != chunksById_.end()) {
			for (uint32_t neighborId : it->second->info.neighborChunkIds) {
				if (destroyedChunkIds_.count(neighborId)) continue;
				UpdateCrackVisual(neighborId, ratio * kNeighborCrackFactor_);
			}
		}
	}

	if (accumulated >= kBreakThreshold_) {
		chunkDamage_.erase(chunkId);
		ApplyDamage(impactPos, 1.0f);
	}
}

void DestructibleObject::UpdateCrackVisual(uint32_t chunkId, float ratio) {
	auto indexIt = chunkIndexInIntact_.find(chunkId);
	auto chunkIt = chunksById_.find(chunkId);
	if (indexIt == chunkIndexInIntact_.end() || chunkIt == chunksById_.end()) {
		return;
	}

	Vector3 dir = chunkIt->second->info.centroid - modelCenter_;
	float len = Math::Length(dir);
	if (len < 1e-4f) {
		dir = Vector3(0.0f, 1.0f, 0.0f);
	} else {
		dir = dir / len;
	}

	auto& transforms = intactInstance_.GetTransformDatas();
	FractureChunkState& state = transforms[indexIt->second];

	// 中心から外側へ押し出す
	state.transform.translate = dir * (ratio * kMaxCrackOffset_);

	// ダメージが大きいほど揺れが激しくなる、微小なランダム回転
	state.transform.rotate.x = RandomGenerator::Get(-ratio, ratio) * kMaxCrackRotate_;
	state.transform.rotate.y = RandomGenerator::Get(-ratio, ratio) * kMaxCrackRotate_;
	state.transform.rotate.z = RandomGenerator::Get(-ratio, ratio) * kMaxCrackRotate_;

	intactInstance_.Update();
}

void DestructibleObject::RebuildIntactIndexMap(const std::vector<uint32_t>& ids) {
	chunkIndexInIntact_.clear();
	for (uint32_t i = 0; i < ids.size(); ++i) {
		chunkIndexInIntact_[ids[i]] = i;
	}
}

