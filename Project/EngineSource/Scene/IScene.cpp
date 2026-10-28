#include "pch.h"
#include "IScene.h"
using namespace GameEngine;

Input* IScene::input_ = nullptr;
InputCommand* IScene::inputCommand_ = nullptr;

TextureManager* IScene::textureManager_ = nullptr;
FontManager* IScene::fontManager_ = nullptr;
ModelManager* IScene::modelManager_ = nullptr;
AnimationManager* IScene::animationManager_ = nullptr;
GameObjectManager* IScene::gameObjectManager_ = nullptr;
EffectsManager* IScene::effectsManager_ = nullptr;

RenderPassController* IScene::renderPassController_ = nullptr;
RenderQueue* IScene::renderQueue_ = nullptr;
PostEffectManager* IScene::postEffectManager_ = nullptr;

DebugRenderer* IScene::debugRenderer_ = nullptr;