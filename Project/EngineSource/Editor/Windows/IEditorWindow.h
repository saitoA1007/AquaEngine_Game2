#pragma once
#include<string>

namespace GameEngine {

	class IEditorWindow {
	public:
		virtual ~IEditorWindow() = default;
		virtual void Draw() = 0;
		virtual std::string GetName() const = 0;

		bool isActive = true;
	};

	/// <summary>
	/// 独自のUndo/Redo・保存のショートカットを持つウィンドウの管理
	/// そのウィンドウがフォーカスされている間は、シーン全体のショートカットを無効にする
	/// </summary>
	namespace EditorShortcut {
		// 最後にフォーカスが報告されたフレーム
		inline int localFocusFrame = -100;

		// フォーカス中であることを報告する
		inline void ReportLocalFocus(int frameCount) { localFocusFrame = frameCount; }

		// 直前のフレームまでにフォーカスが報告されていればtrue
		inline bool IsLocalWindowFocused(int frameCount) { return localFocusFrame >= frameCount - 1; }
	}
}
