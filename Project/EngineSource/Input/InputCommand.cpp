#include "pch.h"
#include"InputCommand.h"

using namespace GameEngine;

namespace {

	// キーボード入力の判定
	bool Check(Input* input, const KeyBinding& binding) {
		switch (binding.press) {
		case PressType::kPush:    return input->PushKey(binding.key);
		case PressType::kTrigger: return input->TriggerKey(binding.key);
		case PressType::kRelease: return input->ReleaseKey(binding.key);
		}
		return false;
	}

	// マウス入力の判定
	bool Check(Input* input, const MouseBinding& binding) {
		switch (binding.press) {
		case PressType::kPush:    return input->PushMouse(binding.button);
		case PressType::kTrigger: return input->TriggerMouse(binding.button);
		case PressType::kRelease: return input->ReleaseMouse(binding.button);
		}
		return false;
	}

	// パッドのボタン入力の判定
	bool Check(Input* input, const PadButtonBinding& binding) {
		switch (binding.press) {
		case PressType::kPush:    return input->PushPad(binding.button);
		case PressType::kTrigger: return input->TriggerPad(binding.button);
		case PressType::kRelease: return input->ReleasePad(binding.button);
		}
		return false;
	}

	// パッドのスティック入力の判定
	bool Check(Input* input, const PadStickBinding& binding) {
		Vector2 stick = (binding.side == PadSide::kLeft) ? input->GetLeftStick() : input->GetRightStick();
		float dot = stick.x * binding.direction.x + stick.y * binding.direction.y;
		return dot > binding.threshold;
	}

	// パッドのトリガー入力の判定
	bool Check(Input* input, const PadTriggerBinding& binding) {
		// XInputのトリガー範囲は0-255なので、threshold(0.0-1.0)を変換する
		float triggerThreshold = binding.threshold * 255.0f;
		bool isLeft = (binding.side == PadSide::kLeft);

		switch (binding.press) {
		case PressType::kPush:
			return isLeft ? input->GetPushPadLeftTrigger(triggerThreshold) : input->GetPushPadRightTrigger(triggerThreshold);
		case PressType::kTrigger:
			return isLeft ? input->GetTriggerPadLeftTrigger(triggerThreshold) : input->GetTriggerPadRightTrigger(triggerThreshold);
		case PressType::kRelease:
			return isLeft ? input->GetReleasePadLeftTrigger(triggerThreshold) : input->GetReleasePadRightTrigger(triggerThreshold);
		}
		return false;
	}
}

InputCommand::InputCommand(Input* input) {
	input_ = input;
}

void InputCommand::Update() {
	// すべてのコマンドをチェック
	for (auto& pair : inputCommands_) {
		const std::string& commandName = pair.first;
		const std::vector<InputBinding>& bindings = pair.second;

		// 押されているかを判定するフラグ
		bool isActive = false;

		// 設定された入力のいずれかが成立していればアクティブ
		for (const InputBinding& binding : bindings) {
			if (CheckCondition(binding)) {
				isActive = true;
				break;
			}
		}
		// 現在の状態を更新
		inputCommandStates_[commandName] = isActive;
	}
}

void InputCommand::RegisterCommand(const std::string& commandName, const std::vector<InputBinding>& bindings) {
	// コマンドを登録
	inputCommands_[commandName] = bindings;
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

bool InputCommand::CheckCondition(const InputBinding& binding) const {
	// 登録された入力の種類に応じた判定関数を呼び出す
	return std::visit([this](const auto& b) { return Check(input_, b); }, binding);
}

const Vector2& InputCommand::GetMousePosition() const {
	return input_->GetMousePosition();
}

const Vector2& InputCommand::GetMouseDelta() {
	return input_->GetMouseDelta();
}

int32_t InputCommand::GetWheel() const {
	return input_->GetWheel();
}

Vector2 InputCommand::GetLeftStick() {
	return input_->GetLeftStick();
}

Vector2 InputCommand::GetRightStick() {
	return input_->GetRightStick();
}

void InputCommand::PlayPadVibration(float left, float right) {
	input_->SetVibration(left, right);
}