#pragma once
#include"IEditorWindow.h"
#include<vector>
#include<unordered_map>

namespace GameEngine {

	class ConsoleWindow : public IEditorWindow {
	public:
		void Draw() override;
		std::string GetName() const override { return "Console"; };

	private:

		// フィルターするグループを保存する
		std::unordered_map<std::string, bool> groupFilters_;
	};
}
