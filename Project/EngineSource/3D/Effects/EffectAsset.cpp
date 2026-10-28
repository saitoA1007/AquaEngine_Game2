#include "pch.h"
#include "EffectAsset.h"
#include "JsonSerializer.h"
#include "LogManager.h"

using namespace GameEngine;
using json = nlohmann::json;

namespace {

	// Vector3を配列に変換
	json ToJsonVector3(const Vector3& v) {
		return json::array({ v.x, v.y, v.z });
	}

	// {x, y, z}の配列からVector3を取得。形式が違えばデフォルト値を返す
	Vector3 FromJsonVector3(const json& node, const Vector3& defaultValue) {
		if (!node.is_array() || node.size() != 3) {
			return defaultValue;
		}
		return { node[0].get<float>(), node[1].get<float>(), node[2].get<float>() };
	}

	// 発生方法を文字列に変換
	const char* ToString(EffectEmitMode mode) {
		switch (mode) {
		case EffectEmitMode::kBurst: return "Burst";
		case EffectEmitMode::kContinuous:
		default: return "Continuous";
		}
	}

	// 文字列から発生方法を取得
	EffectEmitMode ToEmitMode(const std::string& str) {
		if (str == "Burst") { return EffectEmitMode::kBurst; }
		return EffectEmitMode::kContinuous;
	}
}

json EffectAsset::ToJson() const {
	json root;
	root["Duration"] = duration;
	root["IsLoop"] = isLoop;

	json trackArray = json::array();
	for (const auto& track : tracks) {
		json node;
		node["Name"] = track.name;
		node["ParticleName"] = track.particleName;
		node["ModelName"] = track.modelName;
		node["MaxNum"] = track.maxNum;
		node["EmitMode"] = ToString(track.emitMode);
		node["StartTime"] = track.startTime;
		node["Duration"] = track.duration;
		node["Offset"] = ToJsonVector3(track.offset);
		trackArray.push_back(node);
	}
	root["Tracks"] = trackArray;
	return root;
}

EffectAsset EffectAsset::FromJson(const std::string& name, const json& root) {
	EffectAsset asset;
	asset.name = name;
	asset.duration = root.value("Duration", asset.duration);
	asset.isLoop = root.value("IsLoop", asset.isLoop);

	auto it = root.find("Tracks");
	if (it == root.end() || !it->is_array()) {
		return asset;
	}

	for (const auto& node : *it) {
		EffectTrackData track;
		track.name = node.value("Name", track.name);
		track.particleName = node.value("ParticleName", track.particleName);
		track.modelName = node.value("ModelName", track.modelName);
		track.maxNum = node.value("MaxNum", track.maxNum);
		track.emitMode = ToEmitMode(node.value("EmitMode", std::string(ToString(track.emitMode))));
		track.startTime = node.value("StartTime", track.startTime);
		track.duration = node.value("Duration", track.duration);
		if (node.contains("Offset")) {
			track.offset = FromJsonVector3(node["Offset"], track.offset);
		}
		asset.tracks.push_back(track);
	}
	return asset;
}

void EffectAsset::Save() const {
	JsonSerializer::SaveToFile(kDirectoryPath + name + ".json", ToJson());
}

bool EffectAsset::Load(const std::string& name, EffectAsset& outAsset) {
	const std::string filePath = kDirectoryPath + name + ".json";
	if (!JsonSerializer::FileExists(filePath)) {
		return false;
	}

	try {
		outAsset = FromJson(name, JsonSerializer::LoadFromFile(filePath));
	} catch (const std::exception& e) {
		Log("EffectAsset: Failed to load " + filePath + " : " + e.what());
		return false;
	}
	return true;
}
