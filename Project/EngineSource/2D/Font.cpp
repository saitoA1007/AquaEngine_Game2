#include "pch.h"
#include "Font.h"
#include <fstream>
#include <filesystem>
#include <json.hpp>
#include "LogManager.h"
using namespace GameEngine;

namespace {
	uint64_t MakeKerningKey(char32_t left, char32_t right) {
		return (static_cast<uint64_t>(left) << 32) | static_cast<uint64_t>(right);
	}
}

bool Font::Load(const std::string& jsonPath, const std::string& imagePath, ID3D12GraphicsCommandList* cmdList) {
	LogManager::GetInstance().Log("Start LoadFont : " + jsonPath);

	// 日本語のファイル名に対応するためUTF-8としてパスを解釈する
	std::ifstream file(std::filesystem::path(std::u8string(jsonPath.begin(), jsonPath.end())));
	if (!file.is_open()) {
		LogManager::GetInstance().Log("Failed to open font json : " + jsonPath);
		return false;
	}

	nlohmann::json root;
	try {
		file >> root;
	} catch (const nlohmann::json::exception& e) {
		LogManager::GetInstance().Log("Failed to parse font json : " + jsonPath + " (" + e.what() + ")");
		return false;
	}

	// アトラス情報
	const auto& atlas = root.at("atlas");
	distanceRange_ = atlas.at("distanceRange").get<float>();
	atlasWidth_ = atlas.at("width").get<float>();
	atlasHeight_ = atlas.at("height").get<float>();

	// UVはテクスチャの上端を原点とするので、-yorigin topで出力されている
	const bool isYOriginTop = atlas.value("yOrigin", "bottom") == "top";
	if (!isYOriginTop) {
		LogManager::GetInstance().Log("Font atlas must be generated with -yorigin top : " + jsonPath);
		return false;
	}

	// メトリクス
	const auto& metrics = root.at("metrics");
	lineHeight_ = metrics.at("lineHeight").get<float>();
	ascender_ = metrics.at("ascender").get<float>();
	descender_ = metrics.at("descender").get<float>();

	// 文字データ
	glyphs_.clear();
	for (const auto& g : root.at("glyphs")) {
		Glyph glyph;
		glyph.advance = g.at("advance").get<float>();

		if (g.contains("planeBounds") && g.contains("atlasBounds")) {
			const auto& plane = g.at("planeBounds");
			glyph.planeBounds = {
				plane.at("left").get<float>(), plane.at("top").get<float>(),
				plane.at("right").get<float>(), plane.at("bottom").get<float>() };

			const auto& bounds = g.at("atlasBounds");
			glyph.uvBounds = {
				bounds.at("left").get<float>() / atlasWidth_, bounds.at("top").get<float>() / atlasHeight_,
				bounds.at("right").get<float>() / atlasWidth_, bounds.at("bottom").get<float>() / atlasHeight_ };

			glyph.hasQuad = true;
		}

		glyphs_[static_cast<char32_t>(g.at("unicode").get<uint32_t>())] = glyph;
	}

	// カーニング
	kerning_.clear();
	if (root.contains("kerning")) {
		for (const auto& k : root.at("kerning")) {
			char32_t left = static_cast<char32_t>(k.at("unicode1").get<uint32_t>());
			char32_t right = static_cast<char32_t>(k.at("unicode2").get<uint32_t>());
			kerning_[MakeKerningKey(left, right)] = k.at("advance").get<float>();
		}
	}

	// アトラス画像はSDFの距離値なのでリニアとして読み込む
	texture_ = std::make_unique<Texture>();
	texture_->Create(imagePath, cmdList, false, false);

	LogManager::GetInstance().Log("End LoadFont : " + jsonPath + "\n");
	return true;
}

const Font::Glyph* Font::FindGlyph(char32_t codepoint) const {
	auto it = glyphs_.find(codepoint);
	if (it == glyphs_.end()) {
		return nullptr;
	}
	return &it->second;
}

float Font::GetKerning(char32_t left, char32_t right) const {
	if (kerning_.empty()) { return 0.0f; }
	auto it = kerning_.find(MakeKerningKey(left, right));
	if (it == kerning_.end()) {
		return 0.0f;
	}
	return it->second;
}
