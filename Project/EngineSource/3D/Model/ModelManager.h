#pragma once
#include <vector>
#include <future>
#include <unordered_map>
#include "ModelLoader.h"
#include "Model.h"

namespace GameEngine {

	class ModelManager final {
	public:

		// 登録するデータ
		struct ModelEntryData {
			std::string name; // ロードしたモデルファイル名(.obj,gltfなど)
			std::unique_ptr<Model> model; // モデルデータ
		};

		// ロードするモデルファイルの情報
		struct ModelFile {
			std::string folderPath;
			std::string fileName;
		};

	public:

		ModelManager() = default;
		~ModelManager();

		void Initialize(ID3D12GraphicsCommandList4* cmdList, TextureManager* textureManager, SrvManager* srvManager);

		/// <summary>
		/// CPUでおこなうgltfやobjファイルからモデル情報を全てロードする
		/// </summary>
		void BeginLoadAllModel();

		/// <summary>
		/// GPUリソース作成と登録
		/// </summary>
		void FinishLoadAllModel();

		/// <summary>
		/// マテリアルで使用するテクスチャのハンドルを取得する
		/// </summary>
		/// <param name="textureManager"></param>
		void ApplyTexutre(TextureManager* textureManager);

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelFile">モデルファイル名 : 登録名にもなる</param>
		/// <param name="modelName">.obj名</param>
		void RegisterModel(const std::string& modelFile,const std::string& objFileName);

		/// <summary>
		/// モデルデータを登録
		/// </summary>
		/// <param name="modelName">登録したいモデル名</param>
		/// <param name="model">モデルデータ</param>
		void RegisterModel(const std::string& modelName, std::unique_ptr<Model> model);

		/// <summary>
		/// 
		/// 辻褄合わせで作ったので後で削除するように
		/// 
		/// </summary>
		/// <param name="modelName"></param>
		void RegisterGridPlaneModel(const std::string& modelName,const Vector2& size);

		/// <summary>
		/// リングモデルを登録
		/// </summary>
		void RegisterRingModel(const std::string& modelName, uint32_t ringDivide, float outerRadius, float innerRadius);

		/// <summary>
		/// 円柱モデルを登録する
		/// </summary>
		void RegisterCylinderModel(const std::string& modelName, uint32_t cylinderDivide, float topRadius, float bottomRadius, float height);

		/// <summary>
		/// 登録を外す
		/// </summary>
		/// <param name="handle"></param>
		void UnregisterModel(uint32_t handle);

		/// <summary>
		/// モデルの名前からハンドルを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		uint32_t GetHandleByName(const std::string& name) const;

		/// <summary>
		/// ハンドルからモデルの名前を取得
		/// </summary>
		/// <param name="handle"></param>
		/// <returns></returns>
		std::string GetNameByHandle(uint32_t handle) const;

		/// <summary>
		/// ハンドルからモデルを取得
		/// </summary>
		/// <param name="handle"></param>
		/// <returns></returns>
		[[nodiscard]]
		Model* GetHandleByModel(uint32_t handle) const;

		/// <summary>
		/// 名前からモデルを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		[[nodiscard]]
		Model* GetNameByModel(const std::string& name) const;

		/// <summary>
		/// モデルデータ達
		/// </summary>
		/// <returns></returns>
		std::unordered_map<uint32_t, ModelEntryData>& GetModels() { return models_; }

	private:
		ModelManager(ModelManager&) = delete;
		ModelManager& operator=(ModelManager&) = delete;

		// モデル生成
		ModelLoader loader_;

		// ハンドルからモデルデータを保存
		std::unordered_map<uint32_t, ModelEntryData> models_;
		// モデルデータの名前からハンドルを保存
		std::unordered_map<std::string, uint32_t> nameToHandles_;

		// 次のハンドル
		uint32_t nextHandle_ = 1;

		std::vector<ModelFile> loadFiles_;
		std::vector<std::unique_ptr<ModelData>> parsedModels_;
		std::future<void> parseFuture_;

	private:

		/// <summary>
		/// 読み込むファイル一覧を取得
		/// </summary>
		/// <returns></returns>
		std::vector<ModelFile> EnumerateModelFiles() const;
	};
}