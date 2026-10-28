#include "pch.h"
#include "CurveEditor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace GameEngine::CurveEditor {

	namespace {

		constexpr float kMargin = 10.0f;       // グラフの余白
		constexpr float kKeyRadius = 5.0f;     // キーの表示半径
		constexpr float kHandleRadius = 4.0f;  // 接線ハンドルの表示半径
		constexpr float kHandleLength = 40.0f; // 接線ハンドルの長さ
		constexpr float kHitRadius = 8.0f;     // クリック判定の半径
		constexpr float kMinKeySpacing = 0.001f; // 隣のキーとの最小間隔
		constexpr int kSampleCount = 128;      // 曲線の分割数

		// ドラッグ中の対象
		enum class DragTarget : int {
			kNone,
			kKey,
			kInHandle,
			kOutHandle,
		};

		// グラフの表示範囲
		struct View {
			ImVec2 min;
			ImVec2 max;
			float vMin = 0.0f;
			float vMax = 1.0f;

			ImVec2 ToScreen(float t, float v) const {
				return ImVec2(min.x + t * (max.x - min.x), max.y - (v - vMin) / (vMax - vMin) * (max.y - min.y));
			}
			float ToTime(float x) const { return (x - min.x) / (max.x - min.x); }
			float ToValue(float y) const { return vMin + (max.y - y) / (max.y - min.y) * (vMax - vMin); }
		};

		float Distance(const ImVec2& a, const ImVec2& b) {
			float dx = a.x - b.x;
			float dy = a.y - b.y;
			return std::sqrt(dx * dx + dy * dy);
		}

		// 傾きから出り側ハンドルの画面上のオフセットを求める
		ImVec2 HandleOffset(const View& view, float tangent) {
			// 曲線空間の(1, tangent)を画面空間に変換
			ImVec2 dir(view.max.x - view.min.x, -tangent * (view.max.y - view.min.y) / (view.vMax - view.vMin));
			float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (len < 1e-6f) { return ImVec2(kHandleLength, 0.0f); }
			return ImVec2(dir.x / len * kHandleLength, dir.y / len * kHandleLength);
		}

		// 画面上のオフセットから傾きを求める
		float TangentFromOffset(const View& view, const ImVec2& offset) {
			float dt = offset.x / (view.max.x - view.min.x);
			float dv = -offset.y / (view.max.y - view.min.y) * (view.vMax - view.vMin);
			return dv / dt;
		}

		ImVec2 InHandlePos(const View& view, const EaseKey& key) {
			ImVec2 p = view.ToScreen(key.time, key.value);
			ImVec2 o = HandleOffset(view, key.inTangent);
			return ImVec2(p.x - o.x, p.y - o.y);
		}

		ImVec2 OutHandlePos(const View& view, const EaseKey& key) {
			ImVec2 p = view.ToScreen(key.time, key.value);
			ImVec2 o = HandleOffset(view, key.outTangent);
			return ImVec2(p.x + o.x, p.y + o.y);
		}

		// マウス位置にあるキーを探す
		int HitKey(const View& view, const std::vector<EaseKey>& keys, const ImVec2& mouse) {
			for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
				if (Distance(view.ToScreen(keys[i].time, keys[i].value), mouse) <= kHitRadius) {
					return i;
				}
			}
			return -1;
		}

		// 隣のキーを越えないように時間を制限する
		float ClampKeyTime(const std::vector<EaseKey>& keys, int index, float t) {
			const int last = static_cast<int>(keys.size()) - 1;
			if (index == 0 || index == last) { return keys[index].time; }
			return std::clamp(t, keys[index - 1].time + kMinKeySpacing, keys[index + 1].time - kMinKeySpacing);
		}

		// プリセットを適用
		void ApplyPreset(EaseCurve& curve, float startTangent, float endTangent, TangentMode mode) {
			curve.SetKeys({
				{ 0.0f, 0.0f, startTangent, startTangent, mode },
				{ 1.0f, 1.0f, endTangent, endTangent, mode },
				});
		}
	}

	bool Draw(const char* label, EaseCurve& curve, float height) {
		bool changed = false;

		ImGui::PushID(label);

		// フレームをまたいで保持する状態
		ImGuiStorage* storage = ImGui::GetStateStorage();
		const ImGuiID selectedId = ImGui::GetID("Selected");
		const ImGuiID dragId = ImGui::GetID("Drag");
		const ImGuiID vMinId = ImGui::GetID("VMin");
		const ImGuiID vMaxId = ImGui::GetID("VMax");

		int selected = storage->GetInt(selectedId, -1);
		DragTarget drag = static_cast<DragTarget>(storage->GetInt(dragId, static_cast<int>(DragTarget::kNone)));

		const std::vector<EaseKey>& keys = curve.GetKeys();
		if (selected >= static_cast<int>(keys.size())) { selected = -1; }

		// プリセット
		if (ImGui::SmallButton("Linear")) { ApplyPreset(curve, 1.0f, 1.0f, TangentMode::kLinear); selected = -1; changed = true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("EaseIn")) { ApplyPreset(curve, 0.0f, 2.0f, TangentMode::kFree); selected = -1; changed = true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("EaseOut")) { ApplyPreset(curve, 2.0f, 0.0f, TangentMode::kFree); selected = -1; changed = true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("EaseInOut")) { ApplyPreset(curve, 0.0f, 0.0f, TangentMode::kFree); selected = -1; changed = true; }

		// キャンバス
		const float width = (std::max)(ImGui::GetContentRegionAvail().x, 100.0f);
		const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
		const ImVec2 canvasMax(canvasMin.x + width, canvasMin.y + height);
		ImGui::InvisibleButton("Canvas", ImVec2(width, height), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool isHovered = ImGui::IsItemHovered();

		View view;
		view.min = ImVec2(canvasMin.x + kMargin, canvasMin.y + kMargin);
		view.max = ImVec2(canvasMax.x - kMargin, canvasMax.y - kMargin);

		// 縦の表示範囲はドラッグ中は固定する
		if (drag == DragTarget::kNone) {
			float lo = 0.0f;
			float hi = 1.0f;
			for (int i = 0; i <= kSampleCount; ++i) {
				float v = curve.Evaluate(static_cast<float>(i) / kSampleCount);
				lo = (std::min)(lo, v);
				hi = (std::max)(hi, v);
			}
			for (const EaseKey& key : keys) {
				lo = (std::min)(lo, key.value);
				hi = (std::max)(hi, key.value);
			}
			float pad = (hi - lo) * 0.1f;
			view.vMin = lo - pad;
			view.vMax = hi + pad;
			storage->SetFloat(vMinId, view.vMin);
			storage->SetFloat(vMaxId, view.vMax);
		} else {
			view.vMin = storage->GetFloat(vMinId, -0.1f);
			view.vMax = storage->GetFloat(vMaxId, 1.1f);
		}

		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const int last = static_cast<int>(keys.size()) - 1;

		// 左クリック: 選択中キーのハンドル → キーの順で判定
		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			drag = DragTarget::kNone;
			if (selected >= 0) {
				if (selected > 0 && Distance(InHandlePos(view, keys[selected]), mouse) <= kHitRadius) {
					drag = DragTarget::kInHandle;
				} else if (selected < last && Distance(OutHandlePos(view, keys[selected]), mouse) <= kHitRadius) {
					drag = DragTarget::kOutHandle;
				}
			}
			if (drag == DragTarget::kNone) {
				int hit = HitKey(view, keys, mouse);
				selected = hit;
				if (hit >= 0) { drag = DragTarget::kKey; }
			}
		}

		// ダブルクリックは何もない場所にキーを追加
		if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && drag == DragTarget::kNone) {
			EaseKey key{};
			key.time = std::clamp(view.ToTime(mouse.x), kMinKeySpacing, 1.0f - kMinKeySpacing);
			key.value = view.ToValue(mouse.y);
			key.mode = TangentMode::kAuto;
			selected = static_cast<int>(curve.AddKey(key));
			changed = true;
		}

		// ドラッグ
		if (drag != DragTarget::kNone && selected >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			EaseKey key = keys[selected];

			if (drag == DragTarget::kKey) {
				key.time = ClampKeyTime(keys, selected, view.ToTime(mouse.x));
				key.value = view.ToValue(mouse.y);
			} else {
				ImVec2 p = view.ToScreen(key.time, key.value);
				ImVec2 offset(mouse.x - p.x, mouse.y - p.y);
				// ハンドルがキーの反対側へ回り込まないようにする
				if (drag == DragTarget::kInHandle) {
					offset.x = (std::min)(offset.x, -1.0f);
				} else {
					offset.x = (std::max)(offset.x, 1.0f);
				}
				float tangent = TangentFromOffset(view, offset);

				// 手動で触ったら自動計算を外す
				if (key.mode == TangentMode::kAuto || key.mode == TangentMode::kLinear) {
					key.mode = TangentMode::kFree;
				}
				if (drag == DragTarget::kInHandle || key.mode == TangentMode::kFree) { key.inTangent = tangent; }
				if (drag == DragTarget::kOutHandle || key.mode == TangentMode::kFree) { key.outTangent = tangent; }
			}

			curve.SetKey(selected, key);
			changed = true;
		}
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			drag = DragTarget::kNone;
		}

		// 右クリックでキーのメニュー
		if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			int hit = HitKey(view, keys, mouse);
			if (hit >= 0) {
				selected = hit;
				ImGui::OpenPopup("KeyMenu");
			}
		}
		if (ImGui::BeginPopup("KeyMenu")) {
			if (selected >= 0 && selected < static_cast<int>(keys.size())) {
				EaseKey key = keys[selected];
				for (int i = 0; i < static_cast<int>(TangentMode::kMaxCount); ++i) {
					if (ImGui::MenuItem(TangentModeNames[i], nullptr, key.mode == static_cast<TangentMode>(i))) {
						key.mode = static_cast<TangentMode>(i);
						curve.SetKey(selected, key);
						changed = true;
					}
				}
				ImGui::Separator();
				// 端のキーは削除できない
				bool canDelete = selected != 0 && selected != static_cast<int>(keys.size()) - 1;
				if (ImGui::MenuItem("Delete", nullptr, false, canDelete)) {
					curve.RemoveKey(static_cast<size_t>(selected));
					selected = -1;
					changed = true;
				}
			}
			ImGui::EndPopup();
		}

		// 描画
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 35, 255), 4.0f);
		drawList->AddRect(canvasMin, canvasMax, IM_COL32(80, 80, 90, 255), 4.0f);
		drawList->PushClipRect(canvasMin, canvasMax, true);

		// グリッド
		const ImU32 gridColor = IM_COL32(60, 60, 70, 255);
		for (int i = 0; i <= 4; ++i) {
			float t = static_cast<float>(i) / 4.0f;
			drawList->AddLine(view.ToScreen(t, view.vMin), view.ToScreen(t, view.vMax), gridColor);
		}
		const ImU32 baseLineColor = IM_COL32(100, 100, 115, 255);
		drawList->AddLine(ImVec2(canvasMin.x, view.ToScreen(0.0f, 0.0f).y), ImVec2(canvasMax.x, view.ToScreen(0.0f, 0.0f).y), baseLineColor);
		drawList->AddLine(ImVec2(canvasMin.x, view.ToScreen(0.0f, 1.0f).y), ImVec2(canvasMax.x, view.ToScreen(0.0f, 1.0f).y), baseLineColor);
		drawList->AddText(ImVec2(canvasMin.x + 2.0f, view.ToScreen(0.0f, 1.0f).y - ImGui::GetFontSize()), baseLineColor, "1");
		drawList->AddText(ImVec2(canvasMin.x + 2.0f, view.ToScreen(0.0f, 0.0f).y), baseLineColor, "0");

		// 曲線
		ImVec2 points[kSampleCount + 1];
		for (int i = 0; i <= kSampleCount; ++i) {
			float t = static_cast<float>(i) / kSampleCount;
			points[i] = view.ToScreen(t, curve.Evaluate(t));
		}
		drawList->AddPolyline(points, kSampleCount + 1, IM_COL32(90, 200, 255, 255), ImDrawFlags_None, 2.0f);

		// 選択中キーの接線ハンドル
		if (selected >= 0) {
			const EaseKey& key = keys[selected];
			ImVec2 p = view.ToScreen(key.time, key.value);
			const ImU32 handleColor = IM_COL32(255, 170, 60, 255);
			if (selected > 0) {
				ImVec2 h = InHandlePos(view, key);
				drawList->AddLine(p, h, handleColor);
				drawList->AddCircleFilled(h, kHandleRadius, handleColor);
			}
			if (selected < static_cast<int>(keys.size()) - 1) {
				ImVec2 h = OutHandlePos(view, key);
				drawList->AddLine(p, h, handleColor);
				drawList->AddCircleFilled(h, kHandleRadius, handleColor);
			}
		}

		// キー
		for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
			ImVec2 p = view.ToScreen(keys[i].time, keys[i].value);
			ImU32 color = (i == selected) ? IM_COL32(255, 230, 80, 255) : IM_COL32(230, 230, 230, 255);
			drawList->AddCircleFilled(p, kKeyRadius, color);
		}

		drawList->PopClipRect();

		// 選択中キーの数値編集
		if (selected >= 0) {
			EaseKey key = keys[selected];
			bool edited = false;
			const bool isEndKey = selected == 0 || selected == static_cast<int>(keys.size()) - 1;

			ImGui::BeginDisabled(isEndKey);
			if (ImGui::DragFloat("Time", &key.time, 0.005f, 0.0f, 1.0f)) {
				key.time = ClampKeyTime(keys, selected, key.time);
				edited = true;
			}
			ImGui::EndDisabled();

			if (ImGui::DragFloat("Value", &key.value, 0.01f)) { edited = true; }

			int modeIdx = static_cast<int>(key.mode);
			if (ImGui::Combo("Tangent", &modeIdx, TangentModeNames, static_cast<int>(TangentMode::kMaxCount))) {
				key.mode = static_cast<TangentMode>(modeIdx);
				edited = true;
			}

			// 自動計算のモードでは傾きを直接触らせない
			ImGui::BeginDisabled(key.mode == TangentMode::kAuto || key.mode == TangentMode::kLinear);
			if (ImGui::DragFloat("InTangent", &key.inTangent, 0.01f)) {
				if (key.mode == TangentMode::kFree) { key.outTangent = key.inTangent; }
				edited = true;
			}
			if (ImGui::DragFloat("OutTangent", &key.outTangent, 0.01f)) {
				if (key.mode == TangentMode::kFree) { key.inTangent = key.outTangent; }
				edited = true;
			}
			ImGui::EndDisabled();

			if (edited) {
				curve.SetKey(selected, key);
				changed = true;
			}
		}
		storage->SetInt(selectedId, selected);
		storage->SetInt(dragId, static_cast<int>(drag));

		ImGui::PopID();
		return changed;
	}
}
