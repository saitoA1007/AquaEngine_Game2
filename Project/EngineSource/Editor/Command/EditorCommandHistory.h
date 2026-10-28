#pragma once
#include <deque>
#include <memory>
#include <utility>
#include "IEditorCommand.h"

namespace GameEngine {

	class EditorCommandHistory {
	public:
		// コマンドを実行して履歴に積む
		void Execute(std::unique_ptr<IEditorCommand> command) {
			command->Execute();
			undoStack_.push_back(std::move(command));
			redoStack_.clear();

			// 履歴が無制限に伸びないよう上限を設ける
			while (undoStack_.size() > kMaxHistory) {
				undoStack_.pop_front();
			}
		}

		void Undo() {
			if (undoStack_.empty()) {
				return;
			}
			std::unique_ptr<IEditorCommand> command = std::move(undoStack_.back());
			undoStack_.pop_back();
			command->Undo();
			redoStack_.push_back(std::move(command));
		}

		void Redo() {
			if (redoStack_.empty()) {
				return;
			}
			std::unique_ptr<IEditorCommand> command = std::move(redoStack_.back());
			redoStack_.pop_back();
			command->Execute();
			undoStack_.push_back(std::move(command));
		}

		bool CanUndo() const { return !undoStack_.empty(); }
		bool CanRedo() const { return !redoStack_.empty(); }

		// シーンを開き直す時などに履歴をリセットする
		void Clear() {
			undoStack_.clear();
			redoStack_.clear();
		}

	private:
		static constexpr size_t kMaxHistory = 50;

		std::deque<std::unique_ptr<IEditorCommand>> undoStack_;
		std::deque<std::unique_ptr<IEditorCommand>> redoStack_;
	};
}

