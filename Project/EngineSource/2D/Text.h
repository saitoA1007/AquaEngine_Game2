#pragma once
#include <string>
#include "Vector2.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "WorldTransform.h"
#include "Font.h"

namespace GameEngine {

	/// <summary>
	/// SDFフォントを使ったテキスト描画
	/// </summary>
	class Text final {
	public:

		// 1文字分のGPUデータ
		struct GlyphForGPU {
			Vector4 rect; // テキスト内の矩形 (left, top, right, bottom)
			Vector4 uv;   // アトラス内のUV矩形 (left, top, right, bottom)
		};

		// 定数バッファ
		struct ConstBufferData {
			Matrix4x4 WVP;
			Vector4 color;
			Vector2 atlasSize;
			float distanceRange;
			uint32_t textureHandle;
		};

	public:
		/// <param name="font">使用するフォント</param>
		/// <param name="text">表示する文字列(UTF-8)</param>
		/// <param name="maxLength">表示できる最大文字数</param>
		Text(const Font* font, const std::string& text = "", uint32_t maxLength = 16);
		~Text() = default;

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

	public:

		/// <summary>
		/// 表示する文字列を設定
		/// </summary>
		void SetText(const std::string& text);

		/// <summary>
		/// 使用するフォントを設定
		/// </summary>
		void SetFont(const Font* font);

		const std::string& GetText() const { return text_; }

		/// <summary>
		/// 文字間隔を設定 (em単位。0で標準、0.1で1文字の10%分広がる)
		/// </summary>
		void SetLetterSpacing(float letterSpacing);
		float GetLetterSpacing() const { return letterSpacing_; }

		/// <summary>
		/// 行間を設定 (em単位。0で標準)
		/// </summary>
		void SetLineSpacing(float lineSpacing);
		float GetLineSpacing() const { return lineSpacing_; }

		/// <summary>
		/// テキスト全体の大きさ
		/// </summary>
		Vector2 GetSize() const { return { boundsEm_.x * scale_.x, boundsEm_.y * scale_.y }; }

		// 親を設定
		void SetParent(WorldTransform* parent) { parent_ = parent; }

		// 描画用
		ID3D12Resource* GetResource() const { return constBuffer_.GetResource(); }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetGlyphSrvHandle() const { return glyphBuffer_.GetSrvGpuHandle(); }
		uint32_t GetGlyphCount() const { return glyphCount_; }

	public: // 変数

		// 座標
		Vector2 position_{};
		// 回転
		float rotate_ = 0.0f;
		// スケール
		Vector2 scale_ = { 32.0f,32.0f };
		// 色
		Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
		// アンカーポイント
		Vector2 anchorPoint_{};

	private:
		Text(const Text&) = delete;
		Text& operator=(const Text&) = delete;

		// 使用するフォント
		const Font* font_ = nullptr;

		// 表示する文字列
		std::string text_;

		// 文字間隔、行間
		float letterSpacing_ = 0.0f;
		float lineSpacing_ = 0.0f;

		// 親
		WorldTransform* parent_ = nullptr;

		// 文字のデータ
		StructuredBuffer<GlyphForGPU> glyphBuffer_;
		uint32_t maxLength_ = 0;
		uint32_t glyphCount_ = 0;
		// テキスト全体の大きさ
		Vector2 boundsEm_{};

		ConstantBuffer<ConstBufferData> constBuffer_;
		ConstBufferData* constBufferData_ = nullptr;

	private:

		/// <summary>
		/// 文字を並べてGPUに送る
		/// </summary>
		void BuildGlyphs();
	};
}
