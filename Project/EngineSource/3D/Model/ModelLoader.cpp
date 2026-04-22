#include "ModelLoader.h"
#include <fstream>
#include <sstream>
#include <cassert>

#include "LogManager.h"
#include "CreateBufferResource.h"
#include "MyMath.h"
#include "EasingManager.h"
#include "Model.h"

using namespace GameEngine;

void ModelLoader::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,TextureManager* textureManager, SrvManager* srvManager) {
	device_ = device;
	cmdList_ = cmdList;
	textureManager_ = textureManager;
	srvManager_ = srvManager;
}

[[nodiscard]]
std::unique_ptr<Model> ModelLoader::CreateSphere(uint32_t subdivision) {
	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// メッシュを作成
	auto tmpMesh = std::make_unique<Mesh>();
	tmpMesh->CreateSphereMesh(subdivision);
	model->AddMesh(std::move(tmpMesh));

	// Meshを元にBLASを作成する
	model->AddBLAS(cmdList_);

	// マテリアルを作成
	std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
	tmpMaterial->Initialize({ 1.0f,1.0f,1.0f,1.0f }, { 1.0f,1.0f,1.0f }, 500.0f, false);
	std::string materialName = tmpMesh->GetMaterialName();
	model->AddMaterial(materialName, std::move(tmpMaterial));

	return model;
}

[[nodiscard]]
std::unique_ptr<Model> ModelLoader::CreatePlane(const Vector2& size) {

	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();
	// メッシュを作成
	auto tmpMesh = std::make_unique<Mesh>();
	tmpMesh->CreatePlaneMesh(size);
	model->AddMesh(std::move(tmpMesh));

	// マテリアルを作成
	std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
	tmpMaterial->Initialize({ 1.0f,1.0f,1.0f,1.0f }, { 1.0f,1.0f,1.0f }, 500.0f, false);
	std::string materialName = tmpMesh->GetMaterialName();
	model->AddMaterial(materialName, std::move(tmpMaterial));

	return model;
}

[[nodiscard]]
std::unique_ptr<Model> ModelLoader::CreateGridPlane(const Vector2& size) {

	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// メッシュを作成
	std::unique_ptr<Mesh> tmpMesh = std::make_unique<Mesh>();
	tmpMesh->CreateGridPlaneMesh(size);
	model->AddMesh(std::move(tmpMesh));

	return model;
}

[[nodiscard]]
std::unique_ptr<Model> ModelLoader::CreateModel(const std::string& objFilename, const std::string& filename) {

	LogManager::GetInstance().Log("Start create model");

	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// Assimpを使ったモデルの生成するログを出す
	LogManager::GetInstance().Log("Create From Assimp : Start loading Model file: " + filename + objFilename);

	// データを読み込む処理
	LogManager::GetInstance().Log("Create From Assimp : Loading Model file data");

	ModelData modelData = LoadModelFile(kDirectoryPath_, objFilename, filename);

	// モデルが無事に作成されたログを出す
	LogManager::GetInstance().Log("Create From Assimp : Success loaded Model file: " + filename + objFilename);

	// メッシュを作成
	for (uint32_t index = 0; index < modelData.meshes.size(); ++index) {
		std::unique_ptr<Mesh> tmpMesh = std::make_unique<Mesh>();
		tmpMesh->CreateModelMesh(modelData, index);

		model->AddMesh(std::move(tmpMesh));
	}

	// Meshを元にBLASを作成する
	model->AddBLAS(cmdList_);

	// マテリアルを作成
	for (uint32_t index = 0; index < modelData.materials.size(); ++index) {
		std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
		tmpMaterial->Initialize(modelData.materials[index].color, modelData.materials[index].specularColor, modelData.materials[index].shininess, false);

		// テクスチャ情報があればを取得
		if (!modelData.materials[index].textureFilePath.empty()) {
			std::string texPath = std::filesystem::path(modelData.materials[index].textureFilePath).filename().string();
			uint32_t textureHandle = textureManager_->GetHandleByName(texPath);
			tmpMaterial->SetTextureHandle(textureHandle);
			tmpMaterial->SetDefaultTexture(textureHandle);
		}

		model->AddMaterial(modelData.materials[index].name,std::move(tmpMaterial));
	}

	// 外部からモデルをロードした時に必要な情報を取得
	model->SetLoadModelData(modelData.rootNode.name, modelData.rootNode.localMatrix);

	// ボーンのデータが存在している場合、読み込む
	if (modelData.isAnimation_) {
		LogManager::GetInstance().Log(objFilename + " : Load animationData");
		// ボーン情報を取得する
		Skeleton skeletonBone = CreateSkeleton(modelData.rootNode);
		SkinCluster skinClusterBone = CreateSkinCluster(skeletonBone, modelData);

		model->SetSkeletonData(skeletonBone, skinClusterBone);
	}

	LogManager::GetInstance().Log("End create model");

	return model;
}


[[nodiscard]]
Skeleton ModelLoader::CreateSkeleton(const Node& rootNode) {

	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}
	return skeleton;
}

[[nodiscard]]
AnimationData ModelLoader::LoadAnimationFile(const std::string& objFilename, const std::string& filename) {

	// Assimpを使ったモデルの生成するログを出す
	LogManager::GetInstance().Log("Load AnimationData From Assimp : Start loading Model file: " + filename + objFilename);

	AnimationData animation;
	Assimp::Importer importer;
	std::string filePath = kDirectoryPath_ + "/" + filename + "/" + objFilename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0); // アニメーションがない
	aiAnimation* animationAssimp = scene->mAnimations[0];  // 最初のアニメーションだけ採用。
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変換

	// assimpでは個々のNodeのAnimationをchannelと呼んでいるのでchannelを回してNodeAnimationの情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// 位置
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);  // ここも秒に変換
			keyframe.value = { -keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z }; // 右手->左手
			nodeAnimation.translate.push_back(keyframe);
		}
		// 回転
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x,-keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.push_back(keyframe);
		}
		// 拡縮
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x,keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.push_back(keyframe);
		}
	}

	// モデルが無事に作成されたログを出す
	LogManager::GetInstance().Log("Load AnimationData From Assimp : Success loaded Model file: " + filename + objFilename);

	// 解析結果を返す
	return animation;
}

[[nodiscard]]
std::map<std::string, AnimationData> ModelLoader::LoadAnimationsFile(const std::string& objFilename, const std::string& filename) {
	// Assimpを使ったモデルの生成するログを出す
	LogManager::GetInstance().Log("Load AnimationData From Assimp : Start loading Model file: " + filename + objFilename);

	std::map<std::string, AnimationData> loadedAnimations;

	Assimp::Importer importer;
	std::string filePath = kDirectoryPath_ + "/" + filename + "/" + objFilename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0); // アニメーションがない

	for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
		aiAnimation* animationAssimp = scene->mAnimations[animationIndex];

		// 新しいアニメーションデータを作成
		AnimationData animation;

		// 時間を取得する
		animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変換

		// assimpでは個々のNodeのAnimationをchannelと呼んでいるのでchannelを回してNodeAnimationの情報を取ってくる
		for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
			aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
			NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

			// 位置
			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);  // ここも秒に変換
				keyframe.value = { -keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z }; // 右手->左手
				nodeAnimation.translate.push_back(keyframe);
			}
			// 回転
			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
				aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x,-keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
				nodeAnimation.rotate.push_back(keyframe);
			}
			// 拡縮
			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x,keyAssimp.mValue.y, keyAssimp.mValue.z };
				nodeAnimation.scale.push_back(keyframe);
			}
		}

		// アニメーション名を取得
		std::string animationName = animationAssimp->mName.C_Str();

		// 名前が空の場合、名前を設定する
		if (animationName.empty()) {
			animationName = "AnimationIndex" + std::to_string(animationIndex);
		}

		loadedAnimations[animationName] = animation;
	}

	// モデルが無事に作成されたログを出す
	LogManager::GetInstance().Log("Load AnimationData From Assimp : Success loaded Model file: " + filename + objFilename);

	// 解析結果を返す
	return loadedAnimations;
}


[[nodiscard]]
ModelData ModelLoader::LoadModelFile(const std::string& directoryPath, const std::string& objFilename, const std::string& filename) {

	ModelData modelData;

	// ファイルを読み込み
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename + "/" + objFilename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene && scene->HasMeshes()); // メッシュがないのは対応しない

	// アニメーションデータの確認をする
	modelData.isAnimation_ = (scene->mNumAnimations != 0) ? true : false;

	// Material解析
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		LoadMaterialData materialData;

		// マテリアル名を取得
		aiString name;
		material->Get(AI_MATKEY_NAME, name);
		materialData.name = name.C_Str();

		if (name.length > 0) {
			materialData.name = name.C_Str();
		}

		// Assimpのデフォルトマテリアルまたは空のマテリアル名をスキップ
		if (materialData.name == "DefaultMaterial" || materialData.name.empty()) {
			continue;
		}

		// テクスチャを取得
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath) == AI_SUCCESS) {
				materialData.textureFilePath = directoryPath + "/" + filename + "/" + textureFilePath.C_Str();
			}
		}

		// 色を取得
		aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor)) {
			materialData.color = { diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f };
		}

		// 鏡面反射の色を取得
		aiColor3D specularColor(1.0f, 1.0f, 1.0f);
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor)) {
			materialData.specularColor = { specularColor.r, specularColor.g, specularColor.b };
		}

		// 輝度
		float shininess = 0.0f;
		if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess)) {
			materialData.shininess = shininess;
		}

		modelData.materials.push_back(std::move(materialData));
	}

	// Mesh解析
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals()); // 法線がないMeshは今回は非対応

		// 最初に頂点分メモリを確保する
		MeshData meshData;
		meshData.vertices.resize(mesh->mNumVertices);
		// メッシュに対応するマテリアル名を取得する
		aiMaterial* meshMaterial = scene->mMaterials[mesh->mMaterialIndex];
		aiString materialName;
		meshMaterial->Get(AI_MATKEY_NAME, materialName);

		if (materialName.length > 0) {
			meshData.materialName = materialName.C_Str();
		}

		// Assimpのデフォルトマテリアルまたは空のマテリアル名をスキップ
		if (meshData.materialName == "DefaultMaterial" || meshData.materialName.empty()) {
			continue;
		}

		// Vertex解析
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			VertexData vertex;
			// 右手->左手に変換する
			vertex.position = { -position.x, position.y, position.z, 1.0f };
			vertex.normal = { -normal.x, normal.y, normal.z };

			// UVの適応
			if (mesh->HasTextureCoords(0)) {
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
				vertex.texcoord = { texcoord.x, texcoord.y };
			} else {
				// UVがない場合、XZ平面に投影したUVを仮生成
				vertex.texcoord = { (position.x + 1.0f) * 0.5f, (position.z + 1.0f) * 0.5f };
			}

			meshData.vertices[vertexIndex] = vertex;
		}

		// Face解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices >= 3);
			for (uint32_t i = 0; i < face.mNumIndices; ++i) {
				meshData.indices.push_back(face.mIndices[i]);
			}
		}

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix({ scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z });
			jointWeightData.inverseBindPoseMatrix = InverseMatrix(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
			}
		}

		modelData.meshes.push_back(std::move(meshData));
	}

	// シーン全体の階層構造を作る
	modelData.rootNode = ReadNode(scene->mRootNode);

	return modelData;
}

[[nodiscard]]
Node ModelLoader::ReadNode(aiNode* node) {
	Node result;

	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);
	result.transform.scale = { scale.x,scale.y,scale.z };
	result.transform.rotate = { rotate.x,-rotate.y,-rotate.z,rotate.w }; // x軸を反転、さらに回転方向が逆なので軸を反転する
	result.transform.translate = { -translate.x,translate.y,translate.z }; // x軸を反転
	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str(); // Node名を格納
	result.children.resize(node->mNumChildren); // 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 再帰的に読んで階層構造を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}
	return result;
}

[[nodiscard]]
int32_t ModelLoader::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {

	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size()); // 現在登録されている数をIndexに
	joint.parent = parent;
	joints.push_back(joint); // SkeletonのJoint列に追加
	for (const Node& child : node.children) {
		// 子Jointを作成し、そのIndexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;
}


[[nodiscard]]
SkinCluster ModelLoader::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData) {

	assert(!skeleton.joints.empty() && "Skeleton joints are empty!");
	assert(!modelData.meshes.empty() && "Model has no meshes!");
	assert(!modelData.meshes[0].vertices.empty() && "Model mesh[0] has no vertices!");

	uint32_t srvIndex = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

	SkinCluster skinCluster;
	// palette用のResourceを確保
	skinCluster.paletteResource = CreateBufferResource(device_, sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() }; // spanを使ってアクセスするようにする
	skinCluster.paletteSrvHandle.first = srvManager_->GetCPUHandle(srvIndex);
	skinCluster.paletteSrvHandle.second = srvManager_->GetGPUHandle(srvIndex);

	// palette用のsrvを作成。StructuredBufferでアクセス出来るようにする。
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = static_cast<UINT>(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	device_->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	// influence用のResourceを確保。頂点ごとにinfluence情報を追加出来るようにする
	skinCluster.influenceResource = CreateBufferResource(device_, sizeof(VertexInfluence) * modelData.meshes[0].vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.meshes[0].vertices.size()); // 0埋め。weightを0にしておく
	skinCluster.mappedInfluence = { mappedInfluence,modelData.meshes[0].vertices.size() };

	// influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexInfluence) * modelData.meshes[0].vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// inverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	// ModelのSkinClusterの情報を解析
	for (const auto& jointWeight : modelData.skinClusterData) {
		auto it = skeleton.jointMap.find(jointWeight.first); // jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end()) { // 存在しない場合は次に回す
			continue;
		}
		// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) { // 空いているところに入れる
				if (currentInfluence.weights[index] == 0.0f) { // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster;
}
