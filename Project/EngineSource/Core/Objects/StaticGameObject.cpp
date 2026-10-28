#include "pch.h"
#include "StaticGameObject.h"

using namespace GameEngine;

StaticGameObject::StaticGameObject(std::string name, std::string modelName, Model* model) : modelComponent_(model) {
	name_ = name;
	modelName_ = modelName;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name_);
	debugParame_->RegisterWorld("World", modelComponent_.worldTransform_);
	debugParame_->Register("IsActiveCollider", isActiveCollider_, 1);
	std::string subGroup = "Material";
	auto* materialData = modelComponent_.materialData_;
	int index = 0;
	debugParame_->Register("Color", materialData->color, index++, subGroup);
	debugParame_->Register("SpecularColor", materialData->specularColor, index++, subGroup);
	debugParame_->Register("Metalic", materialData->metallic, index++, subGroup);
	debugParame_->Register("Roughness", materialData->roughness, index++, subGroup);
}

void StaticGameObject::Update() {

	debugParame_->ApplyIfDirty();

	// コライダー設定
	SetCollider();

	// 更新
	modelComponent_.Update();
}

void StaticGameObject::DebugUpdate() {
	// デバック状態でも更新をおこなう
	Update();
}

void StaticGameObject::Draw() {
	// 描画
	modelComponent_.DrawRaytracing(renderQueue_);
}

void StaticGameObject::SetCollider() {

	// コライダーを設定
	if (isActiveCollider_ && !makedCollider_) {
		std::string subGroup = "Collider";
		int index = 0;
		debugParame_->Register("ColliderData", shapeData_, index++, subGroup);

		// コライダーを生成
		shapeData_.type = ShapeType::kSphere;
		std::unique_ptr<SphereCollider> sphereCollider = std::make_unique<SphereCollider>();
		sphereCollider->SetRadius(shapeData_.radius);
		sphereCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());

		// 所有を移動
		collider_ = std::move(sphereCollider);

		makedCollider_ = true;
	}

	if (!makedCollider_) { return; }

	// 変更
	if (currentShapeType_ != shapeData_.type) {
		currentShapeType_ = shapeData_.type;
		// リセット
		collider_.reset();

		switch (currentShapeType_)
		{
		case ShapeType::kSphere: {
			std::unique_ptr<SphereCollider> sphereCollider = std::make_unique<SphereCollider>();
			sphereCollider->SetRadius(shapeData_.radius);
			sphereCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());

			// 所有を移動
			collider_ = std::move(sphereCollider);
			break;
		}

		case ShapeType::kAABB: {
			std::unique_ptr<AABBCollider> aabbCollider = std::make_unique<AABBCollider>();
			aabbCollider->SetSize(shapeData_.boxSize);
			aabbCollider->SetAnchorPoint(shapeData_.anchorPoint);
			aabbCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());

			// 所有を移動
			collider_ = std::move(aabbCollider);
			break;
		}

		case ShapeType::kOBB: {
			std::unique_ptr<OBBCollider> obbCollider = std::make_unique<OBBCollider>();
			obbCollider->SetSize(shapeData_.boxSize);
			obbCollider->SetAnchor(shapeData_.anchorPoint);
			obbCollider->UpdateOrientationsFromRotate(shapeData_.rotate);
			obbCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());

			// 所有を移動
			collider_ = std::move(obbCollider);
			break;
		}

		case ShapeType::kSegment:
			break;
		}
	}

	// 更新
	switch (currentShapeType_)
	{
	case ShapeType::kSphere: {
		SphereCollider* sphereCollider = dynamic_cast<SphereCollider*>(collider_.get());
		sphereCollider->SetRadius(shapeData_.radius);
		sphereCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());
		break;
	}

	case ShapeType::kAABB: {
		AABBCollider* aabbCollider = dynamic_cast<AABBCollider*>(collider_.get());
		aabbCollider->SetSize(shapeData_.boxSize);
		aabbCollider->SetAnchorPoint(shapeData_.anchorPoint);
		aabbCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());
		break;
	}

	case ShapeType::kOBB: {
		OBBCollider* obbCollider = dynamic_cast<OBBCollider*>(collider_.get());
		obbCollider->SetSize(shapeData_.boxSize);
		obbCollider->SetAnchor(shapeData_.anchorPoint);
		obbCollider->UpdateOrientationsFromRotate(shapeData_.rotate);
		obbCollider->SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition());
		break;
	}
	}
}