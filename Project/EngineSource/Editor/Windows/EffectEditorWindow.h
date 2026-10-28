#pragma once
#include <string>
#include <vector>
#include <memory>
#include "IEditorWindow.h"
#include "ImGuiManager.h"
#include "Effects/EffectObject.h"
#include "Command/EditorCommandHistory.h"

namespace GameEngine {

	// 前方宣言
	class TextureManager;
	class ModelManager;
	class GameParamEditor;
	class EffectsManager;

	/// <summary>
	/// パーティクルを組み合わせてエフェクトを作成するエディタウィンドウ
	/// </summary>
	class EffectEditorWindow : public IEditorWindow {
	public:

		// タイムラインのドラッグ操作の種類
		enum class DragPart {
			kNone,
			kMove,        // バー全体を移動
			kResizeLeft,  // 開始時間を変更
			kResizeRight, // 終了時間を変更
		};

		// ドラッグ中の状態
		struct DragState {
			int track = -1;
			DragPart part = DragPart::kNone;
			float startMouseX = 0.0f;
			float originalStart = 0.0f;
			float originalDuration = 0.0f;
		};

	public:
		EffectEditorWindow(TextureManager* textureManager, ModelManager* modelManager, GameParamEditor* gameParamEditor,
			EffectsManager* effectsManager);

		void Draw() override;
		std::string GetName() const override { return "EffectEditor"; }

		/// <summary>
		/// 編集中のエフェクトを指定した状態に戻す
		/// </summary>
		/// <param name="snapshot">戻す状態</param>
		void ApplySnapshot(const EffectAsset& snapshot);

	private:
		TextureManager* textureManager_ = nullptr;
		ModelManager* modelManager_ = nullptr;
		GameParamEditor* gameParamEditor_ = nullptr;
		EffectsManager* effectsManager_ = nullptr;

		// 編集中のエフェクト
		EffectAsset asset_;
		bool hasAsset_ = false;

		// 保存されていない変更があるか
		bool isDirty_ = false;

		// 保存されているエフェクトの名前
		std::vector<std::string> effectNames_;

		// 選択中のトラック
		int selectedTrack_ = -1;

		// プレビュー
		std::unique_ptr<EffectObject> preview_;
		// プレビューの位置
		Vector3 previewPos_ = { 0.0f,0.0f,0.0f };
		// プレビューの時間を進めているか
		bool isPlaying_ = false;
		// 再生が終わったら自動で最初から再生するか
		bool isAutoReplay_ = true;
		// パーティクルの作り直しが必要か
		bool needsRebuild_ = false;
		// タイミングの反映が必要か
		bool needsApplyTiming_ = false;

		// Undo/Redoの履歴
		EditorCommandHistory history_;
		// 最後に履歴に積んだ状態
		EffectAsset committedAsset_;
		// 履歴に積んでいない変更があるか
		bool hasPendingChange_ = false;

		// トラック一覧の幅
		float trackListWidth_ = 180.0f;
		// プロパティの幅
		float propertiesWidth_ = 300.0f;

		// 新規作成の名前入力
		char newEffectName_[64] = {};

		DragState drag_;

		// アイコン画像ハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE saveIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE loadIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE newFileIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE undoIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE redoIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE playIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE stopIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE leftSkipIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE rightSkipIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE addIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE deleteIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE copyIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE clearIconTextureHandle_{};

	private:

		// 保存されているエフェクトの一覧を更新する
		void RefreshEffectList();

		// エフェクトを読み込んで編集対象にする
		void LoadEffect(const std::string& name);

		// ファイル操作
		void DrawFileBar();

		// エフェクト全体と選択中トラックの設定
		void DrawProperties();

		/// <summary>
		/// プロパティの幅を変えるための境界線
		/// </summary>
		/// <param name="width">境界の当たり判定の幅</param>
		/// <param name="height">境界の高さ</param>
		void DrawSplitter(float width, float height);

		// 再生操作
		void DrawPlaybackBar();

		// トラックの一覧
		void DrawTrackList();

		// トラックの追加。複製。削除
		void DrawTrackButtons();

		// タイムライン
		void DrawTimeline();

		/// <summary>
		/// タイムライン上のバーの一部分のドラッグ操作
		/// </summary>
		/// <param name="trackIndex">トラックの番号</param>
		/// <param name="min">操作範囲の左上</param>
		/// <param name="max">操作範囲の右下</param>
		/// <param name="part">操作の種類</param>
		/// <param name="pixelsPerSec">1秒あたりのピクセル数</param>
		void HandleBarDrag(int trackIndex, const ImVec2& min, const ImVec2& max, DragPart part, float pixelsPerSec);

		// 選択中のトラックの設定
		void DrawTrackInspector();

		// プレビューの更新と描画
		void UpdatePreview();

		// GameParamEditorから取得したパーティクルの名前の一覧
		std::vector<std::string> GetParticleNames() const;

		// モデルの名前の一覧
		std::vector<std::string> GetModelNames() const;

		// 保存する
		void Save();

		// 編集対象を切り替えた時に履歴をリセットする
		void ResetHistory();

		// 操作が終わった変更を履歴に積む
		void CommitChanges();

		// ショートカットキー
		void HandleShortcuts();

		/// <summary>
		/// 再生と停止を切り替え
		/// </summary>
		void TogglePlay();

		// プレビューを指定した時間まで移動して停止する
		void SeekPreview(float time);

		// パーティクルの作り直しが不要な変更か
		static bool IsSameStructure(const EffectAsset& a, const EffectAsset& b);

		// 変更を記録する
		void MarkDirty() { isDirty_ = true; hasPendingChange_ = true; }
		void MarkTimingChanged() { MarkDirty(); needsApplyTiming_ = true; }
		void MarkStructureChanged() { MarkDirty(); needsRebuild_ = true; }
	};
}
