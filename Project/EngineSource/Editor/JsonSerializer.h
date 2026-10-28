#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <json.hpp>

namespace GameEngine {

	/// <summary>
	/// jsonによる保存、読み込みする汎用関数
	/// </summary>
	namespace JsonSerializer {

		/// <summary>
		/// jsonオブジェクトをファイルに保存する。ディレクトリが無ければ自動生成する。
		/// </summary>
		/// <param name="filePath">保存先のフルパス</param>
		/// <param name="root">保存するjsonオブジェクト</param>
		/// <param name="indent">インデント幅</param>
		void SaveToFile(const std::string& filePath, const nlohmann::json& root, int indent = 4);

		/// <summary>
		/// ファイルからjsonオブジェクトを読み込む。
		/// </summary>
		/// <param name="filePath">読み込むファイルのフルパス</param>
		/// <returns>読み込んだjsonオブジェクト</returns>
		nlohmann::json LoadFromFile(const std::string& filePath);

		/// <summary>
		/// ディレクトリ内の全.jsonファイルを読み込む。
		/// ファイルごとにコールバックを呼ぶ。
		/// </summary>
		/// <param name="directoryPath">読み込むディレクトリ</param>
		/// <param name="callback">ファイル名(拡張子なし)とjsonを受け取るコールバック</param>
		void LoadDirectory(const std::string& directoryPath, const std::function<void(const std::string&, const nlohmann::json&)>& callback);

		/// <summary>
		/// jsonノードに値を書き込む
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="node"></param>
		/// <param name="value"></param>
		template<typename T>
		void Serialize(nlohmann::json& node, const T& value) {
			node = value;
		}

		/// <summary>
		/// jsonノードから値を読み込む
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="node"></param>
		/// <returns></returns>
		template<typename T>
		T Deserialize(const nlohmann::json& node) {
			return node.get<T>();
		}

		/// <summary>
		/// jsonノードが指定型で読み込み可能かチェックする
		/// </summary>
		bool CanDeserialize(const nlohmann::json& node, nlohmann::json::value_t expectedType);

		/// <summary>
		/// ファイルの存在を確認する
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		bool FileExists(const std::string& filePath);

		/// <summary>
		/// ディレクトリの存在を確認する
		/// </summary>
		/// <param name="directoryPath"></param>
		/// <returns></returns>
		bool DirectoryExists(const std::string& directoryPath);
	};
}
