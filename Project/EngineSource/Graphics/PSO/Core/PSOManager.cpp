#include "pch.h"
#include "PSOManager.h"
#include "LogManager.h"
#include <cassert>
#include "FractureInstance.h"

using namespace GameEngine;

void PSOManager::Initialize(ID3D12Device* device, DXC* dxc) {
	LogManager::GetInstance().Log("Initialize PSOManager Start");
	device_ = device;
	dxc_ = dxc;
	// ラスタライザの全パターン生成
	rasterizerBuiler_.Initialize();
	// ブレンドモードの全パターン生成
	blendBuilder_.Initialize();
	// シェーダーコンパイラの初期唖k
	shaderCompiler_.Initialize(dxc);
	LogManager::GetInstance().Log("Initialize PSOManager End");
}

void PSOManager::RegisterPSO(const std::string& name, const CreatePSOData& psoData) {

	// 既に登録されていたら飛ばす
	if (psoList_.find(name) != psoList_.end()) {
		return;
	}

	CreatePSO(name,psoData);

	LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::RegisterPSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout) {

    // 既に登録されていたら飛ばす
    if (psoList_.find(name) != psoList_.end()) {
        return;
    }

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    LogManager::GetInstance().Log("Compiling pixel shader");
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::PS, psoData.psPath);

    if (!vsBlob || !psBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + name);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSignature->GetRootSignature();
        rootSignatureData.parameterTypes = rootSignature->GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // DepthStencilStateの設定
     // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    if (psoData.isDepthEnable) {
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = psoData.depthMask;
        depthStencilDesc.DepthFunc = psoData.depthFunc;
    } else {
        depthStencilDesc.DepthEnable = false;
    }

    // PSO設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    psoDesc.InputLayout = inputLayout->GetInputLayoutDesc();
    psoDesc.RasterizerState = rasterizerBuiler_.GetRasterizerDesc(psoData.drawMode);
    psoDesc.BlendState = blendBuilder_.CreateBlendDesc(psoData.blendMode);
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = psoData.numRenderTargets;
    for (uint32_t i = 0; i < psoData.numRenderTargets; ++i) {
        psoDesc.RTVFormats[i] = psoData.rtvFormats[i];
    }
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.pipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[name] = pso;

    LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::RegisterPSO(const MaterialGraph& graph, const std::wstring& materialName) {

    // シェーダーをコンパイル
    shaderCompiler_.CompileMaterialGraph(graph, materialName);

}

void PSOManager::RegisterComputePSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature) {

    // 既に登録されていたら飛ばす
    if (psoList_.find(name) != psoList_.end()) {
        return;
    }

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling compute shader");
    Microsoft::WRL::ComPtr<IDxcBlob> csBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::Cs, psoData.csPath);

    if (!csBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + name);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSignature->GetRootSignature();
        rootSignatureData.parameterTypes = rootSignature->GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // コンピュートパイプラインを設定
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{};
    computePipelineStateDesc.CS = {
        .pShaderBytecode = csBlob->GetBufferPointer(),
        .BytecodeLength = csBlob->GetBufferSize(),
    };
    computePipelineStateDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 生成
    HRESULT hr = device_->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&pso.pipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[name] = pso;

    LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::RegisterShadowMapPSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout) {
    // 既に登録されていたら飛ばす
    if (psoList_.find(name) != psoList_.end()) {
        return;
    }

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    if (!vsBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + name);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSignature->GetRootSignature();
        rootSignatureData.parameterTypes = rootSignature->GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = psoData.depthMask;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // かリングを設定
    D3D12_RASTERIZER_DESC rast{};
    rast.FillMode = D3D12_FILL_MODE_SOLID;
    rast.CullMode = D3D12_CULL_MODE_BACK;
    // シャドウアクネ対策
    rast.DepthBias = 1000;
    rast.SlopeScaledDepthBias = 1.0f;
    rast.DepthBiasClamp = 0.0f;

    // PSO設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    psoDesc.InputLayout = inputLayout->GetInputLayoutDesc();
    psoDesc.RasterizerState = rast;
    psoDesc.BlendState = blendBuilder_.GetBlendDesc(psoData.blendMode[0]);
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { nullptr,0 };
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 0;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.pipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[name] = pso;

    LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::LoadFromJson(const std::string& fileName) {

	// 読み込みJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + fileName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込み用に開く
	ifs.open(filePath);

	// ファイルオープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for load.";
		MessageBoxA(nullptr, message.c_str(), "PSOManager", 0);
		assert(0);
	}

    json root;

    // json文字列からjsonのデータ構造に展開
    ifs >> root;
    // ファイルを閉じる
    ifs.close();

	LogManager::GetInstance().Log("PSO registerd name : " + fileName);
}

void PSOManager::CreatePSO(const std::string& psoName, const CreatePSOData& psoData) {
	LogManager::GetInstance().Log("PSO create start : " + psoName);

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    LogManager::GetInstance().Log("Compiling pixel shader");
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::PS, psoData.psPath);

    if (!vsBlob || !psBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + psoName);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    // RootSignatureの生成
    RootSignatureBuilder rootSigBuilder;
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        rootSigBuilder.Initialize(device_);
        rootSigBuilder.CreateRootSignatureFromReflection(dxc_->GetIDxcUtils(), vsBlob.Get(), psBlob.Get());
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSigBuilder.GetRootSignature();
        rootSignatureData.parameterTypes = rootSigBuilder.GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // InputLayoutの生成
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateInputLayoutFromReflection(dxc_->GetIDxcUtils(), vsBlob.Get());
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = inputLayoutBuilder.GetInputLayoutDesc();

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    if (psoData.isDepthEnable) {
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    } else {
        depthStencilDesc.DepthEnable = false;
    }

    // PSOの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    // InputLayout
    psoDesc.InputLayout = inputLayoutDesc;
    // RootSignature
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    // シェーダー
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    // BlendState
    psoDesc.BlendState = blendBuilder_.GetBlendDesc(psoData.blendMode[0]);
    // RasterizerState
    psoDesc.RasterizerState = rasterizerBuiler_.GetRasterizerDesc(psoData.drawMode);
    // DepthStencilState
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 描画タイプ
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.pipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[psoName] = pso;

    LogManager::GetInstance().Log("PSO create end : " + psoName);
}

ID3D12RootSignature* PSOManager::GetRootSignature(const std::string& name) {
	auto it = rootSignatureList_.find(name);
	if (it != rootSignatureList_.end()) {
		return it->second.rootSignature.Get();
	}

	LogManager::GetInstance().Log("RootSignature not found: " + name);
	assert(0);
	return nullptr;
}

ID3D12PipelineState* PSOManager::GetPSO(const std::string& name) {
	auto it = psoList_.find(name);
	if (it != psoList_.end()) {
		return it->second.pipelineState.Get();
	}

	LogManager::GetInstance().Log("PSO not found: " + name);
	assert(0);
	return nullptr;
}

DrawPsoData PSOManager::GetDrawPsoData(const std::string& PsoName) const {

    auto pso = psoList_.find(PsoName);
    if (pso == psoList_.end()) {
        assert(0);
    }

    auto root = rootSignatureList_.find(pso->second.rootSigName);
    if (root == rootSignatureList_.end()) {
        assert(0);
    }

    DrawPsoData drawData;
    drawData.rootSignature = root->second.rootSignature.Get();
    drawData.graphicsPipelineState = pso->second.pipelineState.Get();
    return drawData;
}

void PSOManager::RegisterCommandSignature(const std::string& name, ID3D12CommandSignature* commandSignature) {
    // 既に登録されていたら飛ばす
    if (commandSignatureList_.find(name) != commandSignatureList_.end()) {
        return;
    }
    commandSignatureList_[name] = commandSignature;
    LogManager::GetInstance().Log("CommandSignature registered name : " + name);
}

ID3D12CommandSignature* PSOManager::GetCommandSignature(const std::string& name) {
    auto it = commandSignatureList_.find(name);
    if (it != commandSignatureList_.end()) {
        return it->second.Get();
    }

    LogManager::GetInstance().Log("CommandSignature not found: " + name);
    assert(0);
    return nullptr;
}

void PSOManager::DefaultLoadPSO() {

    LogManager::GetInstance().Log("Loading default PSOs");

    // デフォルトの3Dオブジェクト用PSO
    CreatePSOData default3D;
    default3D.rootSigName = "Default3D";
    default3D.vsPath = L"Resources/Shaders/Rasterize/Object3d.VS.hlsl";
    default3D.psPath = L"Resources/Shaders/Rasterize/Object3d.PS.hlsl";
    default3D.drawMode = DrawModel::None;
    default3D.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    default3D.isDepthEnable = true;
    RootSignatureBuilder rootSigBuilder;
    rootSigBuilder.Initialize(device_);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    rootSigBuilder.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSRVDescriptorTable(1, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 1, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSRVDescriptorTable(2, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount),2, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    rootSigBuilder.CreateRootSignature();
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateDefaultObjElement();
    RegisterPSO("Default3D", default3D, &rootSigBuilder, &inputLayoutBuilder);

    // 加算合成用PSO
    default3D.blendMode = { BlendMode::kBlendModeAddAndSaveObjectAlpha };
    RegisterPSO("Additive3D", default3D, &rootSigBuilder, &inputLayoutBuilder);

    // デフォルトのスプライト用PSO
    CreatePSOData defaultSprite;
    defaultSprite.rootSigName = "Default2D";
    defaultSprite.vsPath = L"Resources/Shaders/Rasterize/Sprite.VS.hlsl";
    defaultSprite.psPath = L"Resources/Shaders/Rasterize/Sprite.PS.hlsl";
    defaultSprite.drawMode = DrawModel::None;
    defaultSprite.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    defaultSprite.isDepthEnable = false;
    RootSignatureBuilder spriteRoot;
    spriteRoot.Initialize(device_);
    spriteRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
    spriteRoot.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    spriteRoot.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    spriteRoot.CreateRootSignature();
    InputLayoutBuilder spriteInput;
    spriteInput.CreateDefaultSpriteElement();
    RegisterPSO("DefaultSprite", defaultSprite,&spriteRoot,&spriteInput);
    defaultSprite.blendMode = { BlendMode::kBlendModeAdd };
    RegisterPSO("AdditiveSprite", defaultSprite, &spriteRoot,&spriteInput);

    // インスタンシング描画用PSO
    CreatePSOData instancing3D;
    instancing3D.rootSigName = "Instancing3D";
    instancing3D.vsPath = L"Resources/Shaders/Rasterize/Particle.VS.hlsl";
    instancing3D.psPath = L"Resources/Shaders/Rasterize/Particle.PS.hlsl";
    instancing3D.drawMode = DrawModel::FillFront;
    instancing3D.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    instancing3D.isDepthEnable = true;
    RootSignatureBuilder instancingRootSigBuilder;
    instancingRootSigBuilder.Initialize(device_);
    instancingRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.AddSRVDescriptorTable(0, 1,0, D3D12_SHADER_VISIBILITY_VERTEX);
    instancingRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount),0, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    instancingRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.CreateRootSignature();
    RegisterPSO("Instancing3D", instancing3D, &instancingRootSigBuilder, &inputLayoutBuilder);

    // インスタンシング描画の加算合成用PSO
    instancing3D.drawMode = DrawModel::None;
    instancing3D.blendMode = { BlendMode::kBlendModeAddAndSaveObjectAlpha };
    instancing3D.depthMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みだけ無効化
    RegisterPSO("AdditiveInstancing3D", instancing3D, &instancingRootSigBuilder, &inputLayoutBuilder);

    // テキスト描画用PSO
    CreatePSOData text;
    text.rootSigName = "Text";
    text.vsPath = L"Resources/Shaders/Rasterize/Text.VS.hlsl";
    text.psPath = L"Resources/Shaders/Rasterize/Text.PS.hlsl";
    text.drawMode = DrawModel::None;
    text.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    text.isDepthEnable = false;
    RootSignatureBuilder TextRootSigBuilder;
    TextRootSigBuilder.Initialize(device_);
    TextRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL); // テキストの定数
    TextRootSigBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // 文字データ
    TextRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL); // フォントアトラス
    TextRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);
    TextRootSigBuilder.CreateRootSignature();
    // 頂点はVSで生成するので入力は無し
    InputLayoutBuilder textInput;
    textInput.CreateNone();
    RegisterPSO("Text", text, &TextRootSigBuilder, &textInput);

    // グリッド描画用のPSO
    CreatePSOData grid;
    grid.rootSigName = "Grid";
    grid.vsPath = L"Resources/Shaders/Rasterize/Grid.VS.hlsl";
    grid.psPath = L"Resources/Shaders/Rasterize/Grid.PS.hlsl";
    grid.drawMode = DrawModel::None;
    grid.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    grid.isDepthEnable = true;
    RootSignatureBuilder gridRootSigBuilder;
    gridRootSigBuilder.Initialize(device_);
    gridRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    gridRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    gridRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder gridInputLayoutBuilder;
    gridInputLayoutBuilder.CreateDefaultObjElement();
    RegisterPSO("Grid", grid, &gridRootSigBuilder, &gridInputLayoutBuilder);

    // デバックライン描画用のPSO
    CreatePSOData line;
    line.rootSigName = "Line";
    line.vsPath = L"Resources/Shaders/Rasterize/Primitive.VS.hlsl";
    line.psPath = L"Resources/Shaders/Rasterize/Primitive.PS.hlsl";
    line.drawMode = DrawModel::None;
    line.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    line.isDepthEnable = true;
    line.primitiveType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    RootSignatureBuilder lineRootSigBuilder;
    lineRootSigBuilder.Initialize(device_);
    lineRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    lineRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder lineInputLayoutBuilder;
    lineInputLayoutBuilder.CreateDefaultLineElement();
    RegisterPSO("Line", line, &lineRootSigBuilder, &lineInputLayoutBuilder);

    // アニメーション描画用のPSO
    CreatePSOData animation;
    animation.rootSigName = "Animation";
    animation.vsPath = L"Resources/Shaders/Rasterize/SkinningObject3d.VS.hlsl";
    animation.psPath = L"Resources/Shaders/Rasterize/Object3d.PS.hlsl";
    animation.drawMode = DrawModel::FillFront;
    animation.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
    animation.isDepthEnable = true;
    RootSignatureBuilder animationRootSigBuilder;
    animationRootSigBuilder.Initialize(device_);
    animationRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    animationRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(0, 1,0, D3D12_SHADER_VISIBILITY_VERTEX);
    animationRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    animationRootSigBuilder.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(1, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 1, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(2, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount), 2, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    animationRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder animationInputLayoutBuilder;
    animationInputLayoutBuilder.CreateDefaultAnimationElement();
    RegisterPSO("Animation", animation, &animationRootSigBuilder, &animationInputLayoutBuilder);

    // skyboxのpso設定
    CreatePSOData skybox;
    skybox.rootSigName = "Skybox";
    skybox.vsPath = L"Resources/Shaders/Rasterize/Skybox.VS.hlsl";
    skybox.psPath = L"Resources/Shaders/Rasterize/Skybox.PS.hlsl";
    skybox.drawMode = DrawModel::FillFront;
    skybox.blendMode = { BlendMode::kBlendModeNormal };
    skybox.isDepthEnable = true;
    skybox.depthMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
    RootSignatureBuilder skyRoot;
    skyRoot.Initialize(device_);
    skyRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    skyRoot.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);
    skyRoot.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.CreateRootSignature();
    InputLayoutBuilder skyInput;
    skyInput.CreateDefaultObjElement();
    RegisterPSO("Skybox", skybox, &skyRoot, &skyInput);

    // ShadowMap用のPSO設定
    CreatePSOData shadowMap;
    shadowMap.rootSigName = "ShadowMap";
    shadowMap.vsPath = L"Resources/Shaders/Rasterize/ShadowMap.VS.hlsl";
    shadowMap.blendMode = { BlendMode::kBlendModeNormal };
    RootSignatureBuilder shadowMapRootSigBuilder;
    shadowMapRootSigBuilder.Initialize(device_);
    shadowMapRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    shadowMapRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);
    shadowMapRootSigBuilder.CreateRootSignature();
    RegisterShadowMapPSO("ShadowMap", shadowMap, &shadowMapRootSigBuilder, &inputLayoutBuilder);

    // アニメーション用のコンピュートPSO設定
    CreatePSOData computeAnimation;
    computeAnimation.rootSigName = "ComputeAnimation";
    computeAnimation.csPath = L"Resources/Shaders/CS/Skinning.CS.hlsl";
    RootSignatureBuilder animationCsRs;
    animationCsRs.Initialize(device_);
    animationCsRs.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
    animationCsRs.AddSRVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
    animationCsRs.AddSRVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
    animationCsRs.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
    animationCsRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
    animationCsRs.CreateRootSignature();
    RegisterComputePSO("ComputeAnimation", computeAnimation, &animationCsRs);


    // wboit描画用PSO
    CreatePSOData wboit3D;
    wboit3D.rootSigName = "wboit3D";
    wboit3D.vsPath = L"Resources/Shaders/Rasterize/WBOITAccumulate.VS.hlsl";
    wboit3D.psPath = L"Resources/Shaders/Rasterize/WBOITAccumulate.PS.hlsl";
    wboit3D.drawMode = DrawModel::FillFront;
    wboit3D.blendMode = { BlendMode::kBlendModeWboitAccumulation, BlendMode::kBlendModeWboitRevealage };
    wboit3D.isDepthEnable = true;
    wboit3D.depthMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    wboit3D.numRenderTargets = 2;
    wboit3D.rtvFormats = { DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8_UNORM };
    RootSignatureBuilder wboitRootSigBuilder;
    wboitRootSigBuilder.Initialize(device_);
    wboitRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    wboitRootSigBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    wboitRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    wboitRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    wboitRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_PIXEL);
    wboitRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    wboitRootSigBuilder.CreateRootSignature();
    RegisterPSO("wboit3D", wboit3D, &wboitRootSigBuilder, &inputLayoutBuilder);

    // パーティクル用のコンピュートPSO設定
    {
        CreatePSOData computeParticle;
        computeParticle.rootSigName = "EmitComputeParticle";
        computeParticle.csPath = L"Resources/Shaders/CS/Particle.CS.hlsl";
        RootSignatureBuilder particleCsRs;
        particleCsRs.Initialize(device_);
        particleCsRs.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddUAVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddUAVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.CreateRootSignature();
        RegisterComputePSO("EmitComputeParticle", computeParticle, &particleCsRs);
    }

    {
        CreatePSOData computeParticle;
        computeParticle.rootSigName = "UpdateComputeParticle";
        computeParticle.csPath = L"Resources/Shaders/CS/ParticleUpdate.CS.hlsl";
        RootSignatureBuilder particleCsRs;
        particleCsRs.Initialize(device_);
        particleCsRs.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddUAVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddUAVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
        particleCsRs.CreateRootSignature();
        RegisterComputePSO("UpdateComputeParticle", computeParticle, &particleCsRs);
    }

    // CSパーティクル描画用PSO
    {
        CreatePSOData CSParticle3D;
        CSParticle3D.rootSigName = "CSParticle";
        CSParticle3D.vsPath = L"Resources/Shaders/CS/ParticleC.VS.hlsl";
        CSParticle3D.psPath = L"Resources/Shaders/CS/ParticleC.PS.hlsl";
        CSParticle3D.drawMode = DrawModel::FillFront;
        CSParticle3D.blendMode = { BlendMode::kBlendModeAddAndSaveObjectAlpha };
        CSParticle3D.isDepthEnable = true;
        RootSignatureBuilder CsParticleRs;
        CsParticleRs.Initialize(device_);
        CsParticleRs.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        CsParticleRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
        CsParticleRs.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
        CsParticleRs.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        CsParticleRs.CreateRootSignature();
        RegisterPSO("CSParticle3D", CSParticle3D, &CsParticleRs, &inputLayoutBuilder);
    }

    // 破片描画用PSO
    {
        CreatePSOData fracture3D;
        fracture3D.rootSigName = "Fracture3D";
        fracture3D.vsPath = L"Resources/Shaders/Fracture.VS.hlsl";
        fracture3D.psPath = L"Resources/Shaders/Fracture.PS.hlsl";
        fracture3D.drawMode = DrawModel::FillFront;
        fracture3D.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
        fracture3D.isDepthEnable = true;
        RootSignatureBuilder fractureRs;
        fractureRs.Initialize(device_);
        fractureRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        fractureRs.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
        fractureRs.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.Add32BitConstantsParameter(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        fractureRs.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.CreateRootSignature();
        RegisterPSO("Fracture3D", fracture3D, &fractureRs, &inputLayoutBuilder);

        // ----------------------------------------------------
        // 一時的なテストのためここにExecuteIndirect用のコマンドシグネチャを作成
        // ----------------------------------------------------
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> indirectCommandSignature;

        // 破片描画
        UINT kFractureInstanceIndexRootParam = 5;

        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[2] = {};
        argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        argumentDescs[0].Constant.RootParameterIndex = kFractureInstanceIndexRootParam;
        argumentDescs[0].Constant.DestOffsetIn32BitValues = 0;
        argumentDescs[0].Constant.Num32BitValuesToSet = 1;

        argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
        sigDesc.ByteStride = sizeof(FractureIndirectCommand);
        sigDesc.NumArgumentDescs = _countof(argumentDescs);
        sigDesc.pArgumentDescs = argumentDescs;

        HRESULT hr = device_->CreateCommandSignature(
            &sigDesc,
            fractureRs.GetRootSignature(),
            IID_PPV_ARGS(&indirectCommandSignature));
        assert(SUCCEEDED(hr));

        // 登録
        RegisterCommandSignature("DrawIndexedIndirect", indirectCommandSignature.Get());
    }

    // 氷の破片描画用PSO
    {
        CreatePSOData fracture3D;
        fracture3D.rootSigName = "IceFracture3D";
        fracture3D.vsPath = L"Resources/Shaders/Fracture.VS.hlsl";
        fracture3D.psPath = L"Resources/Shaders/IceFracture.PS.hlsl";
        fracture3D.drawMode = DrawModel::None;
        fracture3D.blendMode = { BlendMode::kBlendModeNormalAndSaveObjectAlpha };
        fracture3D.isDepthEnable = true;
        RootSignatureBuilder fractureRs;
        fractureRs.Initialize(device_);
        fractureRs.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        fractureRs.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
        fractureRs.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.Add32BitConstantsParameter(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        fractureRs.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
        fractureRs.CreateRootSignature();
        RegisterPSO("IceFracture3D", fracture3D, &fractureRs, &inputLayoutBuilder);

        // ----------------------------------------------------
        // 一時的なテストのためここにExecuteIndirect用のコマンドシグネチャを作成
        // ----------------------------------------------------
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> indirectCommandSignature;

        // 破片描画
        UINT kFractureInstanceIndexRootParam = 5;

        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[2] = {};
        argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        argumentDescs[0].Constant.RootParameterIndex = kFractureInstanceIndexRootParam;
        argumentDescs[0].Constant.DestOffsetIn32BitValues = 0;
        argumentDescs[0].Constant.Num32BitValuesToSet = 1;

        argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
        sigDesc.ByteStride = sizeof(FractureIndirectCommand);
        sigDesc.NumArgumentDescs = _countof(argumentDescs);
        sigDesc.pArgumentDescs = argumentDescs;

        HRESULT hr = device_->CreateCommandSignature(
            &sigDesc,
            fractureRs.GetRootSignature(),
            IID_PPV_ARGS(&indirectCommandSignature));
        assert(SUCCEEDED(hr));

        // 登録
        RegisterCommandSignature("IceDrawIndexedIndirect", indirectCommandSignature.Get());
    }

    LogManager::GetInstance().Log("Default PSOs loaded");
}

void PSOManager::DefaultLoadPostEffectPSO() {
    // ポストエフェクト共通のinputlayoutを作成
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateNone();

    // 色調調整、画像装飾のポストエフェクト
    CreatePSOData defaultPostEffect;
    defaultPostEffect.rootSigName = "DefaultPostEffect";
    defaultPostEffect.vsPath = L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/ColorGrading.PS.hlsl";
    defaultPostEffect.drawMode = DrawModel::FillFront;
    defaultPostEffect.blendMode = { BlendMode::kBlendModeNone };
    defaultPostEffect.isDepthEnable = false;
    RootSignatureBuilder rootSigBuilder;
    rootSigBuilder.Initialize(device_);
    rootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount),
        0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.CreateRootSignature();
    RegisterPSO("ColorGrading", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);


    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/HighLumMask.PS.hlsl";
    RegisterPSO("HighLumMask", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // 縦のぼかし
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Smoothing/Gauss9x9Vertical.PS.hlsl";
    RegisterPSO("GaussVertical", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // 横のぼかし
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Smoothing/Gauss9x9Horizontal.PS.hlsl";
    RegisterPSO("GaussHorizontal", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // 合成
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Bloom.PS.hlsl";
    RegisterPSO("Bloom", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // ディゾルブ
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Dissolve/Dissolve.PS.hlsl";
    RegisterPSO("Dissolve", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // ラスタライズとレイトレの描画を合成する
    defaultPostEffect.rootSigName = "LightingComposite";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/LightingComposite.PS.hlsl";
    RootSignatureBuilder rsBuilder;
    rsBuilder.Initialize(device_);
    rsBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsBuilder.AddSRVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsBuilder.AddSRVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsBuilder.AddSRVDescriptorTable(3, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rsBuilder.CreateRootSignature();
    RegisterPSO("LightingComposite", defaultPostEffect, &rsBuilder, &inputLayoutBuilder);

    // wboitとの合成用
    defaultPostEffect.rootSigName = "wboitResolve";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/WBOITResolve.PS.hlsl";
    RootSignatureBuilder rsWboitBuilder;
    rsWboitBuilder.Initialize(device_);
    rsWboitBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsWboitBuilder.AddSRVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsWboitBuilder.AddSRVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsWboitBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rsWboitBuilder.CreateRootSignature();
    RegisterPSO("wboitResolve", defaultPostEffect, &rsWboitBuilder, &inputLayoutBuilder);

    // rtvのコピー
    defaultPostEffect.rootSigName = "ColorCopy";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Copy.PS.hlsl";
    defaultPostEffect.drawMode = DrawModel::FillFront;
    defaultPostEffect.blendMode = { BlendMode::kBlendModeNone };
    defaultPostEffect.isDepthEnable = false;
    RootSignatureBuilder rsCopyBuilder;
    rsCopyBuilder.Initialize(device_);
    rsCopyBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsCopyBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rsCopyBuilder.CreateRootSignature();
    RegisterPSO("ColorCopy", defaultPostEffect, &rsCopyBuilder, &inputLayoutBuilder);

    // 深度値をコピーするため
    defaultPostEffect.rootSigName = "DepthCopy";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/DepthCopy.PS.hlsl";
    defaultPostEffect.depthFunc = D3D12_COMPARISON_FUNC_ALWAYS; // 常に上書き
    defaultPostEffect.numRenderTargets = 0; // カラー出力なし
    defaultPostEffect.isDepthEnable = true;
    RootSignatureBuilder rsDepthCopyBuilder;
    rsDepthCopyBuilder.Initialize(device_);
    rsDepthCopyBuilder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rsDepthCopyBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rsDepthCopyBuilder.CreateRootSignature();
    RegisterPSO("DepthCopy", defaultPostEffect, &rsDepthCopyBuilder, &inputLayoutBuilder);
}
