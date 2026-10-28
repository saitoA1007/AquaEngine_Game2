#include "pch.h"
#include"InputCommand.h"

using namespace GameEngine;

InputCommand::InputCommand(Input* input) {
	input_ = input;
}

void InputCommand::Update() {
	// すべてのコマンドをチェック
	for (auto& pair : inputCommands_) {
		const std::string& commandName = pair.first;
		const std::vector<InputCondition>& conditions = pair.second;

		// 押されているかを判定するフラグ
		bool inputConditions = false;

		// 設定された入力条件を全てチェック
		for (size_t i = 0; i < conditions.size(); ++i) {
			if (CheckCondition(conditions[i])) {
				// 押されていない場合、falseに設定
				inputConditions = true;
				break;
			}
		}
		// 現在の状態を更新
		inputCommandStates_[commandName] = inputConditions;
	}
}

void InputCommand::RegisterCommand(const std::string& commandName, const std::vector<InputCondition>& conditions) {
	// コマンドを登録
	inputCommands_[commandName] = conditions;
	// コマンドの入力状態を登録
	inputCommandStates_[commandName] = false;
}

void InputCommand::UnregisterCommand(const std::string& commandName) {
	// 指定したコマンドが存在している時、削除する
	auto command = inputCommands_.find(commandName);

	if (command != inputCommands_.end()) {
		inputCommands_.erase(commandName);
		inputCommandStates_.erase(commandName);
	}	
}

bool InputCommand::IsCommandActive(const std::string& commandName) const {
	// 指定したコマンドが存在していれば、押されている状態を返す
	auto command = inputCommandStates_.find(commandName);

	if (command != inputCommandStates_.end()) {
		return command->second;
	} else {
		return false;
	}
}

void InputCommand::ClearAllCommands() {
	inputCommands_.clear();
	inputCommandStates_.clear();
}

bool InputCommand::CheckCondition(const InputCondition& condition) {
	
	switch (condition.inputState)
	{
	case GameEngine::InputState::KeyPush:
		return input_->PushKey(static_cast<BYTE>(condition.code));
		break;

	case GameEngine::InputState::KeyTrigger:
		return input_->TriggerKey(static_cast<BYTE>(condition.code));
		break;

	case GameEngine::InputState::KeyRelease:
		return input_->ReleaseKey(static_cast<BYTE>(condition.code));
		break;

	case GameEngine::InputState::MousePush:
		return input_->PushMouse(condition.code);
		break;

	case GameEngine::InputState::MouseTrigger:
		return input_->TriggerMouse(condition.code);
		break;

	case GameEngine::InputState::MouseRelease:
		return input_->ReleaseMouse(condition.code);
		break;

	case GameEngine::InputState::PadPush:
		return input_->PushPad(static_cast<WORD>(condition.code));
		break;

	case GameEngine::InputState::PadTrigger:
		return input_->TriggerPad(static_cast<WORD>(condition.code));
		break;

	case GameEngine::InputState::PadRelease:
		return input_->ReleasePad(static_cast<WORD>(condition.code));
		break;

	case GameEngine::InputState::PadLeftStick: {
		Vector2 stick = input_->GetLeftStick();
		float dot = stick.x * condition.direction.x + stick.y * condition.direction.y;
		return dot > condition.threshold;
		break;
	}

	case GameEngine::InputState::PadRightStick: {
		Vector2 stick = input_->GetRightStick();
		float dot = stick.x * condition.direction.x + stick.y * condition.direction.y;
		return dot > condition.threshold;
		break;
	}

	case GameEngine::InputState::PadPushLeftTrigger: {
		// XInputのトリガー範囲は0-255なので、threshold(0.0-1.0)を変換する
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetPushPadLeftTrigger(triggerThreshold);
		break;
	}

	case GameEngine::InputState::PadTriggerLeftTrigger: {
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetTriggerPadLeftTrigger(triggerThreshold);
		break;
	}

	case GameEngine::InputState::PadReleaseLeftTrigger: {
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetReleasePadLeftTrigger(triggerThreshold);
		break;
	}

	case GameEngine::InputState::PadPushRightTrigger: {
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetPushPadRightTrigger(triggerThreshold);
		break;
	}

	case GameEngine::InputState::PadTriggerRightTrigger: {
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetTriggerPadRightTrigger(triggerThreshold);
		break;
	}

	case GameEngine::InputState::PadReleaseRightTrigger: {
		float triggerThreshold = condition.threshold * 255.0f;
		return input_->GetReleasePadRightTrigger(triggerThreshold);
		break;
	}

	default:
		return false;
		break;
	}
}