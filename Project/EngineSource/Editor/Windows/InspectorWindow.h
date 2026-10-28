#pragma once
#include "IEditorWindow.h"
#include "GameParamEditor.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "CurveEditor.h"

namespace GameEngine {

	class InspectorWindow : public IEditorWindow {
	public:
		InspectorWindow(GameParamEditor* gameParamEditor, TextureManager* textureManager);

		void Draw() override;
		std::string GetName() const override { return "ParameterInspector"; }

	private:
		GameParamEditor* gameParamEditor_ = nullptr;
		
		TextureManager* textureManager_ = nullptr;

		// アイコン画像のハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE saveIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE loadIconTextureHandle_{};

	private:

		void DrawGroup(GameParamEditor::Group& group);

		void DrawItems(GameParamEditor::Group& group);

	};

	// ImGuiで表示する用のパラメータを管理する
	struct DebugParameterVisitor {
		const std::string& itemName;
		bool& isDirty;
		TextureManager* textureManager_;
		explicit DebugParameterVisitor(const std::string& name,bool& dirty, TextureManager* textureManager)
			: itemName(name), isDirty(dirty), textureManager_(textureManager) {}

		// 画像ファイルかどうか判定する
		static bool IsTextureFile(const char* pathStr) {
			std::string ext = std::filesystem::path(pathStr).extension().string();
			for (char& c : ext) {
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			}
			return ext == ".png" || ext == ".jpg";
		}

		// ## 付きのIDを生成するヘルパー
		std::string HiddenLabel() const {
			return "##" + itemName;
		}

		void operator()(int32_t& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragInt(HiddenLabel().c_str(), &value, 1)){ isDirty = true; }
		}

		void operator()(uint32_t& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragScalar(HiddenLabel().c_str(), ImGuiDataType_U32, &value, 1.0f)){ isDirty = true; }
		}

		void operator()(float& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::DragFloat(HiddenLabel().c_str(), &value, 0.01f)){ isDirty = true; }
		}

		void operator()(Vector2& value) const {
			ImGui::Text("%s", itemName.c_str());
			if (GameEngine::DrawFloat2Control(HiddenLabel().c_str(), reinterpret_cast<float*>(&value))) { isDirty = true; }
		}

		void operator()(Vector3& value) const {
			ImGui::Text("%s", itemName.c_str());
			if (GameEngine::DrawFloat3Control(HiddenLabel().c_str(), reinterpret_cast<float*>(&value))) { isDirty = true; }
		}

		void operator()(Vector4& value) const {
			ImGui::Text("%s", itemName.c_str());
			if(ImGui::ColorEdit4(HiddenLabel().c_str(), reinterpret_cast<float*>(&value))){ isDirty = true; }
		}

		void operator()(Range3& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = GameEngine::DrawFloat3Control("Min", reinterpret_cast<float*>(&value.min));
				bool isChangeMax = GameEngine::DrawFloat3Control("Max", reinterpret_cast<float*>(&value.max));

				if (isChangeMin || isChangeMax) {
					value.min = Math::Min(value.min, value.max);
					value.max = Math::Max(value.min, value.max);
					isDirty = true;
				}
				ImGui::TreePop();
			}
		}

		void operator()(Range4& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				bool isChangeMin = ImGui::ColorEdit4("Min", reinterpret_cast<float*>(&value.min));
				bool isChangeMax = ImGui::ColorEdit4("Max", reinterpret_cast<float*>(&value.max));

				if (isChangeMin || isChangeMax) {
					value.min = Math::Min(value.min, value.max);
					value.max = Math::Max(value.min, value.max);
					isDirty = true;
				}
				ImGui::TreePop();
			}
		}

		void operator()(EmitterShape& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {

				// 形状選択
				int typeIdx = static_cast<int>(value.type);
				if (ImGui::Combo("Shape", &typeIdx, EmitShapeTypeNames, kEmitShapeTypeCount)) {
					value.type = static_cast<EmitShapeType>(typeIdx);
					isDirty = true;
				}

				// 形状ごとのパラメータ
				switch (value.type) {
				case EmitShapeType::Point:
					ImGui::TextDisabled("No parameters");
					break;

				case EmitShapeType::Sphere:
				case EmitShapeType::Hemisphere:
				case EmitShapeType::Circle:
					if (ImGui::DragFloat("Radius", &value.radius, 0.01f, 0.0f, FLT_MAX)) {
						isDirty = true;
					}
					if (ImGui::Checkbox("EmitFromShell", &value.emitFromShell)) {
						isDirty = true;
					}
					break;

				case EmitShapeType::Box:
					if (GameEngine::DrawFloat3Control("BoxSize", reinterpret_cast<float*>(&value.boxSize))) {
						isDirty = true;
					}
					break;
				}

				

				ImGui::TreePop();
			}
		}

		void operator()(TextureData& value) const {
			// 名前からハンドルを解決する
			if (textureManager_ && !value.name.empty()) {
				value.handle = textureManager_->GetHandleByName(value.name);
			}

			ImGui::Text("%s", itemName.c_str());

			// コンボボックスでテクスチャを選択
			const char* comboLabel = value.name.empty() ? "None" : value.name.c_str();
			if (ImGui::BeginCombo(HiddenLabel().c_str(), comboLabel)) {
				if (textureManager_) {
					for (const auto& texName : textureManager_->GetRegisteredTextureNames()) {
						bool isSelected = (texName == value.name);
						if (ImGui::Selectable(texName.c_str(), isSelected)) {
							value.name = texName;
							value.handle = textureManager_->GetHandleByName(texName);
							isDirty = true;
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}

			// プレビュー表示
			if (textureManager_ && !value.name.empty()) {
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = textureManager_->GetTextureSrvHandlesGPU(value.handle);
				ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(64, 64));
			}

			// ドラック&ドロップの受け取り
			if (const ImGuiPayload* peek = ImGui::GetDragDropPayload()) {
				if (peek->IsDataType("CONTENT_PATH") && IsTextureFile(static_cast<const char*>(peek->Data))) {
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_PATH")) {
							std::filesystem::path path(static_cast<const char*>(payload->Data));
							std::string fileName = path.filename().string();

							// 登録済みテクスチャのみ受け付ける
							if (textureManager_) {
								const auto& names = textureManager_->GetRegisteredTextureNames();
								if (std::find(names.begin(), names.end(), fileName) != names.end()) {
									value.name = fileName;
									value.handle = textureManager_->GetHandleByName(fileName);
									isDirty = true;
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
		}

		void operator()(ColliderShapeData& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {

				// 形状選択
				int typeIdx = static_cast<int>(value.type);
				if (ImGui::Combo("Shape", &typeIdx, ShapeTypeNames, static_cast<int>(ShapeType::kMaxCount))) {
					value.type = static_cast<ShapeType>(typeIdx);
					isDirty = true;
				}

				// 形状ごとのパラメータ
				switch (value.type) {
				case ShapeType::kSphere:
					if (ImGui::DragFloat("Radius", &value.radius, 0.01f, 0.0f, FLT_MAX)) {
						isDirty = true;
					}
					break;

				case ShapeType::kAABB:
					if (GameEngine::DrawFloat3Control("BoxSize", reinterpret_cast<float*>(&value.boxSize), 100.0f)) {
						isDirty = true;
					}
					if (GameEngine::DrawFloat3Control("AnchorPoint", reinterpret_cast<float*>(&value.anchorPoint), 100.0f)) {
						isDirty = true;
					}
					break;

				case ShapeType::kOBB:
					if (GameEngine::DrawFloat3Control("BoxSize", reinterpret_cast<float*>(&value.boxSize), 100.0f)) {
						isDirty = true;
					}
					if (GameEngine::DrawFloat3Control("AnchorPoint", reinterpret_cast<float*>(&value.anchorPoint),100.0f)) {
						isDirty = true;
					}
					if (GameEngine::DrawFloat3Control("Rotate", reinterpret_cast<float*>(&value.rotate), 100.0f)) {
						isDirty = true;
					}
					break;

				case ShapeType::kSegment:
					ImGui::TextDisabled("No parameters");
					break;
				}

				ImGui::TreePop();
			}
		}

		void operator()(EaseType& value) const {
			// 種類を選択
			int typeIdx = static_cast<int>(value);
			if (ImGui::Combo("EaseType", &typeIdx, EaseTypeNames, static_cast<int>(EaseType::kMaxCount))) {
				value = static_cast<EaseType>(typeIdx);
				isDirty = true;
			}
		}

		void operator()(EaseCurve& value) const {
			if (ImGui::TreeNode(itemName.c_str())) {
				if (CurveEditor::Draw(HiddenLabel().c_str(), value)) { isDirty = true; }
				ImGui::TreePop();
			}
		}

		void operator()(bool& value) const {
			if (ImGui::Checkbox(itemName.c_str(), &value)) { isDirty = true; }
		}

		void operator()(std::string& value) const {
			ImGui::Text("%s", value.c_str());
		}

		// 対応出来ない型がきた場合の処理
		template<typename T>
		void operator()(T& value) const {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[%s] は未対応の型です", HiddenLabel().c_str());
		}
	};
}
