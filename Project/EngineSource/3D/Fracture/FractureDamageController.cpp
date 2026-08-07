#define NOMINMAX
#include <algorithm>
#include <queue>
#include "FractureDamageController.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "LogManager.h"
using namespace GameEngine;

void FractureDamageController::Initialize(Model* model) {
	model_ = model;

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
	breakState_.Intact().Initialize(allIds, *buffer);

	RebuildIntactIndexMap(allIds);
}

void FractureDamageController::Update(float deltaTime) {
	SimulateCrackPhysics();

	if (breakState_.HasMacroDebris()) {
		SimulateFallingDebris(breakState_.MacroDebris(), deltaTime);
	}
	if (breakState_.HasMicroDebris()) {
		SimulateFallingDebris(breakState_.MicroDebris(), deltaTime);
	}
}

void FractureDamageController::ApplyChipDamage(const Vector3& impactPos, float damageAmount, float craterRadius, int craterPlaneCount) {
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
	UpdateCrackVisual(chunkId, ratio, damageAmount);

	// 隣接チャンクにも弱めのひびを波及させる
	if (kNeighborCrackFactor_ > 0.0f) {
		auto it = chunksById_.find(chunkId);
		if (it != chunksById_.end()) {
			for (uint32_t neighborId : it->second->info.neighborChunkIds) {
				if (destroyedChunkIds_.count(neighborId)) continue;
				UpdateCrackVisual(neighborId, ratio * kNeighborCrackFactor_, damageAmount * kNeighborCrackFactor_);
			}
		}
	}

	if (accumulated >= kBreakThreshold_) {
		chunkDamage_.erase(chunkId);
		ApplyDamage(impactPos, 1.0f, craterRadius, craterPlaneCount);
	}
}

void FractureDamageController::ApplyDamage(const Vector3& impactPos, float damageRadius, float craterRadius, int craterPlaneCount) {
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
		breakState_.MacroDebris().Initialize(macroIds, *buffer);

		// 各破片に、衝撃点からの距離に応じた爆発の初速を与える
		ApplyExplosionImpulse(breakState_.MacroDebris(), macroIds, impactPos, damageRadius);
	}

	// チャンクを取得
	Fragment seedFragment = buffer->ExtractChunk(seedChunkId.value());
	Log("seed tris=" + std::to_string(seedFragment.indices.size() / 3));
	// シードチャンクだけランタイムカット
	breakState_.MicroDebris().ApplyRuntimeCut(seedFragment, impactPos, craterRadius, 8, craterPlaneCount);

	// シード由来の破片に爆発の初速を与える
	ApplyExplosionImpulseUniform(breakState_.MicroDebris(), impactPos, 8.0f);

	Log("microDebris numInstance = " + std::to_string(breakState_.MicroDebris().GetNumInstance()));

	// 切り離したチャンクを記録し、無傷インスタンスを残っているチャンクだけで作り直す
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
		breakState_.Intact().Initialize(remainingIds, *buffer);
		RebuildIntactIndexMap(remainingIds);
	} else {
		// 全チャンクが破壊された
		breakState_.Intact().Clear();
	}

	// 破壊されたチャンクの蓄積情報は不要になるので削除
	for (uint32_t id : detachedIds) {
		chunkDamage_.erase(id);
		crackActiveChunkIds_.erase(id);
	}
}

std::vector<uint32_t> FractureDamageController::SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const {
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

std::optional<uint32_t> FractureDamageController::FindNearestChunk(const Vector3& impactPos) const {
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

void FractureDamageController::SimulateFallingDebris(FractureInstance& instance, float deltaTime) {
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		state.velocity.y -= 9.8f * deltaTime;
		state.transform.translate += state.velocity * deltaTime;
		state.transform.rotate.x += 1.0f * deltaTime;
		state.transform.rotate.y += 0.6f * deltaTime;
	}
	instance.Update();
}

void FractureDamageController::ApplyExplosionImpulse(FractureInstance& instance,
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

void FractureDamageController::ApplyExplosionImpulseUniform(FractureInstance& instance,
	const Vector3& impactPos, float strength) {

	// ランタイムカット破片には個別の重心情報がないので、破片ごとにランダム方向へ均等に飛ばす
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		Vector3 randomDir = RandomGenerator::GetVector3(-1.0f, 1.0f);
		randomDir.Normalize();
		state.velocity = randomDir * strength + Vector3(0.0f, strength * 0.3f, 0.0f);
	}
}

void FractureDamageController::UpdateCrackVisual(uint32_t chunkId, float ratio, float damageDelta) {
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

	auto& transforms = breakState_.Intact().GetTransformDatas();
	FractureChunkState& state = transforms[indexIt->second];

	// ダメージが蓄積するほど外側に押し出された位置になる
	state.crackRestOffset = dir * (ratio * kMaxCrackOffset_);

	// 衝撃の瞬間速度インパルス
	float impulse = kCrackImpulseStrength_ * damageDelta;
	state.crackVelocity += dir * impulse;
	state.crackAngularVelocity += RandomGenerator::GetVector3(-1.0f, 1.0f) * impulse * kMaxCrackRotate_;

	// このチャンクは以後、毎フレームのばねシミュレーション対象にする
	crackActiveChunkIds_.insert(chunkId);

	breakState_.Intact().Update();
}

void FractureDamageController::RebuildIntactIndexMap(const std::vector<uint32_t>& ids) {
	chunkIndexInIntact_.clear();
	for (uint32_t i = 0; i < ids.size(); ++i) {
		chunkIndexInIntact_[ids[i]] = i;
	}
}

void FractureDamageController::SimulateCrackPhysics() {
	// 揺れているチャンクがなければ飛ばず
	if (crackActiveChunkIds_.empty()) {
		return;
	}

	auto& transforms = breakState_.Intact().GetTransformDatas();
	// 収束したとみなす値
	constexpr float kSettleThreshold = 0.0005f;

	std::vector<uint32_t> settledIds;

	for (uint32_t chunkId : crackActiveChunkIds_) {
		auto indexIt = chunkIndexInIntact_.find(chunkId);
		// 壊れて消えた等、対象にしない
		if (indexIt == chunkIndexInIntact_.end()) {
			settledIds.push_back(chunkId);
			continue;
		}

		FractureChunkState& state = transforms[indexIt->second];

		// 位置のばね
		Vector3 displacement = state.transform.translate - state.crackRestOffset;
		Vector3 accel = displacement * -kCrackSpringStiffness_ - state.crackVelocity * kCrackDamping_;
		state.crackVelocity += accel * FpsCounter::gameDeltaTime;
		state.transform.translate += state.crackVelocity * FpsCounter::gameDeltaTime;

		// 回転のばね
		Vector3 rotAccel = state.transform.rotate * -kCrackAngularSpringStiffness_ - state.crackAngularVelocity * kCrackAngularDamping_;
		state.crackAngularVelocity += rotAccel * FpsCounter::gameDeltaTime;
		state.transform.rotate += state.crackAngularVelocity * FpsCounter::gameDeltaTime;

		// 十分収まったらアクティブリストから外す
		bool settled = Math::Length(displacement) < kSettleThreshold
			&& Math::Length(state.crackVelocity) < kSettleThreshold
			&& Math::Length(state.transform.rotate) < kSettleThreshold
			&& Math::Length(state.crackAngularVelocity) < kSettleThreshold;
		if (settled) {
			settledIds.push_back(chunkId);
		}
	}

	for (uint32_t id : settledIds) {
		crackActiveChunkIds_.erase(id);
	}

	breakState_.Intact().Update();
}
