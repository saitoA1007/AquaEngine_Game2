#pragma once
#include<iostream>
#include<fstream>
#include<Windows.h>
#include<filesystem>
#include<chrono>
#include<unordered_set>

// ログの書き込むデータ
struct LogData {
	std::string message;    // 表示する文字
	std::string groupName;  // ログのグループ名
};

class LogManager {
public:

	/// <summary>
	/// インスタンスを取得
	/// </summary>
	/// <returns></returns>
	static LogManager& GetInstance() {
		static LogManager instance;
		return instance;
	}

	/// <summary>
	/// ログを書き込むため準備
	/// </summary>
	void Create();

	/// <summary>
	/// ログを出力
	/// </summary>
	/// <param name="message">出力するメッセージ</param>
	void Log(const std::string& message);

	/// <summary>
	/// デバック時にコンソールに出力する用のログ
	/// </summary>
	/// <param name="message"></param>
	/// <param name="groupName"></param>
	void Log(const std::string& message, const std::string& groupName);

	/// <summary>
	/// ログをクリア
	/// </summary>
	void ClearLogs();

	/// <summary>
	/// ログを保存したファイルを取得
	/// </summary>
	/// <returns></returns>
	const std::vector<LogData>& GetLogs() const { return logs_; }

	/// <summary>
	/// 登録しているグループを取得
	/// </summary>
	/// <returns></returns>
	const std::unordered_set<std::string>& GetGroups() const { return groups_; }

private:
	static std::ofstream logStream_;
	// ログを保存する
	std::vector<LogData> logs_;
	// ログの所属するグループを保存する
	std::unordered_set<std::string> groups_;

	// 保存するログの数
	const size_t kMaxLogs = 200;
};

/// <summary>
///	ログを出力する
/// </summary>
/// <param name="message">メッセージを入力</param>
/// <param name="groupName">グループ名を登録 : デフォルトではGenerate</param>
void Log(const std::string& message, const std::string& groupName = "Generate");