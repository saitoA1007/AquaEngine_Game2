#include "pch.h"
#include "UpgradeCardView.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
using namespace GameEngine;

// ---------------------------------------------------------------------------
// ShadowedText
// ---------------------------------------------------------------------------

ShadowedText::ShadowedText(const Font* font, const std::string& text, uint32_t maxLength, float fontSize) {
    main_ = std::make_unique<Text>(font, text, maxLength);
    shadow_ = std::make_unique<Text>(font, text, maxLength);
    for (Text* textObject : { main_.get(), shadow_.get() }) {
        textObject->scale_ = { fontSize, fontSize };
        textObject->anchorPoint_ = { 0.5f, 0.5f };
    }
}

void ShadowedText::SetText(const std::string& text) {
    main_->SetText(text);
    shadow_->SetText(text);
}

void ShadowedText::Update(const Vector2& position, const UpgradeCardStyle& style, float alpha) {
    // 本体
    main_->position_ = position;
    main_->color_ = { style.textColor.x, style.textColor.y, style.textColor.z, style.textColor.w * alpha };
    main_->Update();

    // 影は、同じ文字を暗い色で少しずらす
    shadow_->position_ = { position.x + style.shadowOffset.x, position.y + style.shadowOffset.y };
    shadow_->color_ = { style.shadowColor.x, style.shadowColor.y, style.shadowColor.z, style.shadowColor.w * alpha };
    shadow_->Update();
}

void ShadowedText::Draw(RenderQueue* renderQueue) {
    // 影を先に描いて、本体を重ねる
    renderQueue->SubmitText(shadow_.get());
    renderQueue->SubmitText(main_.get());
}

// ---------------------------------------------------------------------------
// UpgradeCardView
// ---------------------------------------------------------------------------

UpgradeCardView::UpgradeCardView(const Font* font, const UpgradeSelectTextures& textures, const UpgradeCardStyle* style, const Vector2& center, const std::string& name)
    : style_(style), center_(center) {
    CreateCardSprites(textures);
    CreateShards(textures);
    CreateTexts(font, name);
}

void UpgradeCardView::CreateCardSprites(const UpgradeSelectTextures& textures) {
    // 板と枠は、まわりの透明な余白を含めた画像の大きさで、中心を基準に置く（見た目のカードの大きさは変わらない）
    plateSprite_ = std::make_unique<Sprite>(center_, kImageSize, Vector2{ 0.5f, 0.5f }, style_->plateColor);
    plateSprite_->textureHandle_ = textures.plate;

    frameSprite_ = std::make_unique<Sprite>(center_, kImageSize, Vector2{ 0.5f, 0.5f }, style_->frameColor);
    frameSprite_->textureHandle_ = textures.frame;

    // 光は、余白つきの1コマの大きさで置く。UVでコマを選ぶ
    lightSprite_ = std::make_unique<Sprite>(center_, kLightCellSize, Vector2{ 0.5f, 0.5f }, style_->lightColor);
    lightSprite_->textureHandle_ = textures.light;
    UpdateLightFrame();
}

void UpgradeCardView::CreateShards(const UpgradeSelectTextures& textures) {
    // 破片は板と枠の画像をそのまま使う。どちらも同じUVで切り出すので、重ねると1枚のカードと同じ見た目になる
    for (int32_t i = 0; i < kMaxShards; ++i) {
        size_t index = static_cast<size_t>(i);

        plateShards_[index] = std::make_unique<Sprite>(center_, Vector2{ 1.0f, 1.0f }, Vector2{ 0.5f, 0.5f }, style_->plateColor);
        plateShards_[index]->textureHandle_ = textures.plate;

        frameShards_[index] = std::make_unique<Sprite>(center_, Vector2{ 1.0f, 1.0f }, Vector2{ 0.5f, 0.5f }, style_->frameColor);
        frameShards_[index]->textureHandle_ = textures.frame;
    }
    BuildShardLayout();
}

bool UpgradeCardView::IsShardLayoutOutdated() const {
    return style_->shardAngle != builtShardAngle_ || style_->shardColumns != builtShardColumns_ ||
        style_->shardRows != builtShardRows_ || style_->shardJitter != builtShardJitter_;
}

void UpgradeCardView::BuildShardLayout() {
    // 割ったときの調整値を覚える（変わったら割り直す）
    builtShardAngle_ = style_->shardAngle;
    builtShardColumns_ = style_->shardColumns;
    builtShardRows_ = style_->shardRows;
    builtShardJitter_ = style_->shardJitter;

    int32_t columns = std::clamp(style_->shardColumns, 1, kMaxShardColumns);
    int32_t rows = std::clamp(style_->shardRows, 1, kMaxShardRows);
    float jitter = std::clamp(style_->shardJitter, 0.0f, kMaxShardJitter);
    shardAngleRadians_ = style_->shardAngle * std::numbers::pi_v<float> / 180.0f;

    // 格子の向き（u: 列が並ぶ向き、v: 行が並ぶ向き）。スプライトの回転と同じ向きにそろえる
    float cosAngle = std::cos(shardAngleRadians_);
    float sinAngle = std::sin(shardAngleRadians_);
    Vector2 axisU = { cosAngle, sinAngle };
    Vector2 axisV = { -sinAngle, cosAngle };

    // カード全体を覆う格子の範囲（格子の向きから見たカードの半分の大きさ）
    Vector2 halfCard = kCardSize * 0.5f;
    float halfU = halfCard.x * std::abs(cosAngle) + halfCard.y * std::abs(sinAngle);
    float halfV = halfCard.x * std::abs(sinAngle) + halfCard.y * std::abs(cosAngle);

    std::mt19937 random(kShardSeed);
    std::uniform_real_distribution<float> signedUnit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (ShardPiece& piece : shardPieces_) {
        piece.isActive = false;
    }

    // 行の線は全体でそろえ、列の線は行ごとにずらす（ずらしても行の中で隙間なく並ぶ）
    std::vector<float> rowLines(static_cast<size_t>(rows + 1));
    for (int32_t r = 0; r <= rows; ++r) {
        float line = -halfV + 2.0f * halfV * static_cast<float>(r) / static_cast<float>(rows);
        bool isInner = (r > 0 && r < rows);
        rowLines[static_cast<size_t>(r)] = isInner ? line + signedUnit(random) * jitter * (2.0f * halfV / static_cast<float>(rows)) : line;
    }

    int32_t pieceCount = 0;
    for (int32_t r = 0; r < rows; ++r) {
        std::vector<float> columnLines(static_cast<size_t>(columns + 1));
        for (int32_t c = 0; c <= columns; ++c) {
            float line = -halfU + 2.0f * halfU * static_cast<float>(c) / static_cast<float>(columns);
            bool isInner = (c > 0 && c < columns);
            columnLines[static_cast<size_t>(c)] = isInner ? line + signedUnit(random) * jitter * (2.0f * halfU / static_cast<float>(columns)) : line;
        }

        for (int32_t c = 0; c < columns; ++c) {
            float u0 = columnLines[static_cast<size_t>(c)];
            float u1 = columnLines[static_cast<size_t>(c + 1)];
            float v0 = rowLines[static_cast<size_t>(r)];
            float v1 = rowLines[static_cast<size_t>(r + 1)];
            float centerU = (u0 + u1) * 0.5f;
            float centerV = (v0 + v1) * 0.5f;
            Vector2 size = { u1 - u0, v1 - v0 };
            Vector2 offset = { axisU.x * centerU + axisV.x * centerV, axisU.y * centerU + axisV.y * centerV };

            // 乱数は、使わない区画でも同じ数だけ引く（区画の判定が変わっても、他の破片の動きが変わらないようにする）
            float angleNoise = signedUnit(random) * kScatterAngleSpread;
            float distanceRatio = kScatterMinRatio + (1.0f - kScatterMinRatio) * unit(random);
            float rotationRatio = signedUnit(random);
            float fallbackAngle = signedUnit(random) * std::numbers::pi_v<float>;

            // カードと重ならない区画は作らない（画面の縦横の軸で分けて判定する。格子の軸ではカードの外に出ない）
            float halfWidth = size.x * 0.5f;
            float halfHeight = size.y * 0.5f;
            bool isSeparatedX = std::abs(offset.x) > halfCard.x + halfWidth * std::abs(cosAngle) + halfHeight * std::abs(sinAngle);
            bool isSeparatedY = std::abs(offset.y) > halfCard.y + halfWidth * std::abs(sinAngle) + halfHeight * std::abs(cosAngle);
            if (isSeparatedX || isSeparatedY) {
                continue;
            }

            // 散らばる向きは、カードの中心から破片の中心へ向かう向きに、乱数で少しばらつきを足す
            float baseAngle = (offset.LengthSquared() > 1.0f) ? std::atan2(offset.y, offset.x) : fallbackAngle;
            float angle = baseAngle + angleNoise;

            ShardPiece& piece = shardPieces_[static_cast<size_t>(pieceCount)];
            piece.isActive = true;
            piece.offset = offset;
            piece.size = size;
            piece.scatterDirection = { std::cos(angle) * distanceRatio, std::sin(angle) * distanceRatio };
            piece.scatterRotation = rotationRatio;
            ApplyShardPiece(pieceCount);
            ++pieceCount;
        }
    }
}

void UpgradeCardView::ApplyShardPiece(int32_t index) {
    size_t pieceIndex = static_cast<size_t>(index);
    const ShardPiece& piece = shardPieces_[pieceIndex];

    // 画像は正方形なので、UVの回転でゆがまない。破片の中心が、画像の中のどこにあたるか
    float imageSize = kImageSize.x;
    Vector2 uvCenter = { piece.offset.x / imageSize + 0.5f, piece.offset.y / imageSize + 0.5f };

    // UVは「拡大縮小 → 回転 → 移動」の順に掛かる。拡大縮小と回転は、破片の中心を基準にできないので、
    // 中心のずれ（回転した後の半分の大きさ）を、移動から引いておく
    float cosAngle = std::cos(shardAngleRadians_);
    float sinAngle = std::sin(shardAngleRadians_);
    Vector2 halfExtent = { (cosAngle * piece.size.x - sinAngle * piece.size.y) * 0.5f / imageSize, (sinAngle * piece.size.x + cosAngle * piece.size.y) * 0.5f / imageSize };
    Transform uv = {
        { piece.size.x / imageSize, piece.size.y / imageSize, 1.0f },
        { 0.0f, 0.0f, shardAngleRadians_ },
        { uvCenter.x - halfExtent.x, uvCenter.y - halfExtent.y, 0.0f } };

    for (Sprite* shard : { plateShards_[pieceIndex].get(), frameShards_[pieceIndex].get() }) {
        shard->size_ = piece.size;
        shard->SetUvMatrix(uv);
    }
}

void UpgradeCardView::CreateTexts(const Font* font, const std::string& name) {
    nameText_ = std::make_unique<ShadowedText>(font, name, 8, 48.0f);
    descriptionText_ = std::make_unique<ShadowedText>(font, "", 24, 26.0f);
    countText_ = std::make_unique<ShadowedText>(font, "", 16, 28.0f);
}

void UpgradeCardView::Start(float delay, float duration) {
    timer_ = -delay;
    duration_ = (std::max)(duration, 0.05f);
    lightTimer_ = 0.0f;
    wasSelected_ = false;
    isAssembled_ = false;
    textAlpha_ = 0.0f;
}

void UpgradeCardView::SetTexts(const std::string& description, const std::string& count) {
    descriptionText_->SetText(description);
    countText_->SetText(count);
}

void UpgradeCardView::Update(float deltaTime, bool isSelected) {
    timer_ += deltaTime;
    isAssembled_ = (timer_ >= duration_);

    // 組み上がり中の破片
    UpdateShards(std::clamp(timer_ / duration_, 0.0f, 1.0f));

    // 光は、選択された瞬間から最初のコマで回り始める
    bool isSelectedNow = isSelected && isAssembled_;
    if (isSelectedNow && !wasSelected_) {
        lightTimer_ = 0.0f;
    }
    wasSelected_ = isSelectedNow;
    if (isSelectedNow) {
        lightTimer_ += deltaTime;
    }

    UpdateCardSprites(isSelectedNow);
    UpdateLightFrame();

    // 組み上がった後に、文字がふわっと出る
    float fade = (std::max)(style_->textFadeTime, 0.001f);
    textAlpha_ = isAssembled_ ? std::clamp((timer_ - duration_) / fade, 0.0f, 1.0f) : 0.0f;
    UpdateTexts(textAlpha_);
}

void UpgradeCardView::UpdateShards(float progress) {
    // 調整値が変わっていたら、破片を割り直す
    if (IsShardLayoutOutdated()) {
        BuildShardLayout();
    }

    // 後半ほどゆっくり寄る。透明な状態から始まり、早めに見え切る
    float inverse = 1.0f - progress;
    float eased = 1.0f - inverse * inverse * inverse;
    float alpha = (std::min)(progress * 2.5f, 1.0f);
    float remain = 1.0f - eased;

    Vector4 plateColor = style_->plateColor;
    plateColor.w *= alpha;
    Vector4 frameColor = style_->frameColor;
    frameColor.w *= alpha;

    for (int32_t i = 0; i < kMaxShards; ++i) {
        size_t index = static_cast<size_t>(i);
        const ShardPiece& piece = shardPieces_[index];
        if (!piece.isActive) {
            continue;
        }

        // 位置は、元の位置から散らばる向きへ離し、傾きは、格子の角度から散らばりの分だけ回す
        Vector2 position = {
            center_.x + piece.offset.x + piece.scatterDirection.x * style_->scatterDistance * remain,
            center_.y + piece.offset.y + piece.scatterDirection.y * style_->scatterDistance * remain };
        float rotate = shardAngleRadians_ + piece.scatterRotation * style_->scatterRotation * remain;

        for (Sprite* shard : { plateShards_[index].get(), frameShards_[index].get() }) {
            shard->position_ = position;
            shard->rotate_ = rotate;
        }
        plateShards_[index]->color_ = plateColor;
        frameShards_[index]->color_ = frameColor;

        plateShards_[index]->Update();
        frameShards_[index]->Update();
    }
}

void UpgradeCardView::UpdateCardSprites(bool isSelected) {
    // 選んでいるカードは、枠を明るい白にして大きくする
    float scale = isSelected ? style_->selectedScale : 1.0f;
    plateSprite_->color_ = isSelected ? style_->plateSelectedColor : style_->plateColor;
    frameSprite_->color_ = isSelected ? style_->frameSelectedColor : style_->frameColor;
    lightSprite_->color_ = style_->lightColor;

    for (Sprite* sprite : { plateSprite_.get(), frameSprite_.get(), lightSprite_.get() }) {
        sprite->scale_ = { scale, scale };
        sprite->Update();
    }
}

void UpgradeCardView::UpdateLightFrame() {
    // 光の経過時間から、コマの番号を選ぶ
    float period = (std::max)(style_->lightPeriod, 0.01f);
    int32_t frame = static_cast<int32_t>(lightTimer_ / period * static_cast<float>(kLightFrames)) % kLightFrames;
    int32_t column = frame % kLightColumns;
    int32_t row = frame / kLightColumns;

    // UVを1コマぶんに縮め、コマの位置へ動かす
    Transform uv = {
        { 1.0f / static_cast<float>(kLightColumns), 1.0f / static_cast<float>(kLightRows), 1.0f },
        { 0.0f, 0.0f, 0.0f },
        { static_cast<float>(column) / static_cast<float>(kLightColumns), static_cast<float>(row) / static_cast<float>(kLightRows), 0.0f } };
    lightSprite_->SetUvMatrix(uv);
}

void UpgradeCardView::UpdateTexts(float textAlpha) {
    // 出るにつれて、下から少し上がってくる
    float rise = style_->textRiseDistance * (1.0f - textAlpha);
    nameText_->Update({ center_.x, center_.y + kNameOffsetY + rise }, *style_, textAlpha);
    descriptionText_->Update({ center_.x, center_.y + kDescriptionOffsetY + rise }, *style_, textAlpha);
    countText_->Update({ center_.x, center_.y + kCountOffsetY + rise }, *style_, textAlpha);
}

void UpgradeCardView::DrawSprites(RenderQueue* renderQueue) {
    // 遅れの間は何も出さない
    if (timer_ <= 0.0f) {
        return;
    }

    // 組み上がり中は破片、組み上がった後は1枚の画像
    if (!isAssembled_) {
        for (int32_t i = 0; i < kMaxShards; ++i) {
            size_t index = static_cast<size_t>(i);
            if (!shardPieces_[index].isActive) {
                continue;
            }
            renderQueue->SubmitSprite(plateShards_[index].get());
            renderQueue->SubmitSprite(frameShards_[index].get());
        }
        return;
    }
    renderQueue->SubmitSprite(plateSprite_.get());
    renderQueue->SubmitSprite(frameSprite_.get());
    if (wasSelected_) {
        renderQueue->SubmitSprite(lightSprite_.get());
    }
}

void UpgradeCardView::DrawTexts(RenderQueue* renderQueue) {
    // 文字は組み上がった後に出る
    if (!isAssembled_) {
        return;
    }
    nameText_->Draw(renderQueue);
    descriptionText_->Draw(renderQueue);
    countText_->Draw(renderQueue);
}
