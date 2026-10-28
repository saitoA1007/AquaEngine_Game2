#include "pch.h"
#define NOMINMAX
#include "DestructibleObject.h"
#include "FPSCounter.h"
#include "MyMath.h"
using namespace GameEngine;

DestructibleObject::DestructibleObject(std::string name, Model* model, uint32_t colliderId, uint32_t colliderAttribute) {
	name_ = name;
	model_ = model;
	colliderId_ = colliderId;
	colliderAttribute_ = colliderAttribute;

	// 当たり判定の設定
	collider_.SetCollisionAttribute(colliderAttribute_);
	collider_.SetCollisionMask(~colliderAttribute_);
	collider_.SetSize(colliderSize_);
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
	collider_.SetAnchorPoint(Vector3(0.5f, 0.5f, 0.5f));

	UserData userData;
	userData.typeID = colliderId_;
	userData.object = this;
	collider_.SetUserData(userData);

	// コールバック関数を登録
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
		OnCollisionEnter(result);
		});

	// 氷のマテリアルを設定する
	for (auto& [groupName, chunks] : model_->GetFractureChunks()) {
		PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName).get();
		buffer->SetBufferMaterial(iceMaterial_.GetMaterialSrvIndex(), static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));
	}

	// 破砕状態を初期化
	damageController_.Initialize(model_);
	damageController_.SetWorldMatrix(worldTransform_.GetWorldMatrix());
	// 静的なチャンクをまとめたメッシュにも氷のマテリアルを設定する
	damageController_.SetStaticMeshMaterial(iceMaterial_.GetMaterialSrvIndex(), static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));

	std::string subGroup = "";

	// パラメータ機能
	debugParameter_ = std::make_unique<DebugParameter>(name_);
	subGroup = "State";
	debugParameter_->Register("ColliderSize", colliderSize_, 0, subGroup);
	debugParameter_->Register("DamageAmount", damageAmount_, 0, subGroup);
	debugParameter_->Register("CraterRadius", craterRadius_, 0, subGroup);
	debugParameter_->Register("PlaneCount", planeCount_, 0, subGroup);

	debugParameter_->Register("BreakThreshold", damageController_.kBreakThreshold_, 0, subGroup);
	debugParameter_->Register("BreakDetachRadius", damageController_.kBreakDetachRadius_, 0, subGroup);
	debugParameter_->Register("MaxCrackOffset", damageController_.kMaxCrackOffset_, 0, subGroup);
	debugParameter_->Register("MaxCrackRotate", damageController_.kMaxCrackRotate_, 0, subGroup);
	debugParameter_->Register("NeighborCrackFactor", damageController_.kNeighborCrackFactor_, 0, subGroup);

	debugParameter_->Register("DentInwardBiasRatio", damageController_.kDentInwardBiasRatio_, 0, subGroup);
	debugParameter_->Register("MinDentRatio", damageController_.kMinDentRatio_, 0, subGroup);
	debugParameter_->Register("MaxDentRadiusToChunkRatio", damageController_.kMaxDentRadiusToChunkRatio_, 0, subGroup);
	// バネ物理
	subGroup = "Physics";
	debugParameter_->Register("CrackSpringStiffness", damageController_.kCrackSpringStiffness_, 0, subGroup);
	debugParameter_->Register("CrackDamping", damageController_.kCrackDamping_, 0, subGroup);
	debugParameter_->Register("CrackAngularSpringStiffness", damageController_.kCrackAngularSpringStiffness_, 0, subGroup);
	debugParameter_->Register("CrackAngularDamping", damageController_.kCrackAngularDamping_, 0, subGroup);
	debugParameter_->Register("CrackImpulseStrength", damageController_.kCrackImpulseStrength_, 0, subGroup);

	debugParameter_->Apply();
}

void DestructibleObject::Initialize() {
	debugParameter_->Apply();
}

void DestructibleObject::Update() {

	debugParameter_->ApplyIfDirty();

	// 更新
	worldTransform_.UpdateTransformMatrix();
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
	collider_.SetSize(colliderSize_);

	// マイクロ破片・凹んだチャンクのラスタライズ描画で使うワールド行列を渡す
	damageController_.SetWorldMatrix(worldTransform_.GetWorldMatrix());
	damageController_.Update(FpsCounter::gameDeltaTime);
}

void DestructibleObject::Draw() {

	FractureBreakState& breakState = damageController_.GetBreakState();

	// 静的なチャンクは内部の切断面を除いた1つのメッシュとしてレイトレで描画
	damageController_.FlushStaticMesh();
	renderQueue_->SubmitRaytracingFractureStaticMesh(damageController_.GetStaticMesh(), worldTransform_);

	// ひびで揺れているチャンクだけ、事前分割された破片のまま個別にレイトレで描画
	if (breakState.HasIntact()) {
		renderQueue_->SubmitRaytracingFracture(model_, breakState.Intact(), worldTransform_);
	}
	// マクロ破片は破壊イベントごとに積み上がったバッチ全てを描画する
	for (auto& batch : breakState.MacroDebrisBatches()) {
		renderQueue_->SubmitRaytracingFracture(model_, *batch, worldTransform_);
	}

	// ランタイムでカットされた破片はラスタライズで描画
	if (breakState.HasMicroDebris() || breakState.HasDentedChunks()) {
		const auto& fractureChunks = model_->GetFractureChunks();
		if (!fractureChunks.empty()) {
			const auto& chunks = fractureChunks.begin()->second;
			Material* drawMaterial = model_->GetMaterial(chunks.front().materialName);

			// 積み上がったマイクロ破片バッチ全てを描画する
			for (auto& batch : breakState.MicroDebrisBatches()) {
				//renderQueue_->SubmitRuntimeCutFragments(*batch, &drawMaterial->GetMaterialBuffer());
				renderQueue_->SubmitRuntimeCutIceFragments(*batch, &iceMaterial_.GetMaterialBuffer());
			}

			// 付着したまま動的に凹んでいるチャンクを描画する
			for (auto& [chunkId, instance] : breakState.DentedChunks()) {
				//renderQueue_->SubmitRuntimeCutFragments(*instance, &drawMaterial->GetMaterialBuffer());
				renderQueue_->SubmitRuntimeCutIceFragments(*instance, &iceMaterial_.GetMaterialBuffer());
			}
		}
	}
}

void DestructibleObject::OnCollisionEnter(const GameEngine::CollisionResult& result) {
	// ワールド座標で渡ってくる衝突情報をローカル座標へ変換してから渡す
	Matrix4x4 inverseWorld = Matrix4x4::Inverse(worldTransform_.GetWorldMatrix());
	Vector3 localImpactPos = Matrix4x4::Transform(result.contactPosition, inverseWorld);
	Vector3 localImpactDirection = Matrix4x4::TransformNormal(result.contactNormal, inverseWorld);

	damageController_.ApplyChipDamage(localImpactPos, damageAmount_, craterRadius_, planeCount_,
		localImpactDirection, result.penetrationDepth);
}
