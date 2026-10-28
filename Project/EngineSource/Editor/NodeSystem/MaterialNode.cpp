#include "pch.h"
#include "MaterialNode.h"
#include <format>
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "NodeBuilder.h"
using namespace GameEngine;

namespace {
    constexpr float kTexPreviewSize = 128.0f;

    // テクスチャのファイルか判断
    bool IsTextureFile(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        for (char& c : ext) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }
}

//=======================================================
// 数式ノード
//=======================================================

MathNode::MathNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Math";
    inputs_.push_back({ g.GetNextId(), "A", PinType::kFloat4, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "B", PinType::kFloat4, PinKind::kInput, id_ });
    outputs_.push_back({ g.GetNextId(), "Result", PinType::kFloat4, PinKind::kOutput, id_ });
}

void MathNode::DrawNodeUI() {
    static const char* operationNames[] = { "Add", "Subtract", "Multiply", "Divide" };
    const char* currentLabel = operationNames[static_cast<int>(operation_)];

    // ノード単位でIDが衝突しないよう固定IDにする
    std::string buttonId = std::format("{}###MathOpBtn{}", currentLabel, id_);
    std::string popupId = std::format("MathOpPopup{}", id_);

    //ノード内には現在値を表示するボタンだけを置く
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Button(buttonId.c_str(), ImVec2(100.0f, 0.0f))) {
        // クリックされたらポップアップを開く
        ax::NodeEditor::Suspend();
        ImGui::OpenPopup(popupId.c_str());
        ax::NodeEditor::Resume();
    }

    // ポップアップの実描画はSuspendしている間に行う
    ax::NodeEditor::Suspend();
    if (ImGui::BeginPopup(popupId.c_str())) {
        for (int i = 0; i < IM_ARRAYSIZE(operationNames); ++i) {
            bool isSelected = (i == static_cast<int>(operation_));
            if (ImGui::Selectable(operationNames[i], isSelected)) {
                operation_ = static_cast<MathOperation>(i);
            }
        }
        ImGui::EndPopup();
    }
    ax::NodeEditor::Resume();
}

std::string MathNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    // 現在の型に応じたHLSLの型名とデフォルト値を決める
    std::string typeStr = "float4";
    std::string defaultStr = "float4(1,1,1,1)";

    switch (valueType_) {
    case PinType::kFloat:  typeStr = "float";  defaultStr = "1.0f"; break;
    case PinType::kFloat2: typeStr = "float2"; defaultStr = "float2(1,1)"; break;
    case PinType::kFloat3: typeStr = "float3"; defaultStr = "float3(1,1,1)"; break;
    case PinType::kFloat4: typeStr = "float4"; defaultStr = "float4(1,1,1,1)"; break;
    }

    // 適切なデフォルト値を使って変数名を取得
    std::string a = GetPinVar(pinVars, inputs_[0].id, defaultStr);
    std::string b = GetPinVar(pinVars, inputs_[1].id, defaultStr);

    std::string op = "+";
    switch (operation_) {
    case MathOperation::kAdd: op = "+"; break;
    case MathOperation::kSubtract: op = "-"; break;
    case MathOperation::kMultiply: op = "*"; break;
    case MathOperation::kDivide: op = "/"; break;
    }

    return std::format("{} v{} = {} {} {};\\n", typeStr, outputs_[0].id, a, op, b);
}

void MathNode::OnConnectTypePropagate(PinType newType) {
    if (newType == PinType::kFloat || newType == PinType::kFloat2 ||
        newType == PinType::kFloat3 || newType == PinType::kFloat4) {

        // ノードの状態を更新
        valueType_ = newType; 

        // ピンの型を変更する
        inputs_[0].pinType = newType;
        inputs_[1].pinType = newType;
        outputs_[0].pinType = newType;
    }
}

//====================================================
// 定数ノード
//====================================================

ConstantNode::ConstantNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Constant";
    outputs_.push_back({ g.GetNextId(), "Value", PinType::kFloat, PinKind::kOutput, id_ });
}

std::string ConstantNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    return std::format("float v{} = {};\n", outputs_[0].id, value_);
}

void ConstantNode::DrawNodeUI() {
    ImGui::SetNextItemWidth(70.0f);

    ImGui::DragFloat("##val", &value_, 0.01f, 0.0f, 0.0f, "%.3f");
}

//============================================================
// カラーノード
//============================================================

ColorNode::ColorNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Color";
    outputs_.push_back({ g.GetNextId(), "RGBA", PinType::kFloat4, PinKind::kOutput, id_ });
}

std::string ColorNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    return std::format("float4 v{} = float4({}, {}, {}, {});\n",
        outputs_[0].id, color_[0], color_[1], color_[2], color_[3]);
}

void ColorNode::DrawNodeUI() {
   
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;
    ImGui::SetNextItemWidth(100.0f);
    ImGui::ColorEdit4("##color", color_, flags);
}

//=======================================================
// テクスチャノード
//=======================================================

TextureSampleNode::TextureSampleNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Texture_Sample";
    inputs_.push_back({ g.GetNextId(), "UV", PinType::kFloat2, PinKind::kInput, id_ });
    outputs_.push_back({ g.GetNextId(), "RGBA", PinType::kFloat4, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "RGB",  PinType::kFloat3, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "A",    PinType::kFloat,  PinKind::kOutput, id_ });
    textureName = "tex_" + std::to_string(id_);
}

std::string TextureSampleNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    std::string uv = GetPinVar(pinVars, inputs_[0].id, "input.texcoord");

    // バインドレース配列をハンドルで直接添字アクセスする
    return std::format(
        "float4 v{0} = gTexture[{1}].Sample(gSampler, {2});\n"
        "float3 v{3} = v{0}.rgb;\n"
        "float  v{4} = v{0}.a;\n",
        outputs_[0].id, textureHandle_, uv,
        outputs_[1].id, outputs_[2].id
    );
}

void TextureSampleNode::DrawNodeUI() {
    // テクスチャ名の入力
    ImGui::SetNextItemWidth(kTexPreviewSize);
    if (ImGui::InputTextWithHint("##texFile", "texture.png", texFile_.data(), sizeof(texFile_), ImGuiInputTextFlags_EnterReturnsTrue)) {
        ResolveTexture();
    }

    // サムネイル
    ImTextureID tex{};
    if (hasTexture_ && textureManager_) {
        tex = (ImTextureID)textureManager_->GetTextureSrvHandlesGPU(textureHandle_).ptr;
    }
    NodeUI::DrawImagePreview(tex, ImVec2(kTexPreviewSize, kTexPreviewSize));

    // サムネイルをドロップ先にする
    if (ImGui::BeginDragDropTarget()) {
        // 配信前に中身を確認し、画像でなければ強調も受け取りもしない
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
            "CONTENT_PATH",
            ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
        if (payload && payload->DataSize > 0) {
            const std::filesystem::path path(static_cast<const char*>(payload->Data));
            if (IsTextureFile(path)) {
                ImGui::GetWindowDrawList()->AddRect(
                    ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                    IM_COL32(255, 200, 60, 255), 4.0f, 0, 2.0f);
                if (payload->IsDelivery()) { SetTexture(path.filename().string()); }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void TextureSampleNode::ResolveTexture() {
    if (!textureManager_ || texFile_.empty()) {
        hasTexture_ = false; 
        return; 
    }
    textureHandle_ = textureManager_->GetHandleByName(texFile_);
    hasTexture_ = true;
}

void TextureSampleNode::SetTexture(const std::string& fileName) {
    texFile_ = fileName;
    ResolveTexture();
}

//=============================================================
// PBRの出力ノード
//=============================================================

PBROutputNode::PBROutputNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "PBR_Output";
    inputs_.push_back({ g.GetNextId(), "BaseColor",  PinType::kFloat4, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Metallic",   PinType::kFloat,  PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Roughness",  PinType::kFloat,  PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Normal",     PinType::kFloat3, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Emissive",   PinType::kFloat3, PinKind::kInput, id_ });
}

//=============================================================
// カメラノード
//=============================================================

CameraNode::CameraNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Camera";
    outputs_.push_back({ g.GetNextId(), "WorldPosition", PinType::kFloat3, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "ViewDirection",  PinType::kFloat3, PinKind::kOutput, id_ });
}

std::string CameraNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    return std::format(
        "float3 v{0} = gCamera.worldPosition;\n"
        "float3 v{1} = normalize(gCamera.worldPosition - input.worldPosition);\n",
        outputs_[0].id, outputs_[1].id
    );
}

//===============================================================
// ライトノード
//===============================================================

LightNode::LightNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Light";
    outputs_.push_back({ g.GetNextId(), "Direction", PinType::kFloat3, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "Color",     PinType::kFloat3, PinKind::kOutput, id_ });
}

void LightNode::DrawNodeUI() {
    static const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
    const char* currentLabel = lightTypeNames[static_cast<int>(lightType_)];

    std::string buttonId = std::format("{}###LightTypeBtn{}", currentLabel, id_);
    std::string popupId = std::format("LightTypePopup{}", id_);

    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Button(buttonId.c_str(), ImVec2(100.0f, 0.0f))) {
        ax::NodeEditor::Suspend();
        ImGui::OpenPopup(popupId.c_str());
        ax::NodeEditor::Resume();
    }

    ax::NodeEditor::Suspend();
    if (ImGui::BeginPopup(popupId.c_str())) {
        for (int i = 0; i < IM_ARRAYSIZE(lightTypeNames); ++i) {
            bool isSelected = (i == static_cast<int>(lightType_));
            if (ImGui::Selectable(lightTypeNames[i], isSelected)) {
                lightType_ = static_cast<LightType>(i);
            }
        }
        ImGui::EndPopup();
    }
    ax::NodeEditor::Resume();
}

std::string LightNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    pinVars;

    switch (lightType_) {
    case LightType::kPoint:
        return std::format(
            "float3 toLight{0} = gPointLight.position - input.worldPosition;\n"
            "float distLight{0} = length(toLight{0});\n"
            "float3 v{1} = normalize(toLight{0});\n"
            "float attenLight{0} = pow(saturate(-distLight{0} / gPointLight.radius + 1.0f), gPointLight.decay);\n"
            "float3 v{2} = gPointLight.color.rgb * gPointLight.intensity * attenLight{0} * (float)gPointLight.active;\n",
            id_, outputs_[0].id, outputs_[1].id
        );

    case LightType::kSpot:
        return std::format(
            "float3 toLight{0} = gSpotLight.position - input.worldPosition;\n"
            "float distLight{0} = length(toLight{0});\n"
            "float3 dirOnSurface{0} = normalize(toLight{0});\n"
            "float cosAngleLight{0} = dot(-dirOnSurface{0}, normalize(gSpotLight.direction));\n"
            "float falloffLight{0} = saturate((cosAngleLight{0} - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));\n"
            "float attenLight{0} = pow(1.0f / distLight{0}, gSpotLight.decay) * saturate(1.0f - distLight{0} / gSpotLight.distance);\n"
            "float3 v{1} = normalize(-gSpotLight.direction);\n"
            "float3 v{2} = gSpotLight.color.rgb * gSpotLight.intensity * attenLight{0} * falloffLight{0} * (float)gSpotLight.active;\n",
            id_, outputs_[0].id, outputs_[1].id
        );

    case LightType::kDirectional:
    default:
        return std::format(
            "float3 v{0} = normalize(-gDirectionalLight.direction);\n"
            "float3 v{1} = gDirectionalLight.color.rgb * gDirectionalLight.intensity * (float)gDirectionalLight.active;\n",
            outputs_[0].id, outputs_[1].id
        );
    }
}

//==========================================================
// ヘルパー関数
//==========================================================

namespace GameEngine{

    std::string GetPinVar(
        const std::unordered_map<int, std::string>& pinVars, int pinId, const std::string& defaultVal) {
        // ピンIDを確認
        auto it = pinVars.find(pinId);
        if (it != pinVars.end()) {
            // 接続されている変数名を返す
            return it->second;
        }
        // 接続されていなければ、デフォルト値を返す
        return defaultVal;
    }
}

