#pragma once
#include <vector>
#include <unordered_map>

#include "Mesh.h"
#include "Material.h"
#include "TransformationMatrix.h"
#include "Skeleton.h"
#include "RefBuffer.h"
#include "PackedGeometryBuffer.h"

namespace GameEngine {

	struct FractureChunkEntry {
		std::string materialName = "default";
		GeometryRange range;
		FractureChunkInfo info;
	};
	
	class Model final {
	public:
		Model() = default;
		~Model() = default;

		// メッシュを追加
		void AddMesh(std::unique_ptr<Mesh> mesh) {
			meshes_.push_back(std::move(mesh));
		}

		// マテリアルを追加
		void AddMaterial(const std::string& name, std::unique_ptr<Material> material) {
			materials_[name] = std::move(material);
		}

		// 作成したMeshを元にBLASを作成する
		void AddBLAS(ID3D12GraphicsCommandList4* cmdList,const bool& isUpdate) {
			for (auto& mesh : meshes_) {
				mesh->CreateBLAS(cmdList, isUpdate);
			}
		}

		// 破壊オブジェクトのグループを追加する
		void AddFractureGroup(const std::string& groupName, std::unique_ptr<PackedGeometryBuffer> buffer, std::vector<FractureChunkEntry> chunks) {
			fractureBuffers_[groupName] = std::move(buffer);
			fractureChunks_[groupName] = std::move(chunks);
		}

		// 外部読み込み用のデータを設定
		void SetLoadModelData(const Node& node) {
			isLoad_ = true;
			modelName_ = node.name;
			node_ = node;
		}

		// ボーンデータを追加
		void SetSkeleton(std::unique_ptr<Skeleton> skeleton) {
			isSkeleton_ = true;
			skeleton_ = std::move(skeleton);
		}

		// 参照用データを作成
		void CreateRefBuffer() {

			// 参照の値を取得
			refBuffers_.resize(meshes_.size());

			// 参照用のデータを作成
			for (uint32_t i = 0; i < meshes_.size(); ++i) {
				refBuffers_[i].Create();
				uint32_t vertexHandle = 0;
				// スケルトンがあれば参照するデータを変える
				if (isSkeleton_) {
					vertexHandle = skeleton_->GetOutputVertexBufferSrvIndex(i);;
				} else {
					vertexHandle = meshes_[i]->GetVertexBufferSrvIndex();
				}		

				// モデルデータを設定
				refBuffers_[i].SetModelData(vertexHandle, meshes_[i]->GetIndexBufferSrvIndex());

				// マテリアルデータを設定
				Material* drawMaterial = materials_[meshes_[i]->GetMaterialName()].get();
				refBuffers_[i].SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), drawMaterial->GetMaterialSrvIndex());
			}
		}

	public:

		/// <summary>
		/// デフォルトの色を設定
		/// </summary>
		/// <param name="color"></param>
		void SetDefaultColor(const Vector4& color,const std::string& materialName = "default");

		/// <summary>
		/// 鏡面反射の色を設定
		/// </summary>
		/// <param name="specularColor"></param>
		void SetDefaultSpecularColor(const Vector3& specularColor, const std::string& materialName = "default");

		/// <summary>
		/// 輝度の設定
		/// </summary>
		/// <param name="shininess"></param>
		void SetDefaultShininess(const float& shininess, const std::string& materialName = "default");

		/// <summary>
		/// デフォオルトの光源の有無を設定
		/// </summary>
		/// <param name="isEnableLight"></param>
		void SetDefaultIsEnableLight(const bool& isEnableLight, const std::string& materialName = "default");

		/// <summary>
		/// 影の適応の有無を設定
		/// </summary>
		/// <param name="isEnableLight"></param>
		/// <param name="materialName"></param>
		void SetDefaultIsEnableShadow(const bool& isEnableShadow, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトのuvMatrixを設定
		/// </summary>
		/// <param name="uvMatrix"></param>
		void SetDefaultUVMatrix(const Matrix4x4& uvMatrix, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトのuvMatrixを設定
		/// </summary>
		/// <param name="uvTransform"></param>
		/// <param name="index"></param>
		void SetDefaultUVMatrix(const Transform& uvTransform, const std::string& materialName = "default");

		/// <summary>
		/// /デフォルトのテクスチャを設定
		/// </summary>
		/// <param name="handle"></param>
		/// <param name="materialName"></param>
		void SetDefaultTextureHandle(const uint32_t& handle, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトの反射率を設定
		/// </summary>
		/// <param name="metallic"></param>
		/// <param name="materialName"></param>
		void SetDefaultMetallic(const float& metallic, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトの屈折率を設定
		/// </summary>
		/// <param name="ior"></param>
		/// <param name="materialName"></param>
		void SetDefaultIOR(const float& ior, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトの粗さの設定
		/// </summary>
		/// <param name="roughness"></param>
		/// <param name="materialName"></param>
		void SetRoughness(const float& roughness, const std::string& materialName = "default");

		/// <summary>
		/// デフォルトのノーマルマップの設定
		/// </summary>
		/// <param name="texture"></param>
		/// <param name="materialName"></param>
		void SetDefaultNormalTexture(const uint32_t& texture, const std::string& materialName = "default");

		/// <summary>
		/// モデルの名前を取得
		/// </summary>
		/// <returns></returns>
		const std::string GetModelName() const { return modelName_; }

		const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const { return meshes_; }
		Material* GetMaterial(const std::string& name) const;
		std::unordered_map<std::string, std::unique_ptr<Material>>& GetMaterials() { return materials_; }

		// ローカル行列
		Matrix4x4 GetLocalMatrix() const {return node_.localMatrix;}

		// ノードを取得
		Node& GetNodes() { return node_; }

		// ロードしているか
		const bool IsLoad() const { return isLoad_; }
		// ボーンが存在しているか
		const bool IsSkeleton() const { return isSkeleton_; }

		// ボーンデータを取得
		Skeleton* GetSkeleton() { return skeleton_.get(); }
		const Skeleton* GetSkeleton() const { return skeleton_.get(); }

		// 参照用バッファ
		std::vector<RefBuffer>& GetRefBuffers() {return refBuffers_; }

		// モデルの寸法
		const Vector3& GetModelSize() const { return modelSize_; }

		// 破壊グループのPackedGeometryBufferを取得
		const std::unordered_map<std::string, std::unique_ptr<PackedGeometryBuffer>>& GetFractureBuffers() const { return fractureBuffers_; }
		// 破壊グループのチャンク一覧を取得
		const std::unordered_map<std::string, std::vector<FractureChunkEntry>>& GetFractureChunks() const { return fractureChunks_; }

	private:
		Model(Model&) = delete;
		Model& operator=(Model&) = delete;

		// 複数メッシュに対応
		std::vector<std::unique_ptr<Mesh>> meshes_;

		// 複数マテリアルに対応
		std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

		// ボーンデータ
		std::unique_ptr<Skeleton> skeleton_;

		// 参照用バッファ
		std::vector<RefBuffer> refBuffers_;

		// 破壊オブジェクトのグループ
		std::unordered_map<std::string, std::unique_ptr<PackedGeometryBuffer>> fractureBuffers_;
		std::unordered_map<std::string, std::vector<FractureChunkEntry>> fractureChunks_;

		// モデル自体の寸法
		Vector3 modelSize_ = {0.0f,0.0f,0.0f};

		// Nodeのローカル行列を保持しておく変数
		Node node_;
		// 外部からロードされたか
		bool isLoad_ = false;
		bool isSkeleton_ = false;
		// モデルの名前
		std::string modelName_ = "NoName";
	};
}