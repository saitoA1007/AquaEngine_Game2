#include "pch.h"
#include "EditorLayout.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <Windows.h>

using namespace GameEngine;

void EditorLayout::SaveLayout(const std::vector<std::unique_ptr<IEditorWindow>>& windows) const {
	json layoutJson = json::object();

	// すべてのウィンドウの状態をJSONオブジェクトにまとめる
	for (const auto& window : windows) {
		layoutJson[window->GetName()] = window->isActive;
	}

	// 保存先フォルダが無ければ生成する
	if (!EnsureDirectory()) {
		return;
	}

	// ファイルを書き込み用に開く
	std::ofstream file(kFilePath);

	// ファイルオープン失敗
	if (file.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "WindowLayout", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む
	file << std::setw(4) << layoutJson << std::endl;
	// ファイルを閉じる
	file.close();
}

void EditorLayout::LoadLayout(std::vector<std::unique_ptr<IEditorWindow>>& windows) {

	// ファイルがなければ現在のウィンドウ状態で生成して終了
	if (!std::filesystem::exists(kFilePath)) {
		SaveLayout(windows);
		return;
	}

	// 読み込み用ファイルストリーム
	std::ifstream ifs(kFilePath);

	// ファイルオープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for load.";
		MessageBoxA(nullptr, message.c_str(), "WindowLayout", 0);
		assert(0);
		return;
	}

	// json文字列からjsonのデータ構造に展開
	json layoutJson = json::parse(ifs, nullptr, false);
	// ファイルを閉じる
	ifs.close();

	// 空ファイル・壊れたファイルなら現在の状態で作り直す
	if (layoutJson.is_discarded() || !layoutJson.is_object()) {
		SaveLayout(windows);
		return;
	}

	// 登録されているウィンドウの状態をJSONから読み込む
	for (auto& window : windows) {
		const std::string name = window->GetName();

		if (layoutJson.contains(name) && layoutJson[name].is_boolean()) {
			window->isActive = layoutJson[name].get<bool>();
		}
	}
}

bool EditorLayout::EnsureDirectory() {
	const std::filesystem::path dir = kFilePath.parent_path();

	// 親フォルダの指定が無いならそのまま
	if (dir.empty()) {
		return true;
	}

	// フォルダが無ければ階層ごと生成する
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);

	if (ec) {
		std::string message = "Failed create directory : " + dir.string() + "\n" + ec.message();
		MessageBoxA(nullptr, message.c_str(), "WindowLayout", 0);
		assert(0);
		return false;
	}
	return true;
}