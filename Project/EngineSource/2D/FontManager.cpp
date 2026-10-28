#include "pch.h"
#include "FontManager.h"
#include <filesystem>
#include "LogManager.h"
using namespace GameEngine;

namespace {
	// UTF-8の文字列として取得する
	std::string ToUtf8(const std::filesystem::path& path) {
		std::u8string str = path.u8string();
		return std::string(str.begin(), str.end());
	}
}

void FontManager::Initialize(ID3D12GraphicsCommandList* commandList) {
	commandList_ = commandList;
}

void FontManager::Finalize() {
	fonts_.clear();
}

void FontManager::LoadAllFont() {
	namespace fs = std::filesystem;

	if (!fs::exists(kDirectoryPath)) {
		LogManager::GetInstance().Log("Font directory not found, skipping LoadAllFont: " + kDirectoryPath);
		return;
	}

	LogManager::GetInstance().Log("Start Loading All Fonts from: " + kDirectoryPath);

	// JSONと同名のPNGをペアとして読み込む
	for (const auto& entry : fs::directory_iterator(kDirectoryPath)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".json") { continue; }

		fs::path imagePath = entry.path();
		imagePath.replace_extension(".png");
		if (!fs::exists(imagePath)) {
			LogManager::GetInstance().Log("Font image not found: " + ToUtf8(imagePath));
			continue;
		}

		RegisterFont(ToUtf8(entry.path().stem()), ToUtf8(entry.path()), ToUtf8(imagePath));
	}

	LogManager::GetInstance().Log("End Loading All Fonts");
}

void FontManager::RegisterFont(const std::string& name, const std::string& jsonPath, const std::string& imagePath) {
	// 登録している場合は早期リターン
	if (fonts_.contains(name)) {
		return;
	}

	auto font = std::make_unique<Font>();
	if (!font->Load(jsonPath, imagePath, commandList_)) {
		return;
	}
	fonts_[name] = std::move(font);
}

const Font* FontManager::GetFont(const std::string& name) const {
	auto it = fonts_.find(name);
	if (it == fonts_.end()) {
		LogManager::GetInstance().Log("Font not found: " + name);
		return nullptr;
	}
	return it->second.get();
}

std::vector<std::string> FontManager::GetRegisteredFontNames() const {
	std::vector<std::string> names;
	names.reserve(fonts_.size());
	for (const auto& [name, font] : fonts_) {
		names.push_back(name);
	}
	return names;
}
