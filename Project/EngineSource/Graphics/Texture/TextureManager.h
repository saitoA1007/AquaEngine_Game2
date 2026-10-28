#pragma once
#include <unordered_map>
#include "Texture.h"
#include "SrvManager.h"

namespace GameEngine {

	/// <summary>
	/// テクスチャの監理クラス
	/// </summary>
	class TextureManager final {
	public:
		TextureManager() = default;
		~TextureManager() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="commandList">コマンドリスト</param>
		/// <param name="srvManager">srvの管理クラス</param>
		void Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager);

		/// <summary>
		/// 解放処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// 登録する関数
		/// </summary>
		/// <param name="fileName">読み込む画像のファイルパス</param>
		void RegisterTexture(const std::string& fileName);

	public:

		/// <summary>
		/// 全てのテクスチャデータを読み込む
		/// </summary>
		void LoadAllTexture();

		/// <summary>
		/// デバック時に使用する全てのテクスチャデータを読み込む
		/// </summary>
		void LoadDebugAllTexture();

		/// <summary>
		/// 名前からハンドルを取得
		/// </summary>
		/// <param name="name">登録名</param>
		/// <returns>ハンドル</returns>
		uint32_t GetHandleByName(const std::string& name) const;

		// GPUハンドルを取得する
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandlesGPU(const uint32_t& textureHandle);

		/// <summary>
		/// 登録されている全てテクスチャの名前を取得
		/// </summary>
		/// <returns></returns>
		std::vector<std::string> GetRegisteredTextureNames() const;

	private:
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		ID3D12GraphicsCommandList* commandList_ = nullptr;
		SrvManager* srvManager_ = nullptr;

		// 読み取り先のパス名
		static inline const std::string kDirectoryPath = "Resources/Textures/";
		// デバック時に使用する画像の読み取り先のパス名
		static inline const std::string kDebugDirectoryPath = "EngineSource/Resources/Textures/";

		// テクスチャ情報
		std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;

	private:

		/// <summary>
		/// フルパスからファイル名を取得
		/// </summary>
		/// <param name="fullPath"></param>
		/// <returns></returns>
		std::string GetFileName(const std::string& fullPath);
	};
}
