#pragma once
#include "IEditorCommand.h"
#include "StaticGameObjectManager.h"
#include "Transform.h"

namespace GameEngine {

	// ============================================================
	// オブジェクト生成コマンド
	// ============================================================
	class AddObjectCommand : public IEditorCommand {
	public:
		AddObjectCommand(StaticGameObjectManager* manager, std::string objectName, std::string modelName)
			: manager_(manager), objectName_(std::move(objectName)), modelName_(std::move(modelName)) {}

		void Execute() override {
			if (!hasCreated_) {
				// 新規にオブジェクトを生成
				objectId_ = manager_->AddObject(objectName_, modelName_);
				hasCreated_ = true;
			} else {
				// すでに生成済みのデータを再アクティブ化する
				manager_->RestoreObject(objectId_);
			}
		}

		void Undo() override {
			manager_->ReleaseObject(objectId_);
		}

		uint32_t GetObjectId() const { return objectId_; }

	private:
		StaticGameObjectManager* manager_;
		std::string objectName_;
		std::string modelName_;
		uint32_t objectId_ = 0;
		bool hasCreated_ = false;
	};

	// ============================================================
	// オブジェクト削除コマンド
	// ============================================================
	class DeleteObjectCommand : public IEditorCommand {
	public:
		DeleteObjectCommand(StaticGameObjectManager* manager, uint32_t objectId)
			: manager_(manager), objectId_(objectId) {}

		void Execute() override {
			manager_->ReleaseObject(objectId_);
		}

		void Undo() override {
			manager_->RestoreObject(objectId_);
		}

	private:
		StaticGameObjectManager* manager_;
		uint32_t objectId_;
	};

	// ============================================================
	// オブジェクトの移動、回転、拡縮コマンド
	// ============================================================
	class TransformObjectCommand : public IEditorCommand {
	public:
		TransformObjectCommand(StaticGameObjectManager* manager, uint32_t objectId, const Transform& before, const Transform& after)
			: manager_(manager), objectId_(objectId), before_(before), after_(after) {}

		void Execute() override { Apply(after_); }
		void Undo() override { Apply(before_); }

	private:
		StaticGameObjectManager* manager_;
		uint32_t objectId_;
		Transform before_;
		Transform after_;

	private:
		void Apply(const Transform& snapshot) {
			StaticGameObject* obj = manager_->GetStaticObject(objectId_);
			if (obj == nullptr) {
				return;
			}
			WorldTransform& world = obj->GetWorldTransform();
			world.transform_.translate = snapshot.translate;
			world.transform_.rotate = snapshot.rotate;
			world.transform_.scale = snapshot.scale;
			world.UpdateTransformMatrix();
		}
	};
}