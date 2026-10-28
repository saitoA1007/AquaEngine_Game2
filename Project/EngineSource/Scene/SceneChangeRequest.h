#pragma once
#include <string>
#include <optional>
#include <vector>

class SceneChangeRequest {
public:

	/// <summary>
	/// シーンの切り替えをリクエストする
	/// </summary>
	/// <param name="nextScene">次のシーン名</param>
	void RequestChange(const std::string& nextScene) { requestSceneName_ = nextScene; }

	/// <summary>
	/// リクエストがあるかを取得
	/// </summary>
	/// <returns></returns>
	bool HasChangeRequest() const { return requestSceneName_.has_value(); }

	/// <summary>
	/// 変更したいシーン名を取得
	/// </summary>
	/// <returns></returns>
	const std::string& GetRequestScene() const { return requestSceneName_.value(); }

	/// <summary>
	/// シーンを変更するリクエストをクリア
	/// </summary>
	void ClearChangeRequest() { requestSceneName_.reset(); }

	/// <summary>
	/// 現在のシーン名を設定
	/// </summary>
	/// <param name="currentScene">現在のシーン名</param>
	void SetCurrentSceneName(const std::string& currentScene) { currentSceneName_ = currentScene; }

	/// <summary>
	/// 現在のシーン名を取得
	/// </summary>
	/// <returns></returns>
	const std::string& GetCurrentSceneName() const { return currentSceneName_; }

	/// <summary>
	/// シーンの名前を取得する
	/// </summary>
	/// <param name="sceneName"></param>
	void SetSceneNames(const std::vector<std::string>& sceneName) {
		sceneNames_ = sceneName;
	}

	/// <summary>
	/// 登録されているシーン名のリストを取得
	/// </summary>
	/// <returns></returns>
	const std::vector<std::string>& GetSceneNames() const {
		return sceneNames_;
	}

private:
	// リクエストするシーン名
	std::optional<std::string> requestSceneName_;
	// 現在のシーン名
	std::string currentSceneName_;

	// 登録されているシーン名
	std::vector<std::string> sceneNames_;
};