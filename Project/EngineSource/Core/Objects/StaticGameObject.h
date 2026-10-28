#pragma once
#include "Geometry.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Collider.h"

namespace GameEngine {

	/// <summary>
	/// マップ配置用の静的オブジェクト
	/// </summary>
	class StaticGameObject : public IGameObject {
	public:
		StaticGameObject(std::string name, std::string modelName, Model* model);
		~StaticGameObject() = default;

		//void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

	public:
		// 名前を取得
		const std::string& GetName() const { return name_; }
		// 使用しているモデル名を取得
		const std::string& GetModelName() const { return modelName_; }

		// ワールド行列を取得
		WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

		// オブジェクト選択用のAABBの当たり判定
		AABB GetSelectObjectAABB() const {
			AABB aabb;

			aabb.min = (modelComponent_.worldTransform_.transform_.translate - 2.0f) * modelComponent_.worldTransform_.transform_.scale;
			aabb.max = (modelComponent_.worldTransform_.transform_.translate + 2.0f) * modelComponent_.worldTransform_.transform_.scale;

			return aabb;
		}

		void SetActive(bool active) override {
			isActive_ = active;

			if (isActive_) {
				debugParame_->RegisterWorld("World", modelComponent_.worldTransform_);
			} else {
				debugParame_->RemoveGroup();
			}
		}

	private:
		ModelComponent modelComponent_;

		// パラメータ機能
		std::unique_ptr<GameEngine::DebugParameter> debugParame_;

		// 当たり判定
		std::unique_ptr<Collider> collider_;

		bool isActiveCollider_ = false;
		bool makedCollider_ = false;
		
		ColliderShapeData shapeData_;
		ShapeType currentShapeType_ = ShapeType::kSphere;

		std::string name_ = "None";
		// モデル名
		std::string modelName_ = "None";

	private:

		void SetCollider();
	};
}
