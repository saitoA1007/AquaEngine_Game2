#pragma once
#include "ImGuiManager.h"
#include "NodePinIcon.h"
namespace ned = ax::NodeEditor;

namespace GameEngine::NodeUI {

    // ノード共通の見た目設定
    struct NodeStyle {
        float width = 240.0f;                 // ノード内容の幅
        float iconSize = 24.0f;               // ピンアイコンの大きさ
        ImTextureID headerTexture{};          // ヘッダー画像
        ImVec2 headerTexSize{ 64.0f, 64.0f }; // 画像の実サイズ
    };

    // ノードエディタの共有レイアウト
    void ApplyEditorStyle();

    // 画像を描画
    void DrawImagePreview(ImTextureID tex, ImVec2 size, bool dim = false);

    class NodeBuilder {
    public:
        explicit NodeBuilder(const NodeStyle& style = {});

        void Begin(ned::NodeId id);

        // ヘッダー。タイトルだけなら Header()、中にウィジェットを足すなら Begin/End を使う
        void Header(const std::string& title, ImU32 tint);
        void BeginHeader(const std::string& title, ImU32 tint);
        void EndHeader();

        void InputPin(ned::PinId id, const std::string& name, PinIconType icon, ImU32 color, bool connected);
        void OutputPin(ned::PinId id, const std::string& name, PinIconType icon, ImU32 color, bool connected);
        void PinPair(ned::PinId inId, const std::string& inName, PinIconType inIcon, ImU32 inColor, bool inConnected,
            ned::PinId outId, const std::string& outName, PinIconType outIcon, ImU32 outColor, bool outConnected);

        // パラメータ領域の開始
        void BeginContent();

        //中身が何も描かれなかったら区切り線を消す
        void EndContent();

        // 画像プレビュー。tex が空ならプレースホルダを描く。dim で暗くする
        void Preview(ImTextureID tex, bool dim = false, float aspect = 16.0f / 9.0f);

        // ノード描画終了
        void End();

    private:
        NodeStyle style_;
        ned::NodeId nodeId_;
        float originX_ = 0.0f;     // ノード内容の左端
        float headerBottom_ = 0.0f;
        float contentTop_ = -1.0f;
        float contentStartY_ = 0.0f;
        ImU32 headerTint_ = IM_COL32_WHITE;
    };
}