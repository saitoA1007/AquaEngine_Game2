#include "pch.h"
#include "StaticGameObjectManager.h"
#include "CollisionUtils.h"
#include "MyMath.h"
using namespace GameEngine;

void StaticGameObjectManager::Initialize(GameObjectManager* objectManager, ModelManager* modelManager) {
	objectManager_ = objectManager;
	modelManager_ = modelManager;

}

uint32_t StaticGameObjectManager::AddObject(std::string objecctName, std::string modelName) {

	// モデルを取得
	auto* model = modelManager_->GetNameByModel(modelName);

	// オブジェクトを追加
	auto* object = objectManager_->AddObject<StaticGameObject>(objecctName, modelName, model);

	// idを取得
	uint32_t id = currentIndex_++;
	
	// オブジェクトを追加
	objects_[id] = object;

	return id;
}

void StaticGameObjectManager::ReleaseObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	StaticGameObject* object = it->second;
	// オブジェクトの無効化
	object->SetActive(false);
}

void StaticGameObjectManager::RestoreObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	StaticGameObject* object = it->second;
	// オブジェクトの有効化
	object->SetActive(true);
}

StaticGameObject* StaticGameObjectManager::GetStaticObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	if (!it->second->IsActive()) {
		// 論理削除済みのオブジェクトはnullを返す
		return nullptr;
	}
	StaticGameObject* object = it->second;
	return object;
}

int32_t StaticGameObjectManager::SelectObject(Vector2 mousePos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition, float width, float height) {

	Vector3 rayOrigin = cameraPosition;
	Vector3 rayDirection = Math::CalculateRayDirection(mousePos, viewMatrix, projectionMatrix, width, height);
	float rayLength = 1000.0f;
	Vector3 rayDiff = rayDirection * rayLength;

	int32_t selectedId = -1;
	float minDistance = FLT_MAX;

	for (auto [id, object] : objects_) {
		// 論理削除済みのオブジェクトは飛ばす
		if (!object->IsActive()) {
			continue; 
		}

		AABB aabb = object->GetSelectObjectAABB();
		Segment segment = Segment(rayOrigin, rayDiff);

		float distance = 0.0f;
		// レイとAABBの交差判定関数
		CollisionResult result = IsAABBSegmentCollision(aabb, segment);

		if (result.isHit) {
			// 離れている距離を取得
			Vector3 diff = rayOrigin - result.contactPosition;
			distance = diff.Length();

			// 距離が近ければidを設定
			if (distance < minDistance) {
				minDistance = distance;
				selectedId = id;
			}
		}
	}
	return selectedId;
}

void StaticGameObjectManager::LoadSceneObject(const std::string& sceneName, bool isClear) {
	std::string path = kDirectoryPath_ + sceneName + filePath_;

	// ファイルがなければ早期リターン
	if (!JsonSerializer::FileExists(path)) {
		return;
	}

	// JSONファイルを読み込む
	nlohmann::json root = JsonSerializer::LoadFromFile(path);

	if (!root.is_array()) {
		return;
	}
	
	// オブジェクトをクリア
	if (isClear) {
		Clear();
	}

	// オブジェクトを復元
	for (const auto& objectJson : root) {
		std::string name = objectJson["name"];
		std::string modelName = objectJson["modelName"];

		// オブジェクトをシーンに再生成
		uint32_t id = AddObject(name, modelName);
		StaticGameObject* object = GetStaticObject(id);

		if (object) {
			auto& transform = object->GetWorldTransform().transform_;

			// トランスフォームの復元
			if (objectJson.contains("transform")) {
				const auto& tJson = objectJson["transform"];

				if (tJson.contains("translate")) {
					transform.translate = { tJson["translate"][0], tJson["translate"][1], tJson["translate"][2] };
				}
				if (tJson.contains("rotate")) {
					transform.rotate = { tJson["rotate"][0],tJson["rotate"][1],tJson["rotate"][2] };
				}
				if (tJson.contains("scale")) {
					transform.scale = { tJson["scale"][0], tJson["scale"][1], tJson["scale"][2] };
				}

				// 行列を更新
				object->GetWorldTransform().UpdateTransformMatrix();
			}
		}
	}
}

void StaticGameObjectManager::SaveSceneObject(const std::string& sceneName) {
	nlohmann::json root = nlohmann::json::array();

	for (auto [id, object] : objects_) {

		if (!object || !object->IsActive()) {
			continue;
		}

		nlohmann::json objectJson;
		objectJson["name"] = object->GetName();
		objectJson["modelName"] = object->GetModelName();

		// トランスフォーム情報をJSONに格納
		const auto& transform = object->GetWorldTransform().transform_;

		objectJson["transform"]["translate"] = { transform.translate.x, transform.translate.y, transform.translate.z };
		objectJson["transform"]["rotate"] = { transform.rotate.x, transform.rotate.y, transform.rotate.z };
		objectJson["transform"]["scale"] = { transform.scale.x, transform.scale.y, transform.scale.z };

		// 配列に追加
		root.push_back(objectJson);
	}

	// ファイルに保存
	std::string path = kDirectoryPath_ + sceneName + filePath_;
	JsonSerializer::SaveToFile(path, root);
}

void StaticGameObjectManager::Clear() {
	for (auto& object : objects_) {
		object.second->Destroy();
	}
	objects_.clear();
	currentIndex_ = 0;
}