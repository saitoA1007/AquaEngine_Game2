#include "pch.h"
#include "Animator.h"
#include <cassert>
#include <algorithm>
#include "MyMath.h"
#include "EasingManager.h"
#include "FPSCounter.h"
#include "Model.h"
#include "PSOManager.h"
#include "DebugRenderer.h"
using namespace GameEngine;

ID3D12GraphicsCommandList4* Animator::commandList_ = nullptr;
ID3D12RootSignature* Animator::rootSignature_ = nullptr;
ID3D12PipelineState* Animator::pipelineState_ = nullptr;

void Animator::StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager) {
	commandList_ = commandList;

	auto psoData = psoManager->GetDrawPsoData("ComputeAnimation");
	pipelineState_ = psoData.graphicsPipelineState;
	rootSignature_ = psoData.rootSignature;
}

void Animator::Initialize(Model* model, const AnimationData* animationData) {
	SetAnimationData(animationData);
	SetModelData(model);
}

void Animator::Update() {

	timer_ += FpsCounter::deltaTime;

	if (isLoop_) {
		timer_ = std::fmodf(timer_, animationData_->duration);
	} else {
		timer_ = (std::min)(timer_, animationData_->duration);
	}

	// アニメーションの更新処理
	Update(timer_);
}

void Animator::Update(const float& time) {

	if (model_->IsSkeleton()) {
		// アニメーションの更新をおこない、骨ごとのLocal情報を更新する
		ApplyAnimation(*skeleton_, *animationData_, time);

		// 現在の骨ごとのLocal情報を基にSkeletonSpaceの情報を更新する
		SkeletonUpdate(*skeleton_);

		// SkeletonSpaceの情報を基に、SkinClusterのMatrixPaletteを更新する
		SkinClusterUpdate(*skinCluster_, *skeleton_);
	} else {
		Node& rootNode = model_->GetNodes();

		// 全ノードにアニメーション値をサンプリング
		ApplyNodeAnimation(rootNode, *animationData_, time);

		// ノードを更新
		NodeHierarchyUpdate(model_);
	}
}

void Animator::ComputeUpdate() {
	timer_ += FpsCounter::deltaTime;

	if (isLoop_) {
		timer_ = std::fmodf(timer_, animationData_->duration);
	} else {
		timer_ = (std::min)(timer_, animationData_->duration);
	}

	// コンピュートシェーダーを使用したアニメーションの更新
	ComputeUpdate(timer_);
}

void Animator::ComputeUpdate(const float& time) {
	// アニメーションの更新処理
	Update(time);

	// コンピュートシェーダーで頂点を更新
	UpdateCompute();
}

Vector3 Animator::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {

	assert(!keyframes.empty()); // キーがないものはエラーを返す

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float denom = keyframes[nextIndex].time - keyframes[index].time;
			float t = (denom == 0.0f) ? 0.0f : ((time - keyframes[index].time) / denom);
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	// 一番後の時刻より後ろなので最後の値を返す
	return (*keyframes.rbegin()).value;
}

Quaternion Animator::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {

	assert(!keyframes.empty()); // キーがないものはエラーを返す

	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;

		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float denom = keyframes[nextIndex].time - keyframes[index].time;
			float t = (denom == 0.0f) ? 0.0f : ((time - keyframes[index].time) / denom);
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	// 一番後の時刻より後ろなので最後の値を返す
	return (*keyframes.rbegin()).value;
}

void Animator::ApplyNodeAnimation(Node& node, const AnimationData& animation, float animationTime) {
	
	if (auto it = animation.nodeAnimations.find(node.name); it != animation.nodeAnimations.end()) {
		const NodeAnimation& rootNodeAnimation = (*it).second;
		// ノードのトランスフォームを更新
		node.transform.translate = CalculateValue(rootNodeAnimation.translate, animationTime);
		node.transform.rotate = CalculateValue(rootNodeAnimation.rotate, animationTime);
		node.transform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
	}

	// 子ノードも更新
	for (auto& child : node.children) {
		ApplyNodeAnimation(child, animation, animationTime);
	}
}

void Animator::NodeHierarchyUpdate(Model* model) {
	auto& rootNode = model->GetNodes();

	// 親行列
	rootNode.localMatrix = Matrix4x4::MakeAffineMatrix(rootNode.transform.scale, rootNode.transform.rotate, rootNode.transform.translate);

	// 子ノードに対して再帰的に行列計算を行う
	for (auto& child : rootNode.children) {
		NodeHierarchyUpdate(child, rootNode.localMatrix);
	}
}

void Animator::NodeHierarchyUpdate(Node& node, const Matrix4x4& parentMatrix) {
	// 自身のローカル行列を計算
	Matrix4x4 local = Matrix4x4::MakeAffineMatrix(node.transform.scale, node.transform.rotate, node.transform.translate);

	// 親の行列を掛け合わせて、自身のグローバルな累積行列を計算
	node.localMatrix = local * parentMatrix;

	// さらにその子ノードへ、計算した独自の localMatrix を親の行列として伝播させる
	for (auto& child : node.children) {
		NodeHierarchyUpdate(child, node.localMatrix);
	}
}

void Animator::ApplyAnimation(SkeletonData& skeleton, const AnimationData& animation, float animationTime) {
	for (Joint& joint : skeleton.joints) {
		// 対象のJointのAnimationがあれば、値の適応を行う。
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}
}

void Animator::SkeletonUpdate(SkeletonData& skeleton) {
	// すべてのJointを更新。
	for (Joint& joint : skeleton.joints) {
		joint.localMatrix = Matrix4x4::MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Animator::SkinClusterUpdate(SkinCluster& skinCluster, const SkeletonData& skeleton) {
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = Matrix4x4::InverseTranspose(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix);
	}
}

void Animator::SetModelData(Model* model) {
	model_ = model;
	if (model->IsSkeleton()) {
		auto* skeleton = model->GetSkeleton();
		skinCluster_ = skeleton->GetSkinCluster();
		skeleton_ = skeleton->GetSkeletonData();
	} else {
		skinCluster_ = nullptr;
		skeleton_ = nullptr;
	}
}

void Animator::UpdateCompute() {

	commandList_->SetComputeRootSignature(rootSignature_);
	commandList_->SetPipelineState(pipelineState_);

	auto* skeleton = model_->GetSkeleton();
	const auto& meshes = model_->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		auto* outputBuffer = skeleton->GetOutputVertexBuffer(i);
		outputBuffer->TransitionUAV(commandList_);

		commandList_->SetComputeRootDescriptorTable(0, skinCluster_->wellBuffer.GetSrvGpuHandle()); // 共通パレット
		commandList_->SetComputeRootDescriptorTable(1, meshes[i]->GetVertexBuffer().GetSrvGpuHandle());
		commandList_->SetComputeRootDescriptorTable(2, skeleton->GetInfluenceBuffer(i)->GetSrvGpuHandle()); // メッシュ固有
		commandList_->SetComputeRootDescriptorTable(3, outputBuffer->GetUAVGpuHandle());
		commandList_->SetComputeRootConstantBufferView(4, skeleton->GetConstantBuffer(i)->GetGpuVirtualAddress());

		commandList_->Dispatch(UINT(skeleton->GetVerticesNum(i) + 1023) / 1024, 1, 1);

		outputBuffer->TransitionSRV(commandList_);
		meshes[i]->GetBLAS()->Update(commandList_, outputBuffer->GetView());
	}
}

void Animator::DebugDraw(DebugRenderer* debugRenderer, float sphereRadius, const Vector4& color) {
	if (!model_->IsSkeleton()) {
		return;
	}

	for (const auto& joint : skeleton_->joints) {
		// 行列から現在の関節のワールド空間位置を取得
		Vector3 jointPos = {
			joint.skeletonSpaceMatrix.m[3][0],
			joint.skeletonSpaceMatrix.m[3][1],
			joint.skeletonSpaceMatrix.m[3][2]
		};

		// 関節を球としてデバッグ描画に登録
		Sphere jointSphere = { jointPos, sphereRadius };
		debugRenderer->AddSphere(jointSphere, color);

		// 親の関節が存在する場合、親と自分を繋ぐ線を引く
		if (joint.parent) {
			// 親のJoint情報を取得
			const auto& parentJoint = skeleton_->joints[*joint.parent];

			// 親の位置を抽出
			Vector3 parentPos = {
				parentJoint.skeletonSpaceMatrix.m[3][0],
				parentJoint.skeletonSpaceMatrix.m[3][1],
				parentJoint.skeletonSpaceMatrix.m[3][2]
			};

			// 自分と親を繋ぐ線を描画に登録
			debugRenderer->AddLine(jointPos, parentPos, color);
		}
	}
}

Vector3 Animator::GetJointWorldPosition(const std::string& jointName, const Matrix4x4& worldMatrix) const {
	if (!skeleton_) {
		return Vector3(0.0f,0.0f,0.0f);
	}

	auto it = skeleton_->jointMap.find(jointName);
	if (it == skeleton_->jointMap.end()) {
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	const Joint& joint = skeleton_->joints[it->second];
	Matrix4x4 world = joint.skeletonSpaceMatrix * worldMatrix;

	return Vector3{ world.m[3][0], world.m[3][1], world.m[3][2] };
}

Vector3 Animator::GetNodeWorldPosition(const std::string& nodeName, const Matrix4x4& worldMatrix) const {
	if (!model_) {
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	const Node* target = FindNode(model_->GetNodes(), nodeName);
	if (!target) {
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	Matrix4x4 world = target->localMatrix * worldMatrix;
	return Vector3{ world.m[3][0], world.m[3][1], world.m[3][2] };
}

const Node* Animator::FindNode(const Node& node, const std::string& name) {
	if (node.name == name) {
		return &node;
	}
	for (const auto& child : node.children) {
		if (const Node* found = FindNode(child, name)) {
			return found;
		}
	}
	return nullptr;
}