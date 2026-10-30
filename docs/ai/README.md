# AI資料の入口

この資料群はAquaEngine_Game2の制作を支援する。ルールの正本は[AGENTS.md](../../AGENTS.md)。最初から全資料を読まず、現在の依頼に必要な資料だけを参照する。

## 作業別の参照先

| 目的 | 参照先 |
| --- | --- |
| 作業範囲・禁止事項・MyAI・授業条件・Git操作の確認 | [AGENTS.md](../../AGENTS.md) |
| エンジンの基本構造と調査先 | [AquaEngineNotes.md](AquaEngineNotes.md) |
| エンジンの機能不足を調査・報告する | [FeatureGaps.md](FeatureGaps.md) |
| 実装前の整理、段階実装、検証、コードレビュー | [ImplementationReview.md](workflows/ImplementationReview.md) |

## 資料の状態

- 2026-10-30に、会話で合意したAI運用方針と、公式ガイド・手元のソースの調査内容を整理した。
- ゲームの具体的な仕様・構造設計は、この資料群で確定していない。
- 人間が採用した設計を保存する場合は`docs/design/`を使う。現時点では作成していない。
- MyAIの本文、個人の絶対パス、会話ログ、授業画像の複製は含めない。
- AI生成アセットはAI資料に置かず、エンジンの読み込み仕様に沿って`Project/Resources/`へ置く。

## 保存とGit

この資料群はGit登録対象。ただし保存許可とステージング・コミットの許可は別である。個人メモ・PC固有の接続情報は、必要時にGit対象外の`.ai-local/`を使う。認証情報は利用ツールの認証管理先を使う。

内容の更新時は、確認済み事実・推測・未決定事項・人間の採用判断を区別する。ルール変更はAGENTS.md、技術知識はAquaEngineNotes.md、機能不足の案件はFeatureGaps.mdへ反映し、本文を重複管理しない。
