#pragma once
#include "EaseCurve.h"
#include "ImGuiManager.h"

namespace GameEngine {
	namespace CurveEditor {

		/// <summary>
		/// イージングカーブをImGuiで編集する
		/// 左ドラッグでキー/接線の移動
		/// ダブルクリックでキー追加
		/// 右クリックでキーのメニュー
		/// </summary>
		/// <param name="label">ImGuiのID</param>
		/// <param name="curve">編集するカーブ</param>
		/// <param name="height">グラフの高さ</param>
		/// <returns>値が変更されたか</returns>
		bool Draw(const char* label, EaseCurve& curve, float height = 180.0f);
	}
}