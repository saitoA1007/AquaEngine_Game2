#pragma once

namespace GameEngine {

	// 全てのエディタ操作コマンドの基底クラス
	class IEditorCommand {
	public:
		virtual ~IEditorCommand() = default;

		// コマンドを実行する
		virtual void Execute() = 0;

		// コマンドの効果を取り消す
		virtual void Undo() = 0;
	};

}
