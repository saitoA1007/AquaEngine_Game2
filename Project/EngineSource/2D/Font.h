#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <d3d12.h>
#include "Vector4.h"
#include "Texture.h"

namespace GameEngine {

	/// <summary>
	/// msdf-atlas-genで生成したSDFフォントのデータ
	/// </summary>
	class Font final {
	public:

		// 1文字分のデータ
		struct Glyph {
			float advance = 0.0f; // 次の文字までの送り幅
			Vector4 planeBounds{}; // ベースライン基準の矩形 (left, top, right, bottom)
			Vector4 uvBounds{};    // アトラス内のUV矩形 (left, top, right, bottom)
			bool hasQuad = false;  // 描画するかの判断。スペースなどは描画しない
		};

	public:
		Font() = default;
		~Font() = default;

		/// <summary>
		/// フォントデータを読み込む
		/// </summary>
		/// <param name="jsonPath">msdf-atlas-genが出力したJSONのパス</param>
		/// <param name="imagePath">msdf-atlas-genが出力したPNGのパス</param>
		/// <param name="cmdList">テクスチャ転送用のコマンドリスト</param>
		/// <returns>読み込みに成功したか</returns>
		bool Load(const std::string& jsonPath, const std::string& imagePath, ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// 文字のデータを取得する。存在しない場合はnullptr
		/// </summary>
		const Glyph* FindGlyph(char32_t codepoint) const;

		/// <summary>
		/// 2文字間のカーニング量を取得する
		/// </summary>
		float GetKerning(char32_t left, char32_t right) const;

		uint32_t GetTextureHandle() const { return texture_->GetSrvIndex(); }
		float GetDistanceRange() const { return distanceRange_; }
		float GetAtlasWidth() const { return atlasWidth_; }
		float GetAtlasHeight() const { return atlasHeight_; }
		float GetLineHeight() const { return lineHeight_; }
		float GetAscender() const { return ascender_; }
		float GetDescender() const { return descender_; }

	private:
		Font(const Font&) = delete;
		Font& operator=(const Font&) = delete;

		// アトラス画像
		std::unique_ptr<Texture> texture_;

		// アトラス情報
		float distanceRange_ = 2.0f;
		float atlasWidth_ = 1.0f;
		float atlasHeight_ = 1.0f;

		// フォントのメトリクス
		float lineHeight_ = 1.0f;
		float ascender_ = -1.0f;
		float descender_ = 0.0f;

		// 文字データ
		std::unordered_map<char32_t, Glyph> glyphs_;
		// カーニング [(左の文字 << 32) | 右の文字] -> 送り幅の補正
		std::unordered_map<uint64_t, float> kerning_;
	};
}
