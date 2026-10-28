#include "pch.h"
#define NOMINMAX
#include "ModelLoader.h"
#include <cassert>
#include "LogManager.h"
#include "MyMath.h"
#include "Model.h"
#include "ParallelFor.h"

using namespace GameEngine;

void ModelLoader::Initialize(ID3D12GraphicsCommandList4* cmdList,TextureManager* textureManager, SrvManager* srvManager) {
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
	model->AddBLAS(cmdList_, false);

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
std::unique_ptr<Model> ModelLoader::CreateRing(uint32_t ringDivide, float outerRadius, float innerRadius) {
	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// メッシュを作成
	auto tmpMesh = std::make_unique<Mesh>();
	tmpMesh->CreateRingMesh(ringDivide, outerRadius, innerRadius);

	// マテリアルを作成
	std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
	tmpMaterial->Initialize({ 1.0f,1.0f,1.0f,1.0f }, { 1.0f,1.0f,1.0f }, 500.0f, false);
	std::string materialName = tmpMesh->GetMaterialName();

	model->AddMesh(std::move(tmpMesh));
	model->AddMaterial(materialName, std::move(tmpMaterial));
	// blasを作成
	model->AddBLAS(cmdList_, false);

	// レイトレでの参照用
	model->CreateRefBuffer();

	return model;
}

[[nodiscard]]
std::unique_ptr<Model> ModelLoader::CreateCylinder(uint32_t cylinderDivide, float topRadius, float bottomRadius, float height) {
	// インスタンスを生成
	std::unique_ptr<Model> model = std::make_unique<Model>();

	// メッシュを作成
	auto tmpMesh = std::make_unique<Mesh>();
	tmpMesh->CreateCylinder(cylinderDivide, topRadius, bottomRadius, height);

	// マテリアルを作成
	std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
	tmpMaterial->Initialize({ 1.0f,1.0f,1.0f,1.0f }, { 1.0f,1.0f,1.0f }, 500.0f, false);
	std::string materialName = tmpMesh->GetMaterialName();

	model->AddMesh(std::move(tmpMesh));
	model->AddMaterial(materialName, std::move(tmpMaterial));
	// blasを作成
	model->AddBLAS(cmdList_, false);

	// レイトレでの参照用
	model->CreateRefBuffer();

	return model;
}

std::unique_ptr<ModelData> ModelLoader::ParseModel(const std::string& objFilename, const std::string& filename) {
	return std::make_unique<ModelData>(LoadModelFile(kDirectoryPath_, objFilename, filename));
}

std::unique_ptr<Model> ModelLoader::BuildModel(const std::string& objFilename, std::unique_ptr<ModelData> parsed) {
	LogManager::GetInstance().Log("Start create model");

	std::unique_ptr<Model> model = std::make_unique<Model>();
	ModelData& modelData = *parsed;   // 以降は従来の CreateModel の本体と同じ

	std::unordered_map<std::string, std::vector<MeshData>> chunkMeshesByGroup;

	// メッシュを作成
	for (uint32_t index = 0; index < modelData.meshes.size(); ++index) {

		// 破壊のチャンクデータであれば1つのデータに詰め込む
		const auto& meshData = modelData.meshes[index];
		if (meshData.fractureInfo.has_value()) {
			chunkMeshesByGroup[meshData.fractureInfo->groupName].push_back(meshData);
		} else {
			// 破壊データでなければメッシュを作成
			std::unique_ptr<Mesh> tmpMesh = std::make_unique<Mesh>();
			tmpMesh->CreateModelMesh(modelData, index);

			model->AddMesh(std::move(tmpMesh));
		}
	}

	// Meshを元にBLASを作成する。アニメーションがあればBLASを更新用に作成
	model->AddBLAS(cmdList_, modelData.isSkeleton);

	uint32_t materialSrvIndex = 0;

	// マテリアルを作成
	for (uint32_t index = 0; index < modelData.materials.size(); ++index) {
		std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
		tmpMaterial->Initialize(modelData.materials[index].color, modelData.materials[index].specularColor, modelData.materials[index].shininess, true);
		materialSrvIndex = tmpMaterial->GetMaterialSrvIndex();

		// テクスチャ情報があればを取得
		if (!modelData.materials[index].textureFilePath.empty()) {
			std::string texPath = std::filesystem::path(modelData.materials[index].textureFilePath).filename().string();
			//uint32_t textureHandle = textureManager_->GetHandleByName(texPath);
			//tmpMaterial->SetTextureHandle(textureHandle);
			//tmpMaterial->SetDefaultTexture(textureHandle);
			tmpMaterial->SetTextureName(texPath);
		}

		model->AddMaterial(modelData.materials[index].name, std::move(tmpMaterial));
	}

	// 破壊のチャンクデータを作成する
	for (auto& [groupName, chunkMeshes] : chunkMeshesByGroup) {
		auto packedBuffer = std::make_unique<PackedGeometryBuffer>();
		packedBuffer->Build(chunkMeshes);
		packedBuffer->BuildBLAS(cmdList_, materialSrvIndex);

		std::vector<FractureChunkEntry> entries;
		entries.reserve(chunkMeshes.size());
		for (const auto& meshData : chunkMeshes) {
			FractureChunkEntry entry;
			entry.materialName = meshData.materialName;
			entry.range = packedBuffer->GetRange(meshData.fractureInfo->chunkId);
			entry.info = meshData.fractureInfo.value();
			entries.push_back(std::move(entry));
		}

		model->AddFractureGroup(groupName, std::move(packedBuffer), std::move(entries));
	}

	// 外部からモデルをロードした時に必要な情報を取得
	model->SetLoadModelData(modelData.rootNode);

	// ボーンのデータが存在している場合、読み込む
	if (modelData.isSkeleton) {
		LogManager::GetInstance().Log(objFilename + " : Load animationData");
		// ボーン情報を取得する
		SkeletonData skeletonBone = CreateSkeleton(modelData.rootNode);

		std::unique_ptr<Skeleton> skeleton = std::make_unique<Skeleton>();
		skeleton->Create(cmdList_, skeletonBone, modelData);

		model->SetSkeleton(std::move(skeleton));
	}

	// 参照用バッファを作成
	model->CreateRefBuffer();

	LogManager::GetInstance().Log("End create model");

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

	// 破壊のチャンクデータ
	std::unordered_map<std::string, std::vector<MeshData>> chunkMeshesByGroup;

	// メッシュを作成
	for (uint32_t index = 0; index < modelData.meshes.size(); ++index) {

		// 破壊のチャンクデータであれば1つのデータに詰め込む
		const auto& meshData = modelData.meshes[index];
		if (meshData.fractureInfo.has_value()) {
			chunkMeshesByGroup[meshData.fractureInfo->groupName].push_back(meshData);
		} else {
			// 破壊データでなければメッシュを作成
			std::unique_ptr<Mesh> tmpMesh = std::make_unique<Mesh>();
			tmpMesh->CreateModelMesh(modelData, index);

			model->AddMesh(std::move(tmpMesh));
		}
	}

	// Meshを元にBLASを作成する。アニメーションがあればBLASを更新用に作成
	model->AddBLAS(cmdList_, modelData.isSkeleton);

	uint32_t materialSrvIndex = 0;

	// マテリアルを作成
	for (uint32_t index = 0; index < modelData.materials.size(); ++index) {
		std::unique_ptr<Material> tmpMaterial = std::make_unique<Material>();
		tmpMaterial->Initialize(modelData.materials[index].color, modelData.materials[index].specularColor, modelData.materials[index].shininess, true);
		materialSrvIndex = tmpMaterial->GetMaterialSrvIndex();

		// テクスチャ情報があればを取得
		if (!modelData.materials[index].textureFilePath.empty()) {
			std::string texPath = std::filesystem::path(modelData.materials[index].textureFilePath).filename().string();
			//uint32_t textureHandle = textureManager_->GetHandleByName(texPath);
			//tmpMaterial->SetTextureHandle(textureHandle);
			//tmpMaterial->SetDefaultTexture(textureHandle);
			tmpMaterial->SetTextureName(texPath);
		}

		model->AddMaterial(modelData.materials[index].name,std::move(tmpMaterial));
	}

	// 破壊のチャンクデータを作成する
	for (auto& [groupName, chunkMeshes] : chunkMeshesByGroup) {
		auto packedBuffer = std::make_unique<PackedGeometryBuffer>();
		packedBuffer->Build(chunkMeshes);
		packedBuffer->BuildBLAS(cmdList_, materialSrvIndex);

		std::vector<FractureChunkEntry> entries;
		entries.reserve(chunkMeshes.size());
		for (const auto& meshData : chunkMeshes) {
			FractureChunkEntry entry;
			entry.materialName = meshData.materialName;
			entry.range = packedBuffer->GetRange(meshData.fractureInfo->chunkId);
			entry.info = meshData.fractureInfo.value();
			entries.push_back(std::move(entry));
		}

		model->AddFractureGroup(groupName, std::move(packedBuffer), std::move(entries));
	}

	// 外部からモデルをロードした時に必要な情報を取得
	model->SetLoadModelData(modelData.rootNode);

	// ボーンのデータが存在している場合、読み込む
	if (modelData.isSkeleton) {
		LogManager::GetInstance().Log(objFilename + " : Load animationData");
		// ボーン情報を取得する
		SkeletonData skeletonBone = CreateSkeleton(modelData.rootNode);

		std::unique_ptr<Skeleton> skeleton = std::make_unique<Skeleton>();
		skeleton->Create(cmdList_, skeletonBone, modelData);

		model->SetSkeleton(std::move(skeleton));
	}

	// 参照用バッファを作成
	model->CreateRefBuffer();

	LogManager::GetInstance().Log("End create model");

	return model;
}


[[nodiscard]]
SkeletonData ModelLoader::CreateSkeleton(const Node& rootNode) {

	SkeletonData skeleton;
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
	std::string filePath = filename + "/" + objFilename;
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
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	assert(scene && scene->HasMeshes()); // メッシュがないのは対応しない

	// アニメーションデータの確認をする
	modelData.isAnimation_ = (scene->mNumAnimations != 0) ? true : false;
	uint32_t isSkeleton = false;

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

	// 破壊チャンク情報があるか判断
	std::vector<std::optional<FractureChunkInfo>> chunkInfoByMeshIndex(scene->mNumMeshes);
	std::vector<std::optional<aiMatrix4x4>> nodeTransformByMeshIndex(scene->mNumMeshes);
	std::unordered_map<std::string, uint32_t> chunkCounterByGroup;
	DetectFractureChunks(scene->mRootNode, aiMatrix4x4(), chunkInfoByMeshIndex, nodeTransformByMeshIndex, chunkCounterByGroup);

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

			// 破壊チャンクの場合、ノード側が個別のtransformを持っているため頂点に焼き込む
			if (nodeTransformByMeshIndex[meshIndex].has_value()) {
				const aiMatrix4x4& nodeTransform = nodeTransformByMeshIndex[meshIndex].value();
				position = nodeTransform * position;

				aiMatrix3x3 normalMatrix(nodeTransform);
				normal = normalMatrix * normal;
				normal.Normalize();
			}

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

			// 接戦の適応
			if (mesh->HasTangentsAndBitangents()) {
				aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

				if (std::abs(aiTangent.x) > 0.0001f || std::abs(aiTangent.y) > 0.0001f || std::abs(aiTangent.z) > 0.0001f) {
					Vector3 b = { -aiBitangent.x, aiBitangent.y, aiBitangent.z };
					Vector3 t = { -aiTangent.x, aiTangent.y, aiTangent.z };
					// ハンドネスの判定
					float handedness = Vector3::Dot(vertex.normal.Cross(t), b) < 0.0f ? -1.0f : 1.0f;
					// 右手->左手に変換する
					vertex.tangent = { -aiTangent.x, aiTangent.y, aiTangent.z , handedness };
				} else {
					// 接線がない場合のデフォルト値
					vertex.tangent = { 1.0f, 0.0f, 0.0f,1.0f };
				}
			} else {
				// 接線がない場合のデフォルト値
				vertex.tangent = { 1.0f, 0.0f, 0.0f,1.0f };
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

		// スケルトンを取得
		std::unordered_map<std::string, JointWeightData> meshSkinClusterData;
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = meshSkinClusterData[jointName];
			isSkeleton = true;

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = Matrix4x4::MakeAffineMatrix({ scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z });
			jointWeightData.inverseBindPoseMatrix = Matrix4x4::Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
			}
		}

		// 破壊チャンク情報があれば割り当てる
		if (chunkInfoByMeshIndex[meshIndex].has_value()) {
			meshData.fractureInfo = chunkInfoByMeshIndex[meshIndex];
		}

		modelData.meshes.push_back(std::move(meshData));
		modelData.skinClusterData.push_back(std::move(meshSkinClusterData));
	}

	// 破壊チャンクのAABBから隣接関係を構築する
	BuildFractureAdjacency(modelData);
	// 隣接関係を元に、三角形ごとの外側の表面、内部の切断面を判定する
	ClassifyFractureFaces(modelData);

	// シーン全体の階層構造を作る
	modelData.rootNode = ReadNode(scene->mRootNode);
	// ボーンデータを確認
	modelData.isSkeleton = isSkeleton;

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
	result.localMatrix = Matrix4x4::MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

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
	joint.skeletonSpaceMatrix = Matrix4x4::MakeIdentity();
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

void ModelLoader::DetectFractureChunks(aiNode* node, const aiMatrix4x4& parentTransform,
	std::vector<std::optional<FractureChunkInfo>>& outChunkInfoByMeshIndex,
	std::vector<std::optional<aiMatrix4x4>>& outNodeTransformByMeshIndex,
	std::unordered_map<std::string, uint32_t>& chunkCounterByGroup) {

	aiMatrix4x4 worldTransform = parentTransform * node->mTransformation;
	std::string nodeName = node->mName.C_Str();
	std::string groupName;

	// ノード名に_cellが含まれていれば、このノードが参照するメッシュをチャンクとして登録する
	if (TryExtractFractureGroupName(nodeName, groupName)) {
		// グループごとに検出順で連番を振る
		uint32_t chunkId = chunkCounterByGroup[groupName]++;

		for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
			uint32_t meshIndex = node->mMeshes[i];

			FractureChunkInfo info;
			info.groupName = groupName;
			info.chunkId = chunkId;
			outChunkInfoByMeshIndex[meshIndex] = info;
			outNodeTransformByMeshIndex[meshIndex] = worldTransform;
		}
	}

	// 子ノードも再帰的に走査する
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		DetectFractureChunks(node->mChildren[childIndex], worldTransform, outChunkInfoByMeshIndex, outNodeTransformByMeshIndex, chunkCounterByGroup);
	}
}

[[nodiscard]]
bool ModelLoader::TryExtractFractureGroupName(const std::string& nodeName, std::string& outGroupName) {
	// _cellを含むノードだけを破壊チャンクと判断
	size_t pos = nodeName.find(kFractureChunkMarker_);
	if (pos == std::string::npos) {
		return false;
	}

	outGroupName = nodeName.substr(0, pos);
	return true;
}

void ModelLoader::BuildFractureAdjacency(ModelData& modelData, float threshold) {

	// 各チャンクのAABBと重心を計算する
	for (auto& meshData : modelData.meshes) {
		if (!meshData.fractureInfo.has_value() || meshData.vertices.empty()) {
			continue;
		}

		Vector3 aabbMin = { meshData.vertices[0].position.x, meshData.vertices[0].position.y, meshData.vertices[0].position.z };
		Vector3 aabbMax = aabbMin;

		for (const auto& vertex : meshData.vertices) {
			aabbMin.x = std::min(aabbMin.x, vertex.position.x);
			aabbMin.y = std::min(aabbMin.y, vertex.position.y);
			aabbMin.z = std::min(aabbMin.z, vertex.position.z);
			aabbMax.x = std::max(aabbMax.x, vertex.position.x);
			aabbMax.y = std::max(aabbMax.y, vertex.position.y);
			aabbMax.z = std::max(aabbMax.z, vertex.position.z);
		}

		meshData.fractureInfo->aabb.min = aabbMin;
		meshData.fractureInfo->aabb.max = aabbMax;
		meshData.fractureInfo->centroid = {
			(aabbMin.x + aabbMax.x) * 0.5f,
			(aabbMin.y + aabbMax.y) * 0.5f,
			(aabbMin.z + aabbMax.z) * 0.5f,
		};
	}

	// グループごとの高さレンジから、最下層のチャンクを地面に固定されたアンカーとしてマークする
	// （事前分割チャンクの底面はギザギザなので、絶対値のしきい値ではなく全高に対する割合で判定する）
	struct HeightRange {
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;
	};
	std::unordered_map<std::string, HeightRange> heightRangeByGroup;
	for (auto& meshData : modelData.meshes) {
		if (!meshData.fractureInfo.has_value()) {
			continue;
		}
		HeightRange& range = heightRangeByGroup[meshData.fractureInfo->groupName];
		range.minY = std::min(range.minY, meshData.fractureInfo->aabb.min.y);
		range.maxY = std::max(range.maxY, meshData.fractureInfo->aabb.max.y);
	}
	for (auto& meshData : modelData.meshes) {
		if (!meshData.fractureInfo.has_value()) {
			continue;
		}
		const HeightRange& range = heightRangeByGroup[meshData.fractureInfo->groupName];
		float groupHeight = range.maxY - range.minY;
		float anchorThreshold = range.minY + groupHeight * kFractureAnchorHeightRatio_;
		meshData.fractureInfo->isAnchored = (meshData.fractureInfo->aabb.min.y <= anchorThreshold);
	}

	// AABBが近接、重なっているかどうかを判定するラムダ
	auto isNearOrOverlapping = [threshold](const Vector3& minA, const Vector3& maxA, const Vector3& minB, const Vector3& maxB) {
		return (minA.x - threshold <= maxB.x && maxA.x + threshold >= minB.x) &&
			(minA.y - threshold <= maxB.y && maxA.y + threshold >= minB.y) &&
			(minA.z - threshold <= maxB.z && maxA.z + threshold >= minB.z);
		};

	// 同じグループ内のチャンク同士の総当たりで隣接関係を構築する
	for (size_t i = 0; i < modelData.meshes.size(); ++i) {
		auto& chunkA = modelData.meshes[i].fractureInfo;
		if (!chunkA.has_value()) {
			continue;
		}

		for (size_t j = i + 1; j < modelData.meshes.size(); ++j) {
			auto& chunkB = modelData.meshes[j].fractureInfo;
			if (!chunkB.has_value() || chunkB->groupName != chunkA->groupName) {
				continue;
			}

			if (isNearOrOverlapping(chunkA->aabb.min, chunkA->aabb.max, chunkB->aabb.min, chunkB->aabb.max)) {
				chunkA->neighborChunkIds.push_back(chunkB->chunkId);
				chunkB->neighborChunkIds.push_back(chunkA->chunkId);
			}
		}
	}
}

void ModelLoader::ClassifyFractureFaces(ModelData& modelData, float threshold) {

	// 各メッシュの三角形の形状情報を事前に計算しておく
	std::vector<std::vector<TriangleGeometry>> trianglesByMesh(modelData.meshes.size());
	// グループ名、チャンクIDからメッシュを引けるようにする
	std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<size_t>>> meshIndicesByChunk;

	for (size_t meshIndex = 0; meshIndex < modelData.meshes.size(); ++meshIndex) {
		const MeshData& meshData = modelData.meshes[meshIndex];
		if (!meshData.fractureInfo.has_value()) {
			continue;
		}
		meshIndicesByChunk[meshData.fractureInfo->groupName][meshData.fractureInfo->chunkId].push_back(meshIndex);

		auto& triangles = trianglesByMesh[meshIndex];
		triangles.resize(meshData.indices.size() / 3);
		for (size_t t = 0; t < triangles.size(); ++t) {
			TriangleGeometry& tri = triangles[t];
			for (int i = 0; i < 3; ++i) {
				const Vector4& pos = meshData.vertices[meshData.indices[t * 3 + i]].position;
				tri.p[i] = Vector3(pos.x, pos.y, pos.z);
			}
			Vector3 cross = Vector3::Cross(tri.p[1] - tri.p[0], tri.p[2] - tri.p[0]);
			float length = cross.Length();
			if (length < 1e-10f) {
				tri.isDegenerate = true;
				continue;
			}
			tri.normal = cross / length;
			tri.planeDist = tri.normal.Dot(tri.p[0]);
		}
	}

	// 法線がほぼ逆向きとみなす内積
	constexpr float kOppositeNormalDot = -0.98f;

	// pointがtriの上にあるか
	auto isPointOnTriangle = [threshold](const Vector3& point, const TriangleGeometry& tri) {
		if (std::abs(tri.normal.Dot(point) - tri.planeDist) > threshold) {
			return false;
		}
		for (int i = 0; i < 3; ++i) {
			const Vector3& a = tri.p[i];
			const Vector3& b = tri.p[(i + 1) % 3];
			Vector3 edge = b - a;
			// 辺の内側なら正になる。辺の長さで割って距離に直してから許容誤差と比べる
			float side = Vector3::Cross(edge, point - a).Dot(tri.normal);
			float edgeLength = edge.Length();
			if (edgeLength > 1e-10f && side / edgeLength < -threshold) {
				return false;
			}
		}
		return true;
		};

	ParallelFor(modelData.meshes.size(), [&](size_t meshIndex) {
		MeshData& meshData = modelData.meshes[meshIndex];
		if (!meshData.fractureInfo.has_value()) {
			return;
		}
		FractureChunkInfo& info = meshData.fractureInfo.value();
		const auto& chunkMeshes = meshIndicesByChunk.at(info.groupName);
		const auto& triangles = trianglesByMesh[meshIndex];

		info.triangleFacingChunkIds.assign(triangles.size(), kFractureExteriorFace);

		for (size_t t = 0; t < triangles.size(); ++t) {
			const TriangleGeometry& tri = triangles[t];
			if (tri.isDegenerate) {
				continue;
			}
			Vector3 centroid = (tri.p[0] + tri.p[1] + tri.p[2]) / 3.0f;

			// 隣接チャンクの中に、同じ平面上で逆向きの三角形が重なっていれば、その隣接チャンクとの切断面
			bool found = false;
			for (uint32_t neighborId : info.neighborChunkIds) {
				auto it = chunkMeshes.find(neighborId);
				if (it == chunkMeshes.end()) {
					continue;
				}
				for (size_t neighborMeshIndex : it->second) {
					for (const TriangleGeometry& other : trianglesByMesh[neighborMeshIndex]) {
						if (other.isDegenerate || tri.normal.Dot(other.normal) > kOppositeNormalDot) {
							continue;
						}
						if (isPointOnTriangle(centroid, other)) {
							info.triangleFacingChunkIds[t] = neighborId;
							found = true;
							break;
						}
					}
					if (found) { break; }
				}
				if (found) { break; }
			}
		}
		});

	// 判定結果をログに残す
	size_t exteriorCount = 0;
	size_t interiorCount = 0;
	for (const auto& meshData : modelData.meshes) {
		if (!meshData.fractureInfo.has_value()) {
			continue;
		}
		for (uint32_t facingId : meshData.fractureInfo->triangleFacingChunkIds) {
			if (facingId == kFractureExteriorFace) {
				++exteriorCount;
			} else {
				++interiorCount;
			}
		}
	}
	if (exteriorCount + interiorCount > 0) {
		LogManager::GetInstance().Log("ClassifyFractureFaces exterior=" + std::to_string(exteriorCount) + " interior=" + std::to_string(interiorCount));
	}
}
