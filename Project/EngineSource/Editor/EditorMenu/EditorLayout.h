#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

#include "Windows/IEditorWindow.h"

#include <json.hpp>
using json = nlohmann::json;

namespace GameEngine {

	class EditorLayout {
	public:

		/// <summary>
		/// ウィンドウレイアウトをjsonファイルに保存する
		/// </summary>
		/// <param name="windows"></param>
		void SaveLayout(const std::vector<std::unique_ptr<IEditorWindow>>& windows) const;

		/// <summary>
		/// jsonファイルからウィンドウレイアウトを取得する
		/// </summary>
		/// <param name="windows"></param>
		void LoadLayout(std::vector<std::unique_ptr<IEditorWindow>>& windows);

	private:

		// ウィンドウレイアウトが保存してあるファイルパス名
		inline static const std::filesystem::path kFilePath = "EngineSource/Resources/Json/windowLayout.json";

	private:

		/// <summary>
		/// 保存先のフォルダが無ければ生成する
		/// </summary>
		/// <returns></returns>
		static bool EnsureDirectory();
	};
}
