#include "pch.h"
#include "Test.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

Test::Test(GameEngine::Model* model) : modelComponent_(model) {

	
	// パラメータ機能
	debugParame_.Register("TestFloat", testFloat_);
	debugParame_.Register("TestPos", testPos_);
	debugParame_.RegisterWorld("", modelComponent_.worldTransform_);
	debugParame_.Apply();
}

void Test::Initialize() {

}

void Test::Update() {
	debugParame_.ApplyIfDirty();

	// 更新処理
	modelComponent_.Update();
}

void Test::Draw() {
	modelComponent_.DrawRaytracing(renderQueue_);
}