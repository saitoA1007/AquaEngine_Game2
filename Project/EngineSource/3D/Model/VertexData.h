#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "FractureData.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <array>
#include <map>
#include <optional>
#include <span>

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
	Vector4 tangent;
};

struct LoadMaterialData {
	std::string textureFilePath;
	Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
	Vector3 specularColor = { 1.0f,1.0f,1.0f };
	float shininess = 0.0f;
	std::string name;
};

struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

struct MeshData {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	std::string materialName; // 使用するマテリアル名
	std::optional<GameEngine::FractureChunkInfo> fractureInfo; // 事前分割チャンク情報
};

// ランタイムカットで作成した1個の破片
struct Fragment {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
};

struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

struct JointWeightData {
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

struct ModelData {
	std::vector<MeshData> meshes;
	std::vector<LoadMaterialData> materials;
	Node rootNode;
	std::vector<std::unordered_map<std::string, JointWeightData>> skinClusterData;
	bool isAnimation_ = false;
	bool isSkeleton = false;
};

struct GridVertexData {
	Vector4 position;
};

struct Joint {
	QuaternionTransform transform; // Transform情報
	Matrix4x4 localMatrix; /// localMatrix
	Matrix4x4 skeletonSpaceMatrix; // skeletonSpaceでの変換行列
	std::string name; // 名前
	std::vector<int32_t> children; // 子JointのIndexのリスト。いなければ空
	int32_t index; // 自身のIndex
	std::optional<int32_t> parent; // 親JointのIndex。いなければnullptr
};

struct SkeletonData {
	int32_t root; // RootJointのIndex
	std::map<std::string, int32_t> jointMap; // Joint名とIndexとの辞書
	std::vector<Joint> joints; // 所属しているジョイント
};

