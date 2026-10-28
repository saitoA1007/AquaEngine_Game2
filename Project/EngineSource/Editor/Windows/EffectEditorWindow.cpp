#include "pch.h"
#include "EffectEditorWindow.h"
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <cmath>
#include "ModelManager.h"
#include "GameParamEditor.h"
#include "JsonSerializer.h"
#include "Effects/EffectsManager.h"

using namespace GameEngine;

namespace {

	/// <summary>
	/// エフェクトの編集前後の状態を保持するコマンド
	/// </summary>
	class EffectSnapshotCommand : public IEditorCommand {
	public:
		EffectSnapshotCommand(EffectEditorWindow* window, const EffectAsset& before, const EffectAsset& after)
			: window_(window), before_(before), after_(after) {
		}

		void Execute() override { window_->ApplySnapshot(after_); }
		void Undo() override { window_->ApplySnapshot(before_); }

	private:
		EffectEditorWindow* window_ = nullptr;
		EffectAsset before_;
		EffectAsset after_;
	};

	/// <summary>
	/// std::stringを編集するInputText
	/// </summary>
	/// <returns>値が変更されたか</returns>
	bool InputString(const char* label, std::string& str) {
		char buffer[128] = {};
		strncpy_s(buffer, str.c_str(), sizeof(buffer) - 1);
		if (ImGui::InputText(label, buffer, sizeof(buffer))) {
			str = buffer;
			return true;
		}
		return false;
	}

	/// <summary>
	/// 文字列の一覧から選択するCombo
	/// </summary>
	/// <returns>値が変更されたか</returns>
	bool ComboString(const char* label, std::string& value, const std::vector<std::string>& items) {
		bool changed = false;
		const char* preview = value.empty() ? "(None)" : value.c_str();
		if (ImGui::BeginCombo(label, preview)) {
			for (const auto& item : items) {
				bool isSelected = (item == value);
				if (ImGui::Selectable(item.c_str(), isSelected)) {
					value = item;
					changed = true;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	// ツールバー用のアイコンボタン
	bool IconButton(const char* id, ImTextureID tex, const char* tooltip) {
		const bool pressed = ImGui::ImageButton(id, tex, ImVec2(16, 16));
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tooltip); }
		return pressed;
	}
}

EffectEditorWindow::EffectEditorWindow(TextureManager* textureManager, ModelManager* modelManager, GameParamEditor* gameParamEditor,
	EffectsManager* effectsManager) {
	textureManager_ = textureManager;
	modelManager_ = modelManager;
	gameParamEditor_ = gameParamEditor;
	effectsManager_ = effectsManager;

	uint32_t saveIconGH = textureManager_->GetHandleByName("saveIcon.png");
	uint32_t loadIconGH = textureManager_->GetHandleByName("loadIcon.png");
	uint32_t newFileIconGH = textureManager_->GetHandleByName("newFileIcon.png");
	uint32_t undoIconGH = textureManager_->GetHandleByName("undoIcon.png");
	uint32_t redoIconGH = textureManager_->GetHandleByName("redoIcon.png");
	uint32_t playIconGH = textureManager_->GetHandleByName("debugPlay.png");
	uint32_t stopIconGH = textureManager_->GetHandleByName("debugStop.png");
	uint32_t leftSkipIconGH = textureManager_->GetHandleByName("debugLeftStep.png");
	uint32_t rightSkipIconGH = textureManager_->GetHandleByName("debugRightStep.png");
	uint32_t addIconGH = textureManager_->GetHandleByName("addIcon.png");
	uint32_t deleteIconGH = textureManager_->GetHandleByName("deleteIcon.png");
	uint32_t copyIconGH = textureManager_->GetHandleByName("copyIcon.png");
	uint32_t clearIconGH = textureManager_->GetHandleByName("clearIcon.png");

	saveIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(saveIconGH);
	loadIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(loadIconGH);
	newFileIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(newFileIconGH);
	undoIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(undoIconGH);
	redoIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(redoIconGH);
	playIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(playIconGH);
	stopIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(stopIconGH);
	leftSkipIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(leftSkipIconGH);
	rightSkipIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(rightSkipIconGH);
	addIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(addIconGH);
	deleteIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(deleteIconGH);
	copyIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(copyIconGH);
	clearIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(clearIconGH);

	RefreshEffectList();
}

void EffectEditorWindow::Draw() {
	std::string title = "Effect Editor";
	if (hasAsset_) {
		title += " - " + asset_.name + (isDirty_ ? "*" : "");
	}
	title += "###EffectEditor";

	const bool isOpen = ImGui::Begin(title.c_str(), &isActive);

	// フォーカス中はこのウィンドウのショートカットを優先する
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
		EditorShortcut::ReportLocalFocus(ImGui::GetFrameCount());
		HandleShortcuts();
	}

	if (isOpen) {
		DrawFileBar();

		if (hasAsset_) {
			ImGui::Separator();

			// 下端の再生バーを除いた高さを、左からトラック一覧、タイムライン、プロパティで分ける
			const ImGuiStyle& style = ImGui::GetStyle();
			const float playbackHeight = ImGui::GetFrameHeight() + style.ItemSpacing.y * 2.0f;
			const float areaHeight = (std::max)(ImGui::GetContentRegionAvail().y - playbackHeight, 60.0f);
			constexpr float kSplitterWidth = 6.0f;

			// 左側のトラック一覧
			ImGui::BeginChild("TrackList", ImVec2(trackListWidth_, areaHeight), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
			DrawTrackList();
			ImGui::EndChild();

			// 中央のタイムライン
			ImGui::SameLine();
			const float timelineWidth = (std::max)(
				ImGui::GetContentRegionAvail().x - propertiesWidth_ - kSplitterWidth - style.ItemSpacing.x, 100.0f);
			ImGui::BeginChild("TimelineArea", ImVec2(timelineWidth, areaHeight), ImGuiChildFlags_Borders);
			DrawTimeline();
			ImGui::EndChild();

			// 境界をドラッグしてプロパティの幅を変える
			ImGui::SameLine();
			DrawSplitter(kSplitterWidth, areaHeight);

			// 右側のプロパティ
			ImGui::SameLine();
			ImGui::BeginChild("Properties", ImVec2(0.0f, areaHeight), ImGuiChildFlags_Borders);
			DrawProperties();
			ImGui::EndChild();

			// 下端側の再生バー
			DrawPlaybackBar();
		} else {
			ImGui::TextDisabled("Select or create an effect.");
		}
	}
	ImGui::End();

	// 操作が終わった変更を履歴に積む
	CommitChanges();

	// ウィンドウを閉じた場合はプレビューを消す
	if (!isActive) {
		preview_.reset();
		return;
	}

	UpdatePreview();
}

void EffectEditorWindow::RefreshEffectList() {
	effectNames_.clear();
	if (!JsonSerializer::DirectoryExists(EffectAsset::kDirectoryPath)) {
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(EffectAsset::kDirectoryPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			effectNames_.push_back(entry.path().stem().string());
		}
	}
	std::sort(effectNames_.begin(), effectNames_.end());
}

void EffectEditorWindow::LoadEffect(const std::string& name) {
	EffectAsset asset;
	if (!EffectAsset::Load(name, asset)) {
		return;
	}
	asset_ = asset;
	hasAsset_ = true;
	isDirty_ = false;
	selectedTrack_ = asset_.tracks.empty() ? -1 : 0;
	isPlaying_ = false;
	needsRebuild_ = true;
	ResetHistory();
}

void EffectEditorWindow::DrawFileBar() {
	// エフェクトの選択
	ImGui::SetNextItemWidth(200.0f);
	const char* preview = hasAsset_ ? asset_.name.c_str() : "(None)";
	if (ImGui::BeginCombo("##Effect", preview)) {
		for (const auto& name : effectNames_) {
			bool isSelected = hasAsset_ && name == asset_.name;
			if (ImGui::Selectable(name.c_str(), isSelected)) {
				LoadEffect(name);
			}
		}
		ImGui::EndCombo();
	}

	// 新規作成
	ImGui::SameLine();
	if (IconButton("##new", static_cast<ImTextureID>(newFileIconTextureHandle_.ptr), "New")) {
		newEffectName_[0] = '\0';
		ImGui::OpenPopup("NewEffect");
	}
	if (ImGui::BeginPopup("NewEffect")) {
		ImGui::InputText("Name", newEffectName_, sizeof(newEffectName_));
		std::string name = newEffectName_;
		bool exists = std::find(effectNames_.begin(), effectNames_.end(), name) != effectNames_.end();
		if (exists) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already exists");
		}
		ImGui::BeginDisabled(name.empty() || exists);
		if (ImGui::Button("Create")) {
			asset_ = EffectAsset{};
			asset_.name = name;
			hasAsset_ = true;
			selectedTrack_ = -1;
			isPlaying_ = false;
			isDirty_ = true;
			needsRebuild_ = true;
			ResetHistory();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::EndPopup();
	}

	// 保存
	ImGui::SameLine();
	ImGui::BeginDisabled(!hasAsset_);
	if (IconButton("##save", static_cast<ImTextureID>(saveIconTextureHandle_.ptr), "Save")) {
		Save();
	}
	ImGui::EndDisabled();

	// Undo
	ImGui::SameLine();
	ImGui::BeginDisabled(!history_.CanUndo());
	if (IconButton("##undo", static_cast<ImTextureID>(undoIconTextureHandle_.ptr), "Undo")) {
		history_.Undo();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	// redo
	ImGui::BeginDisabled(!history_.CanRedo());
	if (IconButton("##redo", static_cast<ImTextureID>(redoIconTextureHandle_.ptr), "Redo")) {
		history_.Redo();
	}
	ImGui::EndDisabled();

	// 一覧の再読み込み
	ImGui::SameLine();
	if (IconButton("##Refresh", static_cast<ImTextureID>(loadIconTextureHandle_.ptr), "Refresh")) {
		RefreshEffectList();
	}
}

void EffectEditorWindow::DrawProperties() {
	// 選択中トラックの設定
	DrawTrackInspector();
}

void EffectEditorWindow::DrawSplitter(float width, float height) {
	const ImVec2 min = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##Splitter", ImVec2(width, height));

	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	if (ImGui::IsItemActive()) {
		// 左に動かすとプロパティが広くなる
		propertiesWidth_ = std::clamp(propertiesWidth_ - ImGui::GetIO().MouseDelta.x, 120.0f, 800.0f);
	}

	const ImVec2 max = ImGui::GetItemRectMax();
	const ImU32 color = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_SeparatorActive
		: (ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(min.x + width * 0.5f - 1.0f, min.y), ImVec2(min.x + width * 0.5f + 1.0f, max.y), color);
}

void EffectEditorWindow::DrawPlaybackBar() {

	// 再生 / 停止
	D3D12_GPU_DESCRIPTOR_HANDLE  playTexId = isPlaying_ ? stopIconTextureHandle_ : playIconTextureHandle_;
	if (IconButton("##play", static_cast<ImTextureID>(playTexId.ptr), "Play")) {
		TogglePlay();
	}
	// コマ送り
	constexpr float kFrameStep = 1.0f / 60.0f;
	ImGui::SameLine();
	if (IconButton("##prevFrame", static_cast<ImTextureID>(leftSkipIconTextureHandle_.ptr), "PrevFrame") && preview_) {
		SeekPreview(preview_->GetTime() - kFrameStep);
	}
	ImGui::SameLine();
	if (IconButton("##NextFrame", static_cast<ImTextureID>(rightSkipIconTextureHandle_.ptr), "NextFrame") && preview_) {
		SeekPreview(preview_->GetTime() + kFrameStep);
	}

	// 最初に戻す
	ImGui::SameLine();
	if (IconButton("##Clear", static_cast<ImTextureID>(clearIconTextureHandle_.ptr), "Clear")) {
		isPlaying_ = false;
		if (preview_) {
			preview_->StopImmediate();
		}
	}
	// 再生時間
	ImGui::SameLine();
	float time = preview_ ? preview_->GetTime() : 0.0f;
	ImGui::Text("%.2f / %.2f s", time, asset_.duration);

	ImGui::SameLine();
	if (ImGui::Checkbox("Loop", &asset_.isLoop)) {
		MarkTimingChanged();
	}
	ImGui::SameLine();
	ImGui::Checkbox("AutoReplay", &isAutoReplay_);

	// エフェクト全体の長さ
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::DragFloat("Duration", &asset_.duration, 0.01f, 0.01f, 60.0f, "%.2f s")) {
		asset_.duration = (std::max)(asset_.duration, 0.01f);
		MarkTimingChanged();
	}

	// プレビューの位置は保存しないので、変更として記録しない
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::DragFloat3("PreviewPos", &previewPos_.x, 0.1f);
}

void EffectEditorWindow::DrawTrackList() {
	// トラックの一覧
	ImGui::SeparatorText("Tracks");

	for (int i = 0; i < static_cast<int>(asset_.tracks.size()); ++i) {
		const EffectTrackData& track = asset_.tracks[i];
		std::string label = track.name + (track.emitMode == EffectEmitMode::kBurst ? " [B]" : "") + "##" + std::to_string(i);
		if (ImGui::Selectable(label.c_str(), selectedTrack_ == i)) {
			selectedTrack_ = i;
		}
	}

	if (asset_.tracks.empty()) {
		ImGui::TextDisabled("No tracks");
	}

	ImGui::Separator();
	DrawTrackButtons();
}

void EffectEditorWindow::DrawTrackButtons() {

	// 追加
	if (IconButton("##add", static_cast<ImTextureID>(addIconTextureHandle_.ptr), "Add")) {
		EffectTrackData track;
		track.name = "Track" + std::to_string(asset_.tracks.size());
		std::vector<std::string> models = GetModelNames();
		if (std::find(models.begin(), models.end(), "plane.obj") != models.end()) {
			track.modelName = "plane.obj";
		}
		asset_.tracks.push_back(track);
		selectedTrack_ = static_cast<int>(asset_.tracks.size()) - 1;
		MarkStructureChanged();
	}

	const bool hasSelection = 0 <= selectedTrack_ && selectedTrack_ < static_cast<int>(asset_.tracks.size());
	ImGui::BeginDisabled(!hasSelection);

	// 複製
	ImGui::SameLine();
	if (IconButton("##copy", static_cast<ImTextureID>(copyIconTextureHandle_.ptr), "Copy")) {
		EffectTrackData track = asset_.tracks[selectedTrack_];
		track.name += "_Copy";
		asset_.tracks.insert(asset_.tracks.begin() + selectedTrack_ + 1, track);
		selectedTrack_++;
		MarkStructureChanged();
	}

	// 削除
	ImGui::SameLine();
	if (IconButton("##delete", static_cast<ImTextureID>(deleteIconTextureHandle_.ptr), "Delete")) {
		asset_.tracks.erase(asset_.tracks.begin() + selectedTrack_);
		selectedTrack_ = (std::min)(selectedTrack_, static_cast<int>(asset_.tracks.size()) - 1);
		MarkStructureChanged();
	}

	ImGui::EndDisabled();
}

void EffectEditorWindow::DrawTimeline() {
	constexpr float kNameWidth = 140.0f;   // トラック名の列の幅
	constexpr float kRulerHeight = 20.0f;  // 目盛りの高さ
	constexpr float kRowHeight = 22.0f;    // 1トラックの高さ
	constexpr float kBarInset = 3.0f;      // バーの上下の余白

	const int trackCount = static_cast<int>(asset_.tracks.size());
	const float contentHeight = kRulerHeight + kRowHeight * (std::max)(trackCount, 1) + 4.0f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float timelineX = origin.x + kNameWidth;
	const float timelineWidth = (std::max)(totalWidth - kNameWidth, 50.0f);
	const float duration = (std::max)(asset_.duration, 0.01f);
	const float pixelsPerSec = timelineWidth / duration;

	// 時間をX座標に変換
	auto timeToX = [&](float t) { return timelineX + t * pixelsPerSec; };

	const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_Border);
	const ImU32 rowColorA = ImGui::GetColorU32(ImGuiCol_FrameBg);
	const ImU32 rowColorB = ImGui::GetColorU32(ImGuiCol_FrameBg, 0.5f);
	const ImU32 selectedRowColor = ImGui::GetColorU32(ImGuiCol_Header);

	// ===== 目盛り =====
	{
		// 目盛りの間隔が50px以上になる刻みを選ぶ
		const float steps[] = { 0.01f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
		float step = steps[IM_ARRAYSIZE(steps) - 1];
		for (float s : steps) {
			if (s * pixelsPerSec >= 50.0f) {
				step = s;
				break;
			}
		}

		const int tickCount = static_cast<int>(duration / step + 0.001f);
		for (int i = 0; i <= tickCount; ++i) {
			float t = i * step;
			float x = timeToX(t);
			drawList->AddLine(ImVec2(x, origin.y + kRulerHeight - 6.0f), ImVec2(x, origin.y + kRulerHeight), lineColor);
			char label[16];
			snprintf(label, sizeof(label), step < 0.1f ? "%.2f" : "%.1f", t);
			drawList->AddText(ImVec2(x + 2.0f, origin.y + 2.0f), textColor, label);
		}
		drawList->AddLine(ImVec2(timelineX, origin.y + kRulerHeight), ImVec2(timelineX + timelineWidth, origin.y + kRulerHeight), lineColor);

		// 目盛りをクリック・ドラッグで再生位置を移動する（スクラブ）
		ImGui::SetCursorScreenPos(ImVec2(timelineX, origin.y));
		ImGui::InvisibleButton("Ruler", ImVec2(timelineWidth, kRulerHeight));
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		if (ImGui::IsItemActive() && preview_) {
			const ImGuiIO& io = ImGui::GetIO();
			const float snap = io.KeyShift ? 0.1f : 0.01f;
			float t = (io.MousePos.x - timelineX) / pixelsPerSec;
			t = std::round(t / snap) * snap;
			// 同じ時間への移動は再シミュレーションしない
			if (isPlaying_ || std::abs(t - preview_->GetTime()) > 0.0001f) {
				SeekPreview(t);
			}
		}
	}

	// ===== トラック =====
	for (int i = 0; i < trackCount; ++i) {
		EffectTrackData& track = asset_.tracks[i];
		ImGui::PushID(i);

		const float rowY = origin.y + kRulerHeight + kRowHeight * i;
		const ImVec2 rowMin(origin.x, rowY);
		const ImVec2 rowMax(timelineX + timelineWidth, rowY + kRowHeight);
		const bool isSelected = (selectedTrack_ == i);

		// 行の背景
		drawList->AddRectFilled(rowMin, rowMax, isSelected ? selectedRowColor : (i % 2 == 0 ? rowColorA : rowColorB));

		// トラック名
		ImGui::SetCursorScreenPos(rowMin);
		if (ImGui::InvisibleButton("Name", ImVec2(kNameWidth - 10.0f, kRowHeight))) {
			selectedTrack_ = i;
		}
		drawList->PushClipRect(rowMin, ImVec2(timelineX - 4.0f, rowMax.y), true);
		drawList->AddText(ImVec2(rowMin.x + 4.0f, rowY + (kRowHeight - ImGui::GetTextLineHeight()) * 0.5f), textColor, track.name.c_str());
		drawList->PopClipRect();

		// エフェクトの長さを超えている場合は警告色にする
		const bool isOverflow = track.GetEndTime() > asset_.duration;
		const float barTop = rowY + kBarInset;
		const float barBottom = rowY + kRowHeight - kBarInset;

		if (track.emitMode == EffectEmitMode::kBurst) {
			// Burstは開始位置にひし形を表示する
			const float cx = timeToX(track.startTime);
			const float cy = rowY + kRowHeight * 0.5f;
			const float r = kRowHeight * 0.5f - kBarInset;
			ImU32 color = isOverflow ? IM_COL32(230, 90, 90, 255) : IM_COL32(240, 160, 60, 255);
			drawList->AddQuadFilled(ImVec2(cx, cy - r), ImVec2(cx + r, cy), ImVec2(cx, cy + r), ImVec2(cx - r, cy), color);
			if (isSelected) {
				drawList->AddQuad(ImVec2(cx, cy - r), ImVec2(cx + r, cy), ImVec2(cx, cy + r), ImVec2(cx - r, cy), IM_COL32_WHITE, 1.5f);
			}

			HandleBarDrag(i, ImVec2(cx - r, barTop), ImVec2(cx + r, barBottom), DragPart::kMove, pixelsPerSec);
		} else {
			// Continuousは開始から終了までのバーを表示する
			const float x0 = timeToX(track.startTime);
			const float x1 = (std::max)(timeToX(track.GetEndTime()), x0 + 2.0f);
			ImU32 color = isOverflow ? IM_COL32(230, 90, 90, 255) : IM_COL32(80, 150, 230, 255);
			drawList->AddRectFilled(ImVec2(x0, barTop), ImVec2(x1, barBottom), color, 3.0f);
			if (isSelected) {
				drawList->AddRect(ImVec2(x0, barTop), ImVec2(x1, barBottom), IM_COL32_WHITE, 3.0f, 0, 1.5f);
			}

			// 端をつかむ範囲
			constexpr float kHandle = 4.0f;
			HandleBarDrag(i, ImVec2(x0 - kHandle, barTop), ImVec2(x0 + kHandle, barBottom), DragPart::kResizeLeft, pixelsPerSec);
			HandleBarDrag(i, ImVec2(x1 - kHandle, barTop), ImVec2(x1 + kHandle, barBottom), DragPart::kResizeRight, pixelsPerSec);
			if (x1 - x0 > kHandle * 2.0f) {
				HandleBarDrag(i, ImVec2(x0 + kHandle, barTop), ImVec2(x1 - kHandle, barBottom), DragPart::kMove, pixelsPerSec);
			}
		}

		ImGui::PopID();
	}

	if (trackCount == 0) {
		drawList->AddText(ImVec2(origin.x + 4.0f, origin.y + kRulerHeight + 4.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), "No tracks. Press Add.");
	}

	// 名前の列とタイムラインの境界線
	const float bottom = origin.y + kRulerHeight + kRowHeight * (std::max)(trackCount, 1);
	drawList->AddLine(ImVec2(timelineX, origin.y), ImVec2(timelineX, bottom), lineColor);

	// ===== 再生位置 =====
	if (preview_) {
		const float x = timeToX(std::clamp(preview_->GetTime(), 0.0f, duration));
		const ImU32 headColor = IM_COL32(255, 80, 80, 255);
		drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, bottom), headColor, 1.5f);
		drawList->AddTriangleFilled(ImVec2(x - 5.0f, origin.y), ImVec2(x + 5.0f, origin.y), ImVec2(x, origin.y + 7.0f), headColor);
	}

	// スクロール範囲を確保する
	ImGui::SetCursorScreenPos(origin);
	ImGui::Dummy(ImVec2(totalWidth, contentHeight));
}

void EffectEditorWindow::HandleBarDrag(int trackIndex, const ImVec2& min, const ImVec2& max, DragPart part, float pixelsPerSec) {
	const char* id = "Move";
	if (part == DragPart::kResizeLeft) { id = "ResizeL"; }
	if (part == DragPart::kResizeRight) { id = "ResizeR"; }

	ImGui::SetCursorScreenPos(min);
	ImGui::InvisibleButton(id, ImVec2((std::max)(max.x - min.x, 1.0f), (std::max)(max.y - min.y, 1.0f)));

	EffectTrackData& track = asset_.tracks[trackIndex];
	const ImGuiIO& io = ImGui::GetIO();

	// カーソル
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImGui::SetMouseCursor(part == DragPart::kMove ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_ResizeEW);
	}

	// ツールチップ
	if (ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
		if (track.emitMode == EffectEmitMode::kBurst) {
			ImGui::SetTooltip("%s\nStart: %.2f s", track.name.c_str(), track.startTime);
		} else {
			ImGui::SetTooltip("%s\nStart: %.2f s\nEnd: %.2f s\nDuration: %.2f s", track.name.c_str(), track.startTime, track.GetEndTime(), track.duration);
		}
	}

	// ドラッグ開始
	if (ImGui::IsItemActivated()) {
		selectedTrack_ = trackIndex;
		drag_.track = trackIndex;
		drag_.part = part;
		drag_.startMouseX = io.MousePos.x;
		drag_.originalStart = track.startTime;
		drag_.originalDuration = track.duration;
	}

	// ドラッグ中
	if (ImGui::IsItemActive() && drag_.track == trackIndex && drag_.part == part) {
		const float snap = io.KeyShift ? 0.1f : 0.01f;
		auto snapTime = [snap](float t) { return std::round(t / snap) * snap; };

		const float delta = (io.MousePos.x - drag_.startMouseX) / pixelsPerSec;
		const float effectDuration = asset_.duration;
		const float originalEnd = drag_.originalStart + drag_.originalDuration;
		constexpr float kMinDuration = 0.01f;

		float newStart = track.startTime;
		float newDuration = track.duration;

		switch (part) {
		case DragPart::kMove: {
			// 長さを保ったまま移動する
			const float length = track.emitMode == EffectEmitMode::kBurst ? 0.0f : drag_.originalDuration;
			const float maxStart = (std::max)(effectDuration - length, 0.0f);
			newStart = std::clamp(snapTime(drag_.originalStart + delta), 0.0f, maxStart);
			break;
		}
		case DragPart::kResizeLeft:
			// 終了時間を固定して開始時間を変更する
			newStart = std::clamp(snapTime(drag_.originalStart + delta), 0.0f, (std::max)(originalEnd - kMinDuration, 0.0f));
			newDuration = originalEnd - newStart;
			break;
		case DragPart::kResizeRight: {
			// 開始時間を固定して終了時間を変更する
			const float newEnd = std::clamp(snapTime(originalEnd + delta), drag_.originalStart + kMinDuration, (std::max)(effectDuration, drag_.originalStart + kMinDuration));
			newDuration = newEnd - drag_.originalStart;
			break;
		}
		default:
			break;
		}

		if (newStart != track.startTime || newDuration != track.duration) {
			track.startTime = newStart;
			track.duration = newDuration;
			MarkTimingChanged();
		}
	}

	// ドラッグ終了
	if (ImGui::IsItemDeactivated() && drag_.track == trackIndex && drag_.part == part) {
		drag_ = DragState{};
	}
}

void EffectEditorWindow::DrawTrackInspector() {
	ImGui::SeparatorText("Track");

	if (selectedTrack_ < 0 || selectedTrack_ >= static_cast<int>(asset_.tracks.size())) {
		ImGui::TextDisabled("No track selected");
		return;
	}

	EffectTrackData& track = asset_.tracks[selectedTrack_];

	// 表示名
	if (InputString("Name", track.name)) {
		MarkTimingChanged();
	}

	ImGui::SeparatorText("Particle");

	// パーティクルの一覧から選択
	if (ComboString("Particle", track.particleName, GetParticleNames())) {
		MarkStructureChanged();
	}
	if (InputString("ParticleName", track.particleName)) {
		MarkDirty();
	}
	// 入力中に毎フレーム作り直さないよう、入力完了時に反映する
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		needsRebuild_ = true;
	}
	ImGui::TextDisabled("Particle parameters are edited in the Inspector.");

	// モデル
	if (ComboString("Model", track.modelName, GetModelNames())) {
		MarkStructureChanged();
	}

	// 最大数
	int maxNum = static_cast<int>(track.maxNum);
	if (ImGui::DragInt("MaxNum", &maxNum, 1.0f, 1, 10000)) {
		track.maxNum = static_cast<uint32_t>((std::max)(maxNum, 1));
		MarkDirty();
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		needsRebuild_ = true;
	}

	ImGui::SeparatorText("Timing");

	// 発生方法
	const char* emitModes[] = { "Continuous", "Burst" };
	int emitMode = static_cast<int>(track.emitMode);
	if (ImGui::Combo("EmitMode", &emitMode, emitModes, IM_ARRAYSIZE(emitModes))) {
		track.emitMode = static_cast<EffectEmitMode>(emitMode);
		MarkTimingChanged();
	}

	if (ImGui::DragFloat("StartTime", &track.startTime, 0.01f, 0.0f, asset_.duration, "%.2f s")) {
		track.startTime = std::clamp(track.startTime, 0.0f, asset_.duration);
		MarkTimingChanged();
	}

	ImGui::BeginDisabled(track.emitMode == EffectEmitMode::kBurst);
	if (ImGui::DragFloat("Duration##Track", &track.duration, 0.01f, 0.0f, asset_.duration, "%.2f s")) {
		track.duration = (std::max)(track.duration, 0.0f);
		MarkTimingChanged();
	}
	ImGui::EndDisabled();

	if (track.GetEndTime() > asset_.duration) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Track ends after the effect duration.");
	}

	ImGui::SeparatorText("Transform");

	if (ImGui::DragFloat3("Offset", &track.offset.x, 0.01f)) {
		MarkTimingChanged();
	}
}

void EffectEditorWindow::UpdatePreview() {
	if (!hasAsset_) {
		return;
	}

	// 構成が変わったら作り直す
	if (needsRebuild_ || !preview_) {
		const float prevTime = preview_ ? preview_->GetTime() : 0.0f;
		preview_ = std::make_unique<EffectObject>(asset_, textureManager_, modelManager_);
		preview_->SetPosition(previewPos_);
		needsRebuild_ = false;
		needsApplyTiming_ = false;
		// 作り直す前と同じ時間の状態にする
		preview_->Seek(prevTime);
	}

	// タイミングの変更を反映する
	if (needsApplyTiming_) {
		preview_->ApplyTiming(asset_);
		needsApplyTiming_ = false;
		if (!isPlaying_) {
			// 変更後の設定で同じ時間までシミュレーションし直す
			const float time = preview_->GetTime();
			preview_->Play(previewPos_);
			preview_->Seek(time);
		}
	}

	preview_->SetPosition(previewPos_);

	if (isPlaying_) {
		// 再生が終わったら最初から再生する
		if (isAutoReplay_ && !preview_->IsPlaying()) {
			preview_->Play(previewPos_);
		}
		preview_->Update();
	}
	preview_->Draw();
}

void EffectEditorWindow::TogglePlay() {
	if (isPlaying_) {
		// 停止する
		isPlaying_ = false;
		return;
	}

	isPlaying_ = true;

	// 再生が終わっている場合は最初から再生する
	if (preview_ && !preview_->IsPlaying()) {
		preview_->Play(previewPos_);
	}
}

void EffectEditorWindow::SeekPreview(float time) {
	if (!preview_) {
		return;
	}
	isPlaying_ = false;
	preview_->SetPosition(previewPos_);
	preview_->Seek(std::clamp(time, 0.0f, asset_.duration));
}

void EffectEditorWindow::ApplySnapshot(const EffectAsset& snapshot) {
	if (IsSameStructure(asset_, snapshot)) {
		needsApplyTiming_ = true;
	} else {
		needsRebuild_ = true;
	}

	asset_ = snapshot;
	committedAsset_ = snapshot;
	hasPendingChange_ = false;
	isDirty_ = true;
	drag_ = DragState{};

	// 選択中のトラックが無くなった場合は範囲内に収める
	const int trackCount = static_cast<int>(asset_.tracks.size());
	if (selectedTrack_ >= trackCount) {
		selectedTrack_ = trackCount - 1;
	}
}

void EffectEditorWindow::Save() {
	if (!hasAsset_) {
		return;
	}
	asset_.Save();

	// ゲーム中のEffectsManagerにも反映する
	if (effectsManager_ != nullptr) {
		effectsManager_->RegisterEffect(asset_);
	}

	isDirty_ = false;
	RefreshEffectList();
}

void EffectEditorWindow::ResetHistory() {
	history_.Clear();
	committedAsset_ = asset_;
	hasPendingChange_ = false;
	drag_ = DragState{};
}

void EffectEditorWindow::CommitChanges() {
	// ドラッグや入力の途中では積まない
	if (!hasPendingChange_ || ImGui::IsAnyItemActive()) {
		return;
	}
	history_.Execute(std::make_unique<EffectSnapshotCommand>(this, committedAsset_, asset_));
}

void EffectEditorWindow::HandleShortcuts() {
	const ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput || !hasAsset_) {
		return;
	}

	if (io.KeyCtrl) {
		// Ctrl+Z : Undo / Ctrl+Shift+Z, Ctrl+Y : Redo
		if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
			if (io.KeyShift) {
				history_.Redo();
			} else {
				history_.Undo();
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
			history_.Redo();
		}
		// Ctrl+S : 保存
		if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
			Save();
		}
	} else if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
		// Space : 一時停止 / 再開
		TogglePlay();
	}
}

bool EffectEditorWindow::IsSameStructure(const EffectAsset& a, const EffectAsset& b) {
	if (a.tracks.size() != b.tracks.size()) {
		return false;
	}
	for (size_t i = 0; i < a.tracks.size(); ++i) {
		const EffectTrackData& ta = a.tracks[i];
		const EffectTrackData& tb = b.tracks[i];
		if (ta.particleName != tb.particleName || ta.modelName != tb.modelName || ta.maxNum != tb.maxNum) {
			return false;
		}
	}
	return true;
}

std::vector<std::string> EffectEditorWindow::GetParticleNames() const {
	std::vector<std::string> names;
	// パーティクルは"Emitter"のサブグループを持つ
	for (const auto& [name, group] : gameParamEditor_->GetAllGroups()) {
		if (group.children.contains("Emitter")) {
			names.push_back(name);
		}
	}
	return names;
}

std::vector<std::string> EffectEditorWindow::GetModelNames() const {
	std::vector<std::string> names;
	for (const auto& [handle, entry] : modelManager_->GetModels()) {
		if (entry.model) {
			names.push_back(entry.name);
		}
	}
	std::sort(names.begin(), names.end());
	return names;
}
