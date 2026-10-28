#pragma once
#include <unordered_map>
#include "GameObjectManager.h"
#include "ModelManager.h"
#include "StaticGameObject.h"
#include "JsonSerializer.h"
namespace GameEngine {

	class StaticGameObjectManager {
	public:
		StaticGameObjectManager() = default;
		~StaticGameObjectManager() = default;

		void Initialize(GameObjectManager* objectManager, ModelManager* modelManager);

	public:
		
		// 追加
		uint32_t AddObject(std::string objecctName, std::string modelName);

		// 論理削除
		void ReleaseObject(uint32_t id);

		// 論理削除したオブジェクトを再アクティブ化する
		void RestoreObject(uint32_t id);

		// オブジェクトを取得
		StaticGameObject* GetStaticObject(uint32_t id);

		// マウスの位置から選択されるオブジェクトを取得
		int32_t SelectObject(Vector2 mousePos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition,float width, float height);

		// ロードする
		void LoadSceneObject(const std::string& sceneName, bool isClear = true);

		// 保存する
		void SaveSceneObject(const std::string& sceneName);

		// クリアする
		void Clear();

	private:
		// オブジェクト管理
		GameObjectManager* objectManager_ = nullptr;
		// モデル管理
		ModelManager* modelManager_ = nullptr;

		// ディレクトリ
		const std::string kDirectoryPath_ = "Resources/Json/GameData/StaticObjectData/";
		std::string filePath_ = "StaticObjectData.json";

		// オブジェクト
		std::unordered_map<uint32_t, StaticGameObject*> objects_;

		uint32_t currentIndex_ = 0;
	};
}
