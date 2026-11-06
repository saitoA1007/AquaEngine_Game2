#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <variant>

#include"InPut.h"

namespace GameEngine {

	// ボタン系で共通の押し方
	enum class PressType {
		kPush,    // 押しっぱなし
		kTrigger, // 押した瞬間
		kRelease, // 離した瞬間
	};

	// 左右の指定
	enum class PadSide {
		kLeft,
		kRight,
	};

	// キーボード入力
	struct KeyBinding {
		BYTE key; // キー番号(DIK_)
		PressType press = PressType::kPush;
	};

	// マウス入力
	struct MouseBinding {
		int32_t button; // 0:左 1:右 2:中
		PressType press = PressType::kPush;
	};

	// パッドのボタン入力
	struct PadButtonBinding {
		WORD button; // XINPUT_GAMEPAD_
		PressType press = PressType::kPush;
	};

	// パッドのスティック入力
	struct PadStickBinding {
		PadSide side;            // 左右どちらのスティックか
		Vector2 direction;       // 判定する方向
		float threshold = 0.2f;  // しきい値
	};

	// パッドのトリガー入力（L2 / R2）
	struct PadTriggerBinding {
		PadSide side;                       // L2,R2
		PressType press = PressType::kPush;
		float threshold = 0.2f;             // しきい値(0.0~1.0)
	};

	// いずれか1つのデバイス入力
	using InputBinding = std::variant<
		KeyBinding,
		MouseBinding,
		PadButtonBinding,
		PadStickBinding,
		PadTriggerBinding>;

	class InputCommand {
	public:
		~InputCommand() = default;

		/// <summary>
		/// 入力処理を受け取る
		/// </summary>
		/// <param name="input"></param>
		InputCommand(Input* input);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

	public:

		/// <summary>
		/// コマンドを登録する
		/// </summary>
		/// <param name="commandName">コマンドの名前</param>
		/// <param name="bindings">コマンドに割り当てる入力（いずれかが成立すればアクティブ）</param>
		void RegisterCommand(const std::string& commandName, const std::vector<InputBinding>& bindings);

		/// <summary>
		/// コマンドを削除
		/// </summary>
		/// <param name="commandName">コマンド名</param>
		void UnregisterCommand(const std::string& commandName);

		/// <summary>
		/// 全てのコマンドを削除
		/// </summary>
		void ClearAllCommands();

		/// <summary>
		/// 指定したコマンドが押されているかをチェック
		/// </summary>
		/// <returns></returns>
		bool IsCommandActive(const std::string& commandName) const;

		/// <summary>
		/// マウスの位置を取得する（ウィンドウ座標系）
		/// </summary>
		/// <returns>マウスの位置</returns>
		const Vector2& GetMousePosition() const;

		/// <summary>
		/// マウスの移動量を取得（ウィンドウ座標系）
		/// </summary>
		/// <returns></returns>
		const Vector2& GetMouseDelta();

		/// <summary>
		/// ホイールスクロール量を取得する
		/// </summary>
		/// <returns>ホイールスクロール量。奥側に回したら+。Windowsの設定で逆にしてたら逆</returns>
		int32_t GetWheel() const;

		/// <summary>
		/// パッドの左スティック座標を取得
		/// </summary>
		/// <returns></returns>
		Vector2 GetLeftStick();

		/// <summary>
		/// パッドの右スティック座標を取得
		/// </summary>
		/// <returns></returns>
		Vector2 GetRightStick();

		/// <summary>
		/// 振動させる(0~1の範囲を入れる)
		/// </summary>
		/// <param name="left">左モーター : 低周波</param>
		/// <param name="right">右モーター : 高周波</param>
		void PlayPadVibration(float left, float right);

	private:
		InputCommand(const InputCommand&) = delete;
		const InputCommand& operator=(const InputCommand&) = delete;

		// 入力処理
		Input* input_ = nullptr;

		// 登録されたコマンドと条件
		std::unordered_map<std::string, std::vector<InputBinding>> inputCommands_;
		// 現在の状態
		std::unordered_map<std::string, bool> inputCommandStates_;

	private:

		/// <summary>
		/// コマンドの入力条件チェック
		/// </summary>
		/// <param name="binding"></param>
		/// <returns></returns>
		bool CheckCondition(const InputBinding& binding) const;
	};
}
