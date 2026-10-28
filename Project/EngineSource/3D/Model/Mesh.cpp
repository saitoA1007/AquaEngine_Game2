#include "pch.h"
#include "Mesh.h"
#include <cassert>
#include <numbers>

using namespace GameEngine;

void Mesh::CreateTrianglePlaneMesh() {

	std::vector<VertexData> vertices(3);
	vertices[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertices[0].texcoord = { 0.0f, 1.0f };
	vertices[0].normal = { 0.0f, 0.0f, -1.0f };
	// 上
	vertices[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertices[1].texcoord = { 0.5f, 0.0f };
	vertices[1].normal = { 0.0f, 0.0f, -1.0f };
	// 右下
	vertices[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertices[2].texcoord = { 1.0f, 1.0f };
	vertices[2].normal = { 0.0f, 0.0f, -1.0f };

	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreateGridPlaneMesh(const Vector2& size) {
	std::vector<VertexData> vertices(4);

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertices[0].position = { left,0.0f,top,1.0f }; // 左下
	// 右上
	vertices[1].position = { right,0.0f,top,1.0f }; // 左上
	// 左下
	vertices[2].position = { left,0.0f,bottom,1.0f }; // 右下
	// 右下
	vertices[3].position = { right,0.0f,bottom,1.0f }; // 左上

	// インデックスデータを生成
	std::vector<uint32_t> indices(6);
	// 三角形
	indices[0] = 0;  indices[1] = 1;  indices[2] = 2;
	// 三角形2
	indices[3] = 1;  indices[4] = 3;  indices[5] = 2;

	// インデックスデータを作成
	indexBuffer_.Create(indices);
	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreatePlaneMesh(const Vector2& size) {

	std::vector<VertexData> vertices(4);

	float left = -size.x / 2.0f;
	float right = size.x / 2.0f;
	float top = size.y / 2.0f;
	float bottom = -size.y / 2.0f;

	// 左上
	vertices[0].position = { left,top,0.0f,1.0f };
	vertices[0].texcoord = { 0.0f,0.0f };
	vertices[0].normal = { 0.0f,0.0f,-1.0f };
	// 右上
	vertices[1].position = { right,top,0.0f,1.0f };
	vertices[1].texcoord = { 0.0f,0.0f };
	vertices[1].normal = { 0.0f,0.0f,-1.0f };
	// 左下
	vertices[2].position = { left,bottom,0.0f,1.0f };
	vertices[2].texcoord = { 0.0f,0.0f };
	vertices[2].normal = { 0.0f,0.0f,-1.0f };
	// 右下
	vertices[3].position = { right,bottom,0.0f,1.0f };
	vertices[3].texcoord = { 0.0f,0.0f };
	vertices[3].normal = { 0.0f,0.0f,-1.0f };

	// インデックスデータを生成
	std::vector<uint32_t> indices(6);

	// 三角形
	indices[0] = 0;  indices[1] = 1; indices[2] = 2;
	// 三角形2
	indices[3] = 1;  indices[4] = 3;  indices[5] = 2;

	// インデックスデータを作成
	indexBuffer_.Create(indices);
	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreateSphereMesh(uint32_t subdivision) {
	
	const float kLatEvery = std::numbers::pi_v<float> / static_cast<float>(subdivision);
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / static_cast<float>(subdivision);

	// 頂点生成
	std::vector<VertexData> vertices;
	for (uint32_t latIndex = 0; latIndex <= subdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - static_cast<float>(latIndex) / static_cast<float>(subdivision);
		for (uint32_t lonIndex = 0; lonIndex <= subdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			float u = static_cast<float>(lonIndex) / static_cast<float>(subdivision);

			VertexData vertex;
			vertex.position = { std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 1.0f };
			vertex.texcoord = { u, v };
			vertex.normal = { vertex.position.x, vertex.position.y, vertex.position.z };
			vertices.push_back(vertex);
		}
	}

	// インデックス生成
	std::vector<uint32_t> indices;
	for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
			uint32_t a = (latIndex * (subdivision + 1)) + lonIndex;
			uint32_t b = a + subdivision + 1;
			uint32_t c = a + 1;
			uint32_t d = b + 1;
			indices.insert(indices.end(), { a, b, d, a, d, c });
		}
	}

	// インデックスデータを作成
	indexBuffer_.Create(indices);
	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreateRingMesh(uint32_t ringDivide, float outerRadius, float innerRadius) {

	// 頂点データ
	std::vector<VertexData> vertices;
	vertices.reserve((ringDivide + 1) * 2);

	// インデックスデータ
	std::vector<uint32_t> indices;
	indices.reserve(ringDivide * 6);

	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(ringDivide);

	// 頂点データを生成する
	for (uint32_t i = 0; i < ringDivide; ++i) {
		float angle = static_cast<float>(i) * radianPerDivide;
		float sin = std::sinf(angle);
		float cos = std::cosf(angle);

		// u座標
		float u = static_cast<float>(i) / static_cast<float>(ringDivide);

		// 内側の頂点
		VertexData innerVertex;
		innerVertex.position = { cos * innerRadius,sin * innerRadius,0.0f,1.0f };
		innerVertex.texcoord = { u,1.0f };
		innerVertex.normal = { 0.0f, 0.0f, 1.0f };
		vertices.push_back(innerVertex);

		// 外側の頂点
		VertexData outerVertex;
		outerVertex.position = { cos * outerRadius,sin * outerRadius,0.0f,1.0f };
		outerVertex.texcoord = { u,0.0f };
		outerVertex.normal = { 0.0f,0.0f,1.0f };
		vertices.push_back(outerVertex);
	}

	// インデックスデータを生成する
	for (uint32_t i = 0; i < ringDivide; ++i) {
		// 現在のインデックス番号
		uint32_t innerCurr = i * 2;
		uint32_t outerCurr = i * 2 + 1;
		// 次のインデックス番号
		uint32_t innerNext = ((i + 1) % ringDivide) * 2;
		uint32_t outerNext = ((i + 1) % ringDivide) * 2 + 1;
	
		// 三角形1枚目
		indices.push_back(innerCurr);
		indices.push_back(outerCurr);
		indices.push_back(innerNext);
	
		// 三角形2枚目
		indices.push_back(innerNext);
		indices.push_back(outerCurr);
		indices.push_back(outerNext);
	}

	// インデックスデータを作成
	indexBuffer_.Create(indices);
	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreateCylinder(uint32_t cylinderDivide, float topRadius, float bottomRadius, float height) {

	// 頂点データ
	std::vector<VertexData> vertices;
	
	// インデックスデータ
	std::vector<uint32_t> indices;

	float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(cylinderDivide);
	float halfHeight = height / 2.0f;

	// 側面の傾きに応じた法線ベクトルの比率計算
	float slopeX = bottomRadius - topRadius;
	float slopeY = height;
	float slopeLength = std::sqrtf(slopeX * slopeX + slopeY * slopeY);
	float normalXZ = slopeY / slopeLength;
	float normalY = slopeX / slopeLength;

	for (uint32_t index = 0; index < cylinderDivide; ++index) {
		float i = static_cast<float>(index);
		float iNext = static_cast<float>(index + 1);

		float sin = std::sinf(i * radianPerDivide);
		float cos = std::cosf(i * radianPerDivide);
		float sinNext = std::sinf(iNext * radianPerDivide);
		float cosNext = std::cosf(iNext * radianPerDivide);
		float u = i / static_cast<float>(cylinderDivide);
		float uNext = iNext / static_cast<float>(cylinderDivide);

		// 側面の頂点
		uint32_t sideBaseIndex = static_cast<uint32_t>(vertices.size());

		VertexData tmpVertex;

		// 左下
		tmpVertex.position = { cos * bottomRadius, -halfHeight, sin * bottomRadius, 1.0f };
		tmpVertex.texcoord = { u, 1.0f };
		tmpVertex.normal = { cos * normalXZ, normalY, sin * normalXZ };
		vertices.push_back(tmpVertex);
		// 左上
		tmpVertex.position = { cos * topRadius, halfHeight, sin * topRadius, 1.0f };
		tmpVertex.texcoord = { u, 0.0f };
		tmpVertex.normal = { cos * normalXZ, normalY, sin * normalXZ };
		vertices.push_back(tmpVertex);
		// 右下
		tmpVertex.position = { cosNext * bottomRadius, -halfHeight, sinNext * bottomRadius, 1.0f };
		tmpVertex.texcoord = { uNext, 1.0f };
		tmpVertex.normal = { cosNext * normalXZ, normalY, sinNext * normalXZ };
		vertices.push_back(tmpVertex);
		// 右上
		tmpVertex.position = { cosNext * topRadius, halfHeight, sinNext * topRadius, 1.0f };
		tmpVertex.texcoord = { uNext, 0.0f };
		tmpVertex.normal = { cosNext * normalXZ, normalY, sinNext * normalXZ };
		vertices.push_back(tmpVertex);

		// 側面のインデックス
		// 左下、左上、右上
		indices.push_back(sideBaseIndex + 0);
		indices.push_back(sideBaseIndex + 1);
		indices.push_back(sideBaseIndex + 3);
		// 左下、右上、右下
		indices.push_back(sideBaseIndex + 0);
		indices.push_back(sideBaseIndex + 3);
		indices.push_back(sideBaseIndex + 2);
	}

	// インデックスデータを作成
	indexBuffer_.Create(indices);
	// 頂点データを作成
	vertexBuffer_.Create(vertices);
}

void Mesh::CreateModelMesh(ModelData modelData, const uint32_t& index) {

	// 要素が無ければエラー
	assert(modelData.meshes.size() > index);

	const auto& meshData = modelData.meshes[index];
	materialName_ = meshData.materialName;

	// インデックスデータを作成
	indexBuffer_.Create(meshData.indices);
	// 頂点データを作成
	vertexBuffer_.Create(meshData.vertices);
}

void Mesh::CreateBLAS(ID3D12GraphicsCommandList4* cmdList, const bool& isUpdate) {
	blas_ = std::make_unique<BLAS>();
	blas_->Create(cmdList, GetVertexBufferView(), GetIndexBufferView(), GetTotalVertices(), GetTotalIndices(), isUpdate);
}