#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"

namespace GameEngine {

    // 前方宣言
    class GameParamEditor;

    class HierarchyWindow : public IEditorWindow {
    public:
        HierarchyWindow(GameParamEditor* gameParamEditor);

        void Draw() override;
        std::string GetName() const override { return "ParameterHierarchy"; }

    private:
        GameParamEditor* gameParamEditor_ = nullptr;
    };
}
