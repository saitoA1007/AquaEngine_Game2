#pragma once
#include<vector>
#include<memory>

#include"Windows/IEditorWindow.h"

namespace GameEngine {

	class EditorWindowManager {
	public:

		/// <summary>
		/// ウィンドウを登録する
		/// </summary>
		/// <param name="window"></param>
		void RegisterWindow(std::unique_ptr<IEditorWindow> window);

		/// <summary>
		/// 登録したウィンドウをすべて描画する
		/// </summary>
		void DrawAllWindows();

		/// <summary>
		/// 登録しているウィンドウデータを取得する
		/// </summary>
		/// <returns></returns>
		std::vector<std::unique_ptr<IEditorWindow>>& GetWindows() { return windows_; }
		const std::vector<std::unique_ptr<IEditorWindow>>& GetWindows() const { return windows_; }

	private:
		std::vector<std::unique_ptr<IEditorWindow>> windows_;
	};
}
