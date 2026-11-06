#include "pch.h"
#include "RaytracingPipeline.h"
#include "EngineSource/Graphics/PSO/Core/RootSignatureBuilder.h"
using namespace GameEngine;

void RaytracingPipeline::Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc) {
	device_ = device;
	srvManager_ = srvManager;

	// シェーダコンパイル機能を初期化
	rayLibShaderCompiler_.Initialize(dxc);

	// ルートシグネチャを作成する
	CreateGlobalRootsignature();

	// ステートオブジェクトを作成する
	CreateStateObject();

	// シェーダーテーブルを作成
	CreateShaderTable();
}

void RaytracingPipeline::CreateGlobalRootsignature() {

	// SRV/UAVはResourceDescriptorHeapから直接取得するため、ヒープのインデックスだけをルート定数で渡す
	const uint32_t indexCount = sizeof(RaytracingGlobalIndices) / sizeof(uint32_t);

	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.Add32BitConstantsParameter(indexCount, 2, D3D12_SHADER_VISIBILITY_ALL); // ヒープのインデックス
	builder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL); // camera
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL); // light
	builder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
	builder.CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED);
	rootSignatureGlobal_ = builder.MoveOwnerRootSignature();
}

void RaytracingPipeline::CreateStateObject() {
	LibraryResult raygenResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/RayGen.hlsl");
	LibraryResult missResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/Miss.hlsl");
	LibraryResult objectResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/chsObject.hlsl");
	LibraryResult iceObjectResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/ChsIceObject.hlsl");

	// 初期化処理
	stateObjectBuilder_.Initialize();

	// シェーダーを設定
	stateObjectBuilder_.AddDXILLibrary(raygenResult.blob.Get(), raygenResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(missResult.blob.Get(), missResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(objectResult.blob.Get(), objectResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(iceObjectResult.blob.Get(), iceObjectResult.exportNames);

	// ヒットグループを設定
	stateObjectBuilder_.AddHitGroup(AppHitGroups::DefaultModel, L"MainObjectCHS");
	stateObjectBuilder_.AddHitGroup(AppHitGroups::IceModel, L"MainIceObjectCHS");

	// シェーダー設定
	const uint32_t MaxPayloadSize = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float);
	const uint32_t MaxAttributeSize = sizeof(float) * 2;
	stateObjectBuilder_.SetShaderConfig(MaxPayloadSize, MaxAttributeSize);
	stateObjectBuilder_.SetPipelineConfig(4);

	// グローバルルートシグネチャを設定
	stateObjectBuilder_.SetGlobalRootSignature(rootSignatureGlobal_.Get());

	// 生成する
	stateObject_ = stateObjectBuilder_.Build(device_);
}

void RaytracingPipeline::CreateShaderTable() {

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
	stateObject_.As(&rtsoProps);

	// raygen
	{
		auto id = rtsoProps->GetShaderIdentifier(L"MainRayGen");
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}

		ShaderRecord record;
		auto table = record.SetIdentifier(id);
		shaderTableBuilder_.RayGen().AddRecord(std::move(record));
	}

	// miss
	{
		auto id = rtsoProps->GetShaderIdentifier(L"MainMiss");
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord record;
		record.SetIdentifier(id);
		shaderTableBuilder_.Miss().AddRecord(std::move(record));

		// シャドウ判定用のMissシェーダー
		auto shadowId = rtsoProps->GetShaderIdentifier(L"ShadowMiss");
		if (shadowId == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord shadowRecord;
		shadowRecord.SetIdentifier(shadowId);
		shaderTableBuilder_.Miss().AddRecord(std::move(shadowRecord));
	}
	
	// hitGroup
	{
		// デフォルト
		auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::DefaultModel.c_str());
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord record;
		auto& table = record.SetIdentifier(id);
		shaderTableBuilder_.HitGroup().AddRecord(std::move(record));

		// 氷
		auto iceId = rtsoProps->GetShaderIdentifier(AppHitGroups::IceModel.c_str());
		if (iceId == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord iceRecord;
		auto& iceTable = iceRecord.SetIdentifier(iceId);
		shaderTableBuilder_.HitGroup().AddRecord(std::move(iceRecord));
	}

	// テーブルを設定する
	shaderTableBuilder_.Build(device_);

	// 保存する
	dispatchRayDesc_ = shaderTableBuilder_.CreateDispatchRaysDesc(1280, 720);
}