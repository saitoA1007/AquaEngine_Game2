#pragma once
#ifdef USE_IMGUI
#include "IEngineSubsystem.h"

namespace GameEngine {

    // エディター機能の前方宣言
    class EditorWindowManager;
    class EditorMenuBar;
    class SceneMenuBar;
    class EditorLayout;
    class EditorToolBar;
    class ViewOptionsBar;
    class AddObjectBar;
    class GameParamEditor;

    /// <summary>
    /// エディタシステム
    /// </summary>
    class EditorSubsystem : public IEngineSubsystem {
    public:
        EditorSubsystem();
        ~EditorSubsystem();

        void Initialize() override;
        void Update()     override;
        void Finalize()   override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        bool IsActiveUpdate() const;
        bool IsPause() const;

        void SceneReset();

    private:
        // 各ウィンドウ
        std::unique_ptr<EditorWindowManager> windowManager_;

        // メニューバー
        std::unique_ptr<EditorMenuBar> menuBar_;

        // シーンの管理機能
        std::unique_ptr<SceneMenuBar> sceneMenuBar_;

        // エディターの表示管理
        std::unique_ptr<EditorLayout> editorLayout_;

        // シーンの操作などをおこなう
        std::unique_ptr<EditorToolBar> editorToolBar_;

        // ビュー
        std::unique_ptr<ViewOptionsBar> viewOptionsBar_;

        // オブジェクトの配置をおこなう
        std::unique_ptr<AddObjectBar> addObjectBar_;

    private:

        /// <summary>
        /// Dockをするためのスペースを作成する
        /// </summary>
        void BeginDockSpace();
    };
} 
#endif