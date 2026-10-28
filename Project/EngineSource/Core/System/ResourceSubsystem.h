#pragma once
#include "IEngineSubsystem.h"

#include "TextureManager.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "AudioManager.h"
#include "GameParamEditor.h"
#include "EffectsManager.h"

namespace GameEngine {

    /// <summary>
    /// リソース管理機能
    /// </summary>
    class ResourceSubsystem : public IEngineSubsystem {
    public:
        ResourceSubsystem();
        ~ResourceSubsystem();

        void Initialize() override;
        void Finalize()   override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        /// リソースの一括ロード
        void LoadAllResources();

        TextureManager* GetTextureManager()   const { return textureManager_.get(); }
        FontManager* GetFontManager()         const { return fontManager_.get(); }
        ModelManager* GetModelManager()     const { return modelManager_.get(); }
        AnimationManager* GetAnimationManager() const { return animationManager_.get(); }
        GameParamEditor* GetGameParamEditor() const { return gameParamEditor_.get(); }
        EffectsManager* GetEffectsManager() const { return effectsManager_.get(); }
    private:

        // 画像データ管理
        std::unique_ptr<TextureManager> textureManager_;
        // フォントデータ管理
        std::unique_ptr<FontManager> fontManager_;
        // モデルデータ管理
        std::unique_ptr<ModelManager> modelManager_;
        // アニメーションデータ管理
        std::unique_ptr<AnimationManager> animationManager_;
        // パラメータシステム
        std::unique_ptr<GameParamEditor> gameParamEditor_;
        // エフェクトの管理
        std::unique_ptr<EffectsManager> effectsManager_;
    };
}