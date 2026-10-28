#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "Font.h"

namespace GameEngine {

	/// <summary>
	/// SDFフォントの管理クラス
	/// </summary>
	class FontManager final {
	public:
		FontManager() = default;
		~FontManager() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="commandList">テクスチャ転送用のコマンドリスト</param>
		void Initialize(ID3D12GraphicsCommandList* commandList);

		/// <summary>
		/// 解放処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// Resources/Text/Generated にある全てのフォントを読み込む
		/// </summary>
		void LoadAllFont();

		/// <summary>
		/// フォントを登録する
		/// </summary>
		/// <param name="name">登録名</param>
		/// <param name="jsonPath">JSONのパス(UTF-8)</param>
		/// <param name="imagePath">PNGのパス(UTF-8)</param>
		void RegisterFont(const std::string& name, const std::string& jsonPath, const std::string& imagePath);

		/// <summary>
		/// 名前からフォントを取得。見つからない場合はnullptr
		/// </summary>
		const Font* GetFont(const std::string& name) const;

		/// <summary>
		/// 登録されている全てのフォント名を取得
		/// </summary>
		std::vector<std::string> GetRegisteredFontNames() const;

	private:
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;

		ID3D12GraphicsCommandList* commandList_ = nullptr;

		// 読み取り先のパス名
		static inline const std::string kDirectoryPath = "Resources/Text/Generated/";

		// フォント情報
		std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
	};
}
