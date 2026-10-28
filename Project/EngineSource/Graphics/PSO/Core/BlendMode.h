#pragma once

namespace GameEngine {

    enum BlendMode {
        // ブレンドなし
        kBlendModeNone,
        // 通常aブレンド Src * SrcA + Dest + (1-SrcA)
        kBlendModeNormal,
        // 加算 Src * SrcA * Dest * 1
        kBlendModeAdd,
        // 減算 Dest * 1 - Src * SrcA
        kBlendModeSubtract,
        // 乗算 Src * 0 + Dest * Src
        kBlendModeMultily,
        // スクリーン Src * (1-Dest) + Dest * 1
        kBlendModeScreen,
        // オブジェクトの透明度を保存する通常aブレンド
        kBlendModeNormalAndSaveObjectAlpha,
        // オブジェクトの透明度を保存する加算ブレンド
        kBlendModeAddAndSaveObjectAlpha,

        kBlendModeWboitAccumulation,
        kBlendModeWboitRevealage,
       
        // カラー書き込み無効
        kNoBlend,

        // 利用禁止
        kCountOfBlendMode,
    };

    enum DrawModel {
        FillFront, // 中身を描画
        FrameFront, // フレームを描画
        FrameBack, // 後ろ側を描画
        None, // 両面描画

        kCountOfDrawMode,
    };

}