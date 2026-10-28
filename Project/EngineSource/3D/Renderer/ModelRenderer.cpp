#include "pch.h"
#include "ModelRenderer.h"
#include <cassert>

using namespace GameEngine;

ID3D12GraphicsCommandList* ModelRenderer::commandList_ = nullptr;
ID3D12Resource* ModelRenderer::cameraResource_ = nullptr;
SrvManager* ModelRenderer::srvManager_ = nullptr;

void ModelRenderer::StaticInitialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
	commandList_ = commandList;
	srvManager_ = srvManager;
}

void ModelRenderer::SetCamera(ID3D12Resource* cameraResource) {
	cameraResource_ = cameraResource;
}

void ModelRenderer::Draw(const Model* model, WorldTransform& worldTransform, const GpuResource* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetGpuVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::Draw(const Model* model, WorldTransform& worldTransform, GpuResource* lightGroupResource, const GpuResource* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetGpuVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGpuVirtualAddress());
		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const GpuResource* material) {

	// 描画するのが0以下の場合は早期リターン
	if (numInstance <= 0) { return; }

	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransforms.SetWVPMatrix(numInstance, model->GetLocalMatrix());
	} else {
		worldTransforms.SetWVPMatrix(numInstance);
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}

		commandList_->SetGraphicsRootDescriptorTable(1, worldTransforms.GetInstancingSrvGPU());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), numInstance, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), numInstance, 0, 0);
		}
	}
}

void ModelRenderer::DrawWboitInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const GpuResource* wboit, const GpuResource* material) {
	// 描画するのが0以下の場合は早期リターン
	if (numInstance <= 0) { return; }

	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransforms.SetWVPMatrix(numInstance, model->GetLocalMatrix());
	} else {
		worldTransforms.SetWVPMatrix(numInstance);
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList_->SetGraphicsRootConstantBufferView(0, cameraResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(1, worldTransforms.GetInstancingSrvGPU());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, wboit->GetGpuVirtualAddress());

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(4, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(4, material->GetGpuVirtualAddress());
		}
		
		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), numInstance, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), numInstance, 0, 0);
		}
	}
}

void ModelRenderer::DrawParticleCS(const Model* model, const uint32_t& numInstance, const SrvResource* particle, const GpuResource* camera) {
	// 描画するのが0以下の場合は早期リターン
	if (numInstance <= 0) { return; }

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList_->SetGraphicsRootDescriptorTable(0, particle->GetSrvGpuHandle());
		commandList_->SetGraphicsRootConstantBufferView(1, camera->GetGpuVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());

		// マテリアルを設定
		//const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
		//commandList_->SetGraphicsRootConstantBufferView(4, drawMaterial->GetGpuVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), numInstance, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), numInstance, 0, 0);
		}
	}
}

void ModelRenderer::DrawFracture(const Model* model, FractureInstance& fractureInstance, ID3D12CommandSignature* sig, GpuResource* lightGroupResource, const GpuResource* material) {

	for (auto& [groupName, chunks] : model->GetFractureChunks()) {
		PackedGeometryBuffer* buffer = model->GetFractureBuffers().at(groupName).get();

		auto vbView = buffer->GetVertexBufferView();
		auto ibView = buffer->GetIndexBufferView();
		commandList_->IASetVertexBuffers(0, 1, &vbView);
		commandList_->IASetIndexBuffer(&ibView);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(chunks.begin()->materialName);
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}

		commandList_->SetGraphicsRootDescriptorTable(1, fractureInstance.GetInstancingSrvGPU());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGpuVirtualAddress());

		commandList_->ExecuteIndirect(
			sig, // コマンドシグネチャ
			fractureInstance.GetNumInstance(),
			fractureInstance.GetArgumentBuffer().GetResource(),
			0, nullptr, 0
		);
	}
}

void ModelRenderer::DrawRuntimeCutFragments(FractureInstance& fractureInstance, ID3D12CommandSignature* sig, GpuResource* lightGroupResource, const GpuResource* material) {

	assert(fractureInstance.HasRuntimeGeometry() && "ランタイムカットで構築されたFractureInstanceではありません");

	commandList_->IASetVertexBuffers(0, 1, &fractureInstance.GetRuntimeVertexBufferView());
	commandList_->IASetIndexBuffer(&fractureInstance.GetRuntimeIndexBufferView());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(1, fractureInstance.GetInstancingSrvGPU());
	commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGpuVirtualAddress());

	commandList_->ExecuteIndirect(
		sig, fractureInstance.GetNumInstance(),
		fractureInstance.GetArgumentBuffer().GetResource(), 0, nullptr, 0);
}

void ModelRenderer::DrawAnimation(const Model* model, WorldTransform& worldTransform, GpuResource* lightGroupResource, const GpuResource* material) {
	
	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();
	const auto* skeleton = model->GetSkeleton();

	for (uint32_t i = 0; i < meshes.size(); ++i) {

		D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
			meshes[i]->GetVertexBufferView(),
			skeleton->GetInfluenceBufferData(i)->GetView()
		};

		commandList_->IASetVertexBuffers(0, 2, vbvs);
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetGpuVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());

		commandList_->SetGraphicsRootDescriptorTable(3, skeleton->GetSkinClusterData()->wellBuffer.GetSrvGpuHandle());
		commandList_->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());

		commandList_->SetGraphicsRootConstantBufferView(5, lightGroupResource->GetGpuVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(6, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootDescriptorTable(7, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawGrid(const Model* model, WorldTransform& worldTransform) {

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	commandList_->IASetVertexBuffers(0, 1, &meshes[0]->GetVertexBufferView());
	commandList_->IASetIndexBuffer(&meshes[0]->GetIndexBufferView());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, worldTransform.GetGpuVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, cameraResource_->GetGPUVirtualAddress());
	commandList_->DrawIndexedInstanced(meshes[0]->GetTotalIndices(), 1, 0, 0, 0);
}

void ModelRenderer::DrawDebugLine(const D3D12_VERTEX_BUFFER_VIEW& vertexView, const uint32_t& totalVertices) {
	if (totalVertices < 2) { return; }
	// 頂点バッファを設定
	commandList_->IASetVertexBuffers(0, 1, &vertexView);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, cameraResource_->GetGPUVirtualAddress());
	commandList_->DrawInstanced(totalVertices, 1, 0, 0);
}

void ModelRenderer::DrawSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルが設定されていなければデフォルトのマテリアルを使う
		if (material == nullptr) {
			// マテリアルを設定
			const Material* drawMaterial = model->GetMaterial(meshes[i]->GetMaterialName());
			commandList_->SetGraphicsRootConstantBufferView(0, drawMaterial->GetGpuVirtualAddress());
		} else {
			commandList_->SetGraphicsRootConstantBufferView(0, material->GetGpuVirtualAddress());
		}
		commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
		commandList_->SetGraphicsRootConstantBufferView(1, worldTransform.GetGpuVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawShadowMap(const Model* model, WorldTransform& worldTransform) {
	// カメラ座標に変換
	if (model->IsLoad()) {
		worldTransform.SetWVPMatrix(model->GetLocalMatrix());
	}

	// メッシュを取得
	const std::vector<std::unique_ptr<Mesh>>& meshes = model->GetMeshes();

	for (uint32_t i = 0; i < meshes.size(); ++i) {
		commandList_->IASetVertexBuffers(0, 1, &meshes[i]->GetVertexBufferView());
		commandList_->IASetIndexBuffer(&meshes[i]->GetIndexBufferView());
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList_->SetGraphicsRootConstantBufferView(0, worldTransform.GetGpuVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(1, cameraResource_->GetGPUVirtualAddress());

		if (meshes[i]->GetTotalIndices() != 0) {
			commandList_->DrawIndexedInstanced(meshes[i]->GetTotalIndices(), 1, 0, 0, 0);
		} else {
			commandList_->DrawInstanced(meshes[i]->GetTotalVertices(), 1, 0, 0);
		}
	}
}

void ModelRenderer::DrawLight(GpuResource* lightGroupResource) {
	commandList_->SetGraphicsRootConstantBufferView(4, lightGroupResource->GetGpuVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(5, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	commandList_->SetGraphicsRootDescriptorTable(6, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
}

void ModelRenderer::DrawRaytracing(const Model* model, WorldTransform& worldTransform) {
	model;
	worldTransform;
	//auto blasList = model->GetBLASList();
	//
	//for (const auto& blas : blasList) {
	//
	//}
}