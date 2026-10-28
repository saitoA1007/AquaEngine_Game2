#include "pch.h"
#include "Text.h"
#include <algorithm>
#include "MyMath.h"
#include "Sprite.h"
#include "LogManager.h"

using namespace GameEngine;

namespace {

	// UTF-8の文字列をコードポイントに分解する
	std::u32string DecodeUtf8(const std::string& text) {
		std::u32string result;
		result.reserve(text.size());

		size_t i = 0;
		while (i < text.size()) {
			uint8_t c = static_cast<uint8_t>(text[i]);
			char32_t codepoint = 0;
			size_t length = 0;

			if (c < 0x80) { codepoint = c; length = 1; }
			else if ((c & 0xE0) == 0xC0) { codepoint = c & 0x1F; length = 2; }
			else if ((c & 0xF0) == 0xE0) { codepoint = c & 0x0F; length = 3; }
			else if ((c & 0xF8) == 0xF0) { codepoint = c & 0x07; length = 4; }
			else { ++i; continue; } // 不正なバイトは読み飛ばす

			if (i + length > text.size()) { break; }

			for (size_t j = 1; j < length; ++j) {
				codepoint = (codepoint << 6) | (static_cast<uint8_t>(text[i + j]) & 0x3F);
			}
			result.push_back(codepoint);
			i += length;
		}
		return result;
	}
}

Text::Text(const Font* font, const std::string& text, uint32_t maxLength) {
	font_ = font;
	maxLength_ = (std::max)(maxLength, 1u);

	// 文字データのバッファを作成
	glyphBuffer_.Create(maxLength_);

	// 定数バッファを作成
	constBuffer_.Create();
	constBufferData_ = constBuffer_.GetData();
	constBufferData_->WVP = Matrix4x4::MakeIdentity();
	constBufferData_->color = color_;
	constBufferData_->atlasSize = { 1.0f,1.0f };
	constBufferData_->distanceRange = 1.0f;
	constBufferData_->textureHandle = 0;

	SetText(text);
}

void Text::Update() {
	if (!font_) { return; }

	// フォントの情報
	constBufferData_->color = color_;
	constBufferData_->atlasSize = { font_->GetAtlasWidth(), font_->GetAtlasHeight() };
	constBufferData_->distanceRange = font_->GetDistanceRange();
	constBufferData_->textureHandle = font_->GetTextureHandle();

	// 更新
	Matrix4x4 anchorMatrix = Matrix4x4::MakeTranslateMatrix({ -anchorPoint_.x * boundsEm_.x, -anchorPoint_.y * boundsEm_.y, 0.0f });
	Matrix4x4 worldMatrix = anchorMatrix * Matrix4x4::MakeAffineMatrix(
		Vector3(scale_.x, scale_.y, 1.0f), Vector3(0.0f, 0.0f, rotate_), Vector3(position_.x, position_.y, 0.0f));
	if (parent_) {
		worldMatrix *= parent_->GetWorldMatrix();
	}
	constBufferData_->WVP = worldMatrix * Sprite::GetOrthoMatrix();
}

void Text::SetText(const std::string& text) {
	text_ = text;
	BuildGlyphs();
}

void Text::SetFont(const Font* font) {
	font_ = font;
	BuildGlyphs();
}

void Text::SetLetterSpacing(float letterSpacing) {
	letterSpacing_ = letterSpacing;
	BuildGlyphs();
}

void Text::SetLineSpacing(float lineSpacing) {
	lineSpacing_ = lineSpacing;
	BuildGlyphs();
}

void Text::BuildGlyphs() {
	glyphCount_ = 0;
	boundsEm_ = { 0.0f,0.0f };
	if (!font_) { return; }

	GlyphForGPU* glyphData = glyphBuffer_.GetData();
	const std::u32string codepoints = DecodeUtf8(text_);

	// 1行目の上端が0になるようにベースラインを置く
	float penX = 0.0f;
	float baseline = -font_->GetAscender();
	float maxWidth = 0.0f;
	// 行末の文字間隔を含まない行の幅
	float lineWidth = 0.0f;
	uint32_t lineCount = 1;
	char32_t prev = 0;

	// 1行分の送り幅
	const float lineAdvance = font_->GetLineHeight() + lineSpacing_;

	const Font::Glyph* fallback = font_->FindGlyph(U'?');
	const Font::Glyph* space = font_->FindGlyph(U' ');

	for (char32_t codepoint : codepoints) {
		// 改行
		if (codepoint == U'\n') {
			maxWidth = (std::max)(maxWidth, lineWidth);
			penX = 0.0f;
			lineWidth = 0.0f;
			baseline += lineAdvance;
			++lineCount;
			prev = 0;
			continue;
		}
		if (codepoint == U'\r') { continue; }
		if (codepoint == U'\t') {
			penX += space ? space->advance * 4.0f : 0.0f;
			lineWidth = penX;
			penX += letterSpacing_;
			prev = 0;
			continue;
		}

		// フォントに無い文字は'?'で代用する
		const Font::Glyph* glyph = font_->FindGlyph(codepoint);
		if (!glyph) {
			glyph = fallback;
			if (!glyph) { continue; }
		}

		penX += font_->GetKerning(prev, codepoint);

		if (glyph->hasQuad) {
			if (glyphCount_ >= maxLength_) {
				LogManager::GetInstance().Log("Text exceeds maxLength (" + std::to_string(maxLength_) + ") : " + text_);
				break;
			}
			GlyphForGPU& data = glyphData[glyphCount_++];
			data.rect = {
				penX + glyph->planeBounds.x, baseline + glyph->planeBounds.y,
				penX + glyph->planeBounds.z, baseline + glyph->planeBounds.w };
			data.uv = glyph->uvBounds;
		}

		penX += glyph->advance;
		lineWidth = penX;
		// 次の文字との間隔
		penX += letterSpacing_;
		prev = codepoint;
	}

	maxWidth = (std::max)(maxWidth, lineWidth);
	// 最後の行の下には行間を入れない
	boundsEm_ = { maxWidth, static_cast<float>(lineCount - 1) * lineAdvance + font_->GetLineHeight() };
}
