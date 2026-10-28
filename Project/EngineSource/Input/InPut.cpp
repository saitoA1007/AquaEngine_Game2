#include "pch.h"
#define NOMINMAX
#include "InPut.h"
#include <cassert>
#include <algorithm>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib, "xinput.lib")

#include "MyMath.h"

using namespace GameEngine;

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {

	hwnd_ = hwnd;

	// DirectInputの初期化
	directInput_ = nullptr;
	HRESULT result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput_, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	keyboard_ = nullptr;
	result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(result));
	// 入力データ形式のセット
	result = keyboard_->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(result));
	// 排他制御レベルのセット
	result = keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	// マウスデバイスの生成
	mouseDevice_ = nullptr;
	result = directInput_->CreateDevice(GUID_SysMouse, &mouseDevice_, NULL);
	assert(SUCCEEDED(result));
	// 入力データ形式のセット
	result = mouseDevice_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(result));
	// マウスの排他制御レベルのセット
	result = mouseDevice_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(result));

	// コントローラーデバイスの初期化
	preControllerState_ = controllerState_;
	ZeroMemory(&controllerState_, sizeof(XINPUT_STATE));
	XInputGetState(0, &controllerState_);
}

void Input::Update() {

	// キーボード情報の取得開始
	keyboard_->Acquire();
	// 全キーの前フレームのの入力状態をコピー
	preKeys_ = keys_;
	// 全キーの入力状態を取得する
	keyboard_->GetDeviceState(sizeof(keys_), keys_.data());

	// マウス情報を取得
	mouseDevice_->Acquire();
	// 前フレームのマウスの状態をコピー
	preMouse_ = mouse_;
	mouseDevice_->GetDeviceState(sizeof(mouse_), &mouse_);

	// Windowのカーソル位置を取得(スクリーン座標)
	GetCursorPos(&point_);
	// 自分のwindow基準の座標に変換
	ScreenToClient(hwnd_, &point_);
	// 前フレームの位置を取得する
	preMousePosition_ = mousePosition_;
	// Vector2に格納
	mousePosition_.x = static_cast<float>(point_.x);
	mousePosition_.y = static_cast<float>(point_.y);

	// コントローラーステートの更新
	preControllerState_ = controllerState_;
	ZeroMemory(&controllerState_, sizeof(XINPUT_STATE));
	XInputGetState(0, &controllerState_);
}

bool Input::PushKey(BYTE keyNumber) const {
	if (keys_[keyNumber] == 0x80) {
		return true;
	} else {
		return false;
	}
}

bool Input::TriggerKey(BYTE keyNumber) const {
	if (keys_[keyNumber] == 0x80 && preKeys_[keyNumber] == 0x00) {
		return true;
	} else {
		return false;
	}
}

bool Input::ReleaseKey(BYTE keyNumber) const {
	// 前フレーム押されていた かつ 今フレーム離された
	if (keys_[keyNumber] == 0x00 && preKeys_[keyNumber] == 0x80) {
		return true;
	} else {
		return false;
	}
}

bool Input::PushMouse(int32_t mouseNumber) const {
	if (mouse_.rgbButtons[mouseNumber] == 0x80) {
		return true;
	} else {
		return false;
	}
}

bool Input::TriggerMouse(int32_t buttonNumber) const {
	if (mouse_.rgbButtons[buttonNumber] == 0x80 && preMouse_.rgbButtons[buttonNumber] == 0x00) {
		return true;
	} else {
		return false;
	}
}

bool Input::ReleaseMouse(int32_t buttonNumber) const {
	if (mouse_.rgbButtons[buttonNumber] == 0x00 && preMouse_.rgbButtons[buttonNumber] == 0x80) {
		return true;
	} else {
		return false;
	}
}

const Vector2& Input::GetMousePosition() const {
	return mousePosition_;
}

const Vector2& Input::GetMouseDelta() {
	// マウス位置の差分を取得
	mouseDelta_.x = mousePosition_.x - preMousePosition_.x;
	mouseDelta_.y = mousePosition_.y - preMousePosition_.y;
	// 正規化
	mouseDelta_.Normalize();
	return mouseDelta_;
}

int32_t Input::GetWheel() const {
	return static_cast<int32_t>(mouse_.lZ);
}

bool Input::PushPad(WORD button) const {
	if ((controllerState_.Gamepad.wButtons & button) != 0) {
		return true;
	} else {
		return false;
	}
}

bool Input::TriggerPad(WORD button) const {
	if ((controllerState_.Gamepad.wButtons & button) != 0 &&
		(preControllerState_.Gamepad.wButtons & button) == 0) {
		return true;
	} else {
		return false;
	}
}

bool Input::ReleasePad(WORD button) const {
	if ((controllerState_.Gamepad.wButtons & button) == 0 &&
		(preControllerState_.Gamepad.wButtons & button) != 0) {
		return true;
	} else {
		return false;
	}
}

Vector2 Input::GetLeftStick() {
	Vector2 result{};

	// デッドゾーン除去
	const float deadZone = static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	float x = static_cast<float>(controllerState_.Gamepad.sThumbLX);
	float y = static_cast<float>(controllerState_.Gamepad.sThumbLY);

	float magnitude = sqrtf(x * x + y * y);

	if (magnitude > deadZone) {
		// デッドゾーン補正と正規化（-1.0～1.0）
		magnitude = std::min(magnitude, static_cast<float>(SHRT_MAX));
		magnitude = (magnitude - deadZone) / (static_cast<float>(SHRT_MAX) - deadZone);

		result.x = x / static_cast<float>(SHRT_MAX) * magnitude;
		result.y = y / static_cast<float>(SHRT_MAX) * magnitude;
	}

	return result;
}

Vector2 Input::GetRightStick() {
	Vector2 result{};

	// デッドゾーン除去
	const float deadZone = static_cast<float>(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	float x = static_cast<float>(controllerState_.Gamepad.sThumbRX);
	float y = static_cast<float>(controllerState_.Gamepad.sThumbRY);

	float magnitude = sqrtf(x * x + y * y);

	if (magnitude > deadZone) {
		// デッドゾーン補正と正規化（-1.0～1.0）
		magnitude = std::min(magnitude, static_cast<float>(SHRT_MAX));
		magnitude = (magnitude - deadZone) / (static_cast<float>(SHRT_MAX) - deadZone);

		result.x = x / static_cast<float>(SHRT_MAX) * magnitude;
		result.y = y / static_cast<float>(SHRT_MAX) * magnitude;
	}

	return result;
}

bool Input::GetPushPadLeftTrigger(const float& value) {

	float lt = static_cast<float>(controllerState_.Gamepad.bLeftTrigger);

	if (lt > value) {
		return true;
	}
	return false;
}

bool Input::GetTriggerPadLeftTrigger(const float& value) {
	float lt = static_cast<float>(controllerState_.Gamepad.bLeftTrigger);
	float preLt = static_cast<float>(preControllerState_.Gamepad.bLeftTrigger);
	// 今フレーム閾値を超えている かつ 前フレーム閾値以下
	if (lt > value && preLt <= value) {
		return true;
	}
	return false;
}

bool Input::GetReleasePadLeftTrigger(const float& value) {
	float lt = static_cast<float>(controllerState_.Gamepad.bLeftTrigger);
	float preLt = static_cast<float>(preControllerState_.Gamepad.bLeftTrigger);
	// 前フレーム閾値を超えていた かつ 今フレーム閾値以下
	if (lt <= value && preLt > value) {
		return true;
	}
	return false;
}

bool Input::GetPushPadRightTrigger(const float& value) {

	float rt = static_cast<float>(controllerState_.Gamepad.bRightTrigger);

	if (rt > value) {
		return true;
	}
	return false;
}

bool Input::GetTriggerPadRightTrigger(const float& value) {
	float rt = static_cast<float>(controllerState_.Gamepad.bRightTrigger);
	float preRt = static_cast<float>(preControllerState_.Gamepad.bRightTrigger);
	// 今フレーム閾値を超えている かつ 前フレーム閾値以下
	if (rt > value && preRt <= value) {
		return true;
	}
	return false;
}

bool Input::GetReleasePadRightTrigger(const float& value) {
	float rt = static_cast<float>(controllerState_.Gamepad.bRightTrigger);
	float preRt = static_cast<float>(preControllerState_.Gamepad.bRightTrigger);
	if (rt <= value && preRt > value) {
		return true;
	}
	return false;
}

void Input::SetVibration(float leftMotorSpeed, float rightMotorSpeed) {
	leftMotorSpeed = std::clamp(leftMotorSpeed, 0.0f, 1.0f);
	rightMotorSpeed = std::clamp(rightMotorSpeed, 0.0f, 1.0f);

	WORD leftSpeed = static_cast<WORD>(leftMotorSpeed * 65535.0f);
	WORD rightSpeed = static_cast<WORD>(rightMotorSpeed * 65535.0f);

	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = leftSpeed;   // 左
	vibration.wRightMotorSpeed = rightSpeed; // 右
	XInputSetState(0, &vibration);
}

bool Input::IsPadConnected() const {
	return (controllerState_.dwPacketNumber != 0);
}

