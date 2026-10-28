#include "pch.h"
#include "GameParamEditor.h"
#include <cassert>
#include <algorithm>
#include "JsonSerializer.h"
using json = nlohmann::json;
using namespace GameEngine;

void GameParamEditor::SaveFile(const std::string& rootGroupName) {

	// グループを検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(rootGroupName);
	// 未登録チェック
	assert(itGroup != datas_.end());

	// jsonオブジェクト登録
	json root = json::object();
	root[rootGroupName] = json::object();
	// シーン名を保存
	root[rootGroupName]["SceneName"] = activeSceneName_;

	// グループツリーをシリアライズ
	SerializeGroupToJson(root[rootGroupName], itGroup->second);

	// 保存する
	const std::string filePath = kDirectoryPath + rootGroupName + ".json";
	JsonSerializer::SaveToFile(filePath, root);
}

void GameParamEditor::LoadFiles() {
	// ディレクトリ内の全ファイルを取得
	JsonSerializer::LoadDirectory(kDirectoryPath,
		[this](const std::string& stem, const json& root) {
			ParseGroupJson(stem, root);
		}
	);
}

void GameParamEditor::LoadFile(const std::string& rootGroupName) {
	// 読み込みJSONファイルのフルパスを合成する
	const std::string filePath = kDirectoryPath + rootGroupName + ".json";
	const json root = JsonSerializer::LoadFromFile(filePath);
	ParseGroupJson(rootGroupName, root);
}

void GameParamEditor::SetRootGroupName(const std::string& rootGroupName) {
	rootGroupName_ = rootGroupName;
}

void GameParamEditor::SetActiveScene(const std::string& sceneName) {
	activeSceneName_ = sceneName;
}

void GameParamEditor::ParseGroupJson(const std::string& rootGroupName, const json& root) {

	auto itRoot = root.find(rootGroupName);
	assert(itRoot != root.end());

	// シーン名をルートグループに設定
	if (itRoot->contains("SceneName")) {
		datas_[rootGroupName].sceneName = itRoot->at("SceneName").get<std::string>();
	}

	// グループツリーを取得
	DeserializeGroupFromJson(datas_[rootGroupName], *itRoot);
}

void GameParamEditor::SerializeGroupToJson(json& node, const Group& group) const {

	// このグループのアイテムをシリアライズ
	for (const auto& [itemName, item] : group.items) {
		json& jsonNode = node[itemName];
		std::visit(JsonSaveVisitor{ jsonNode }, item.value);
	}

	// サブグループを再帰的にシリアライズ
	for (const auto& [childName, childGroup] : group.children) {
		node[childName] = json::object();
		SerializeGroupToJson(node[childName], childGroup);
	}
}

void GameParamEditor::DeserializeGroupFromJson(Group& group, const json& node) {

	for (auto itItem = node.begin(); itItem != node.end(); ++itItem) {
		const std::string& itemName = itItem.key();

		// SceneName はスキップ
		if (itemName == "SceneName") { continue; }

		switch (itItem->type()) {
		case json::value_t::boolean:
			group.items[itemName].value = itItem->get<bool>();
			break;
		case json::value_t::number_integer:
			group.items[itemName].value = itItem->get<int32_t>();
			break;
		case json::value_t::number_unsigned:
			group.items[itemName].value = itItem->get<uint32_t>();
			break;
		case json::value_t::number_float:
			group.items[itemName].value = itItem->get<float>();
			break;
		case json::value_t::array:
			if (itItem->size() == 2) {
				group.items[itemName].value = Vector2{ itItem->at(0), itItem->at(1) };
			} else if (itItem->size() == 3) {
				group.items[itemName].value = Vector3{ itItem->at(0), itItem->at(1), itItem->at(2) };
			} else if (itItem->size() == 4) {
				group.items[itemName].value = Vector4{ itItem->at(0), itItem->at(1), itItem->at(2), itItem->at(3) };
			}
			break;
		case json::value_t::object:
			if (itItem->contains("_Min") && itItem->contains("_Max")) {
				const auto& minArray = itItem->at("_Min");
				const auto& maxArray = itItem->at("_Max");
				if (minArray.size() == 3) {
					group.items[itemName].value = Range3{
						{ minArray[0], minArray[1], minArray[2] },
						{ maxArray[0], maxArray[1], maxArray[2] }
					};
				} else if (minArray.size() == 4) {
					group.items[itemName].value = Range4{
						{ minArray[0], minArray[1], minArray[2], minArray[3] },
						{ maxArray[0], maxArray[1], maxArray[2], maxArray[3] }
					};
				}
			} else if (itItem->contains("_TextureName")) {
				TextureData texData{};
				texData.name = itItem->at("_TextureName").get<std::string>();
				group.items[itemName].value = texData;
			} else if (itItem->contains("_ShapeType") && itItem->contains("_radius")) {
				EmitterShape shape{};
				std::string typeStr = itItem->at("_ShapeType").get<std::string>();
				for (int i = 0; i < kEmitShapeTypeCount; ++i) {
					if (typeStr == EmitShapeTypeNames[i]) {
						shape.type = static_cast<EmitShapeType>(i);
						break;
					}
				}
				if (itItem->contains("_radius")) { shape.radius = itItem->at("_radius").get<float>(); }
				if (itItem->contains("_emitFromShell")) { shape.emitFromShell = itItem->at("_emitFromShell").get<bool>(); }
				if (itItem->contains("_boxSize")) {
					const auto& s = itItem->at("_boxSize");
					shape.boxSize = { s[0], s[1], s[2] };
				}
				group.items[itemName].value = shape;
			} else if (itItem->contains("_ColliderShapeType") && itItem->contains("_radius")) {

				ColliderShapeData data;
				std::string typeStr = itItem->at("_ColliderShapeType").get<std::string>();
				for (int i = 0; i < static_cast<int>(ShapeType::kMaxCount); ++i) {
					if (typeStr == ShapeTypeNames[i]) {
						data.type = static_cast<ShapeType>(i);
						break;
					}
				}
				if (itItem->contains("_radius")) { data.radius = itItem->at("_radius").get<float>(); }
				if (itItem->contains("_anchorPoint")) {
					const auto& s = itItem->at("_anchorPoint");
					data.anchorPoint = { s[0], s[1], s[2] };
				}
				if (itItem->contains("_boxSize")) {
					const auto& s = itItem->at("_boxSize");
					data.boxSize = { s[0], s[1], s[2] };
				}
				if (itItem->contains("_rotate")) {
					const auto& s = itItem->at("_rotate");
					data.boxSize = { s[0], s[1], s[2] };
				}
				group.items[itemName].value = data;

			} else if (itItem->contains("_EaseType")) {
				EaseType data;
				std::string typeStr = itItem->at("_EaseType").get<std::string>();
				for (int i = 0; i < static_cast<int>(EaseType::kMaxCount); ++i) {
					if (typeStr == EaseTypeNames[i]) {
						data = static_cast<EaseType>(i);
						break;
					}
				}
				group.items[itemName].value = data;
			} else if (itItem->contains("_EaseCurve")) {
				std::vector<EaseKey> keys;
				for (const auto& keyJson : itItem->at("_EaseCurve")) {
					EaseKey key{};
					key.time = keyJson.value("time", 0.0f);
					key.value = keyJson.value("value", 0.0f);
					key.inTangent = keyJson.value("inTangent", 0.0f);
					key.outTangent = keyJson.value("outTangent", 0.0f);
					std::string modeStr = keyJson.value("mode", std::string(TangentModeNames[0]));
					for (int i = 0; i < static_cast<int>(TangentMode::kMaxCount); ++i) {
						if (modeStr == TangentModeNames[i]) {
							key.mode = static_cast<TangentMode>(i);
							break;
						}
					}
					keys.push_back(key);
				}
				group.items[itemName].value = EaseCurve(keys);
			} else {
				// それ以外のオブジェクトはサブグループとして再帰的に読み込む
				DeserializeGroupFromJson(group.children[itemName], *itItem);
			}
			break;
		case json::value_t::string:
			group.items[itemName].value = itItem->get<std::string>();
			break;
		default:
			break;
		}
	}
}