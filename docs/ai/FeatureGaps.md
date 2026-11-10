# エンジン機能不足の調査記録

報告ルールの正本は[AGENTS.md 第9節](../../AGENTS.md#9-エンジンの機能不足)。この文書は案件の記録に使い、エンジン変更や回避策の実装を許可するものではない。

## 現在の案件

### ラスタの破片描画で光を切れない

| 項目 | 記載内容 |
| --- | --- |
| 案件名・確認日 | 破片(`FractureInstance`)を光なしの平らな色で描けない。2026-11-08 |
| 状態 | 解決済み(エンジン制作者が修正。2026-11-08、コミット`077851e`) |
| 目的・期待動作 | HPアイコンの核が砕けて、欠片が枠の中に積もる([HUD_HpIcon3D.md](../design/HUD_HpIcon3D.md))。UIは2D感を出すため、影を付けずに平らな色で描きたい |
| 現状・根拠 | `SubmitFracture`と`SubmitRuntimeCutFragments`の`Fracture.PS.hlsl`は、`gMaterial.enableLighting`を見ずに`CalculateBRDF`で必ず光を計算する。`IceFracture.PS.hlsl`は`enableLighting`を見るが、縁の光と疑似SSSが残る |
| 結論 | エンジン対応が必要と推定(共通シェーダーの変更) |
| 不足内容 | `enableLighting == 0`のとき、`gMaterial.color * textureColor * input.color`をそのまま出す分岐(`Object3d.PS.hlsl`と同じ扱い) |
| Application側の案 | 欠片を1個ずつのモデル(`hpCoreShard0`〜`7.gltf`)に分け、光を使わない`SubmitInstancing`で描く。欠片の位置の表(`HpShardShapes.h`)が要る。描画は欠片1種につき1回(計8回)。一度実装したが、人間の判断で取りやめた |
| エンジン側の案 | `Fracture.PS.hlsl`に5行ほどの分岐を足す。今このゲームでラスタの破片描画を使う箇所はないため、今の見た目への影響はない。実際には変更しない |
| 暫定難易度 | 低。分岐の追加だけ。ただし、ラスタの`SubmitFracture`はこのゲームで未使用の経路なので、動作の確認が要る。AI評価であり担当者の見積もりではない |
| 制作への影響 | 修正が反映されるまで、HPアイコンの欠片だけ陰影付きで描かれる(動作は止まらない) |
| 人間の判断 | 2026-11-08、最初はApplication側の案で実装したが、同日「破片個別モデルを消して一つのモデルを破片にする形で実装して」で取りやめ、8個のモデルと`HpShardShapes.h`を削除した。`hpCoreShard.gltf`1つと`FractureInstance`で描く。エンジン制作者に修正を頼み、対応してもらえることになった(未反映)。反映を前提に、破片モデルのマテリアルの光を切って実装した |
| 担当者への相談 | 相談した(人間が送った) |
| 検証結果 | 修正後の`Fracture.PS.hlsl`が`enableLighting == 0`で光を計算しないことをソースで確認。修正後のエンジンでDebugビルドは成功。画面で欠片が平らな色になるかは、人間の確認待ち |

送る文(人間が制作者へ送る):

> `Resources/Shaders/Fracture.PS.hlsl`(ラスタの`SubmitFracture`と`SubmitRuntimeCutFragments`で使う)は、`gMaterial.enableLighting`を見ずに必ずライティングするため、破片を光なしの平らな色で描けません。
> `Object3d.PS.hlsl`と同じように、`enableLighting == 0`のときは`gMaterial.color * textureColor * input.color`をそのまま出すようにしていただけないでしょうか。

### ラスタの破片描画で、渡したマテリアルが使われない

| 項目 | 記載内容 |
| --- | --- |
| 案件名・確認日 | `SubmitFracture`の`material`引数が描画に届かない。2026-11-08 |
| 状態 | 調査済み・判断待ち(Application側の代わりの方法で実装済み) |
| 目的・期待動作 | HPアイコンごとに欠片の色を変えたい(砕けた核だけを明→暗にする) |
| 現状・根拠 | `RenderQueue::SubmitFracture`は`request.material`を記録するが、`SceneRenderManager::Execute3dRequest`の`Draw3dType::Fracture`は`ModelRenderer::DrawFracture`へ`material`を渡さず、モデル自身のマテリアルが使われる(`DrawFracture`自体は`material`引数を受け取れる) |
| 結論 | エンジン対応が必要と推定 |
| 不足内容 | `Draw3dType::Fracture`で`request.material`を`DrawFracture`へ渡す |
| Application側の案 | 破片モデルのマテリアルの色を毎フレーム書き換える。色は全アイコンで共通になり、直近に砕けた核に合わせて変わる(採用。人間の判断、2026-11-08) |
| エンジン側の案 | `SceneRenderManager.cpp`の1行で`request.material`を渡す。今このゲームでラスタの破片描画を使う箇所はないため、今の見た目への影響はない。実際には変更しない |
| 暫定難易度 | 低。引数を1つ渡すだけ。AI評価であり担当者の見積もりではない |
| 制作への影響 | なし(色が共通になるだけ) |
| 人間の判断 | 未決定(制作者へ伝えるかどうか) |
| 担当者への相談 | 未判断。送る場合の文は下記 |
| 検証結果 | 未検証 |

送る場合の文:

> `SceneRenderManager::Execute3dRequest`の`Draw3dType::Fracture`で、`request.material`が`ModelRenderer::DrawFracture`へ渡されていないため、`SubmitFracture`で指定したマテリアルが使われません。`DrawFracture`の最後の引数に`request.material`を渡していただけないでしょうか。

## 記録方法

実際の案件が発生し、記録を保存する許可がある場合に、下記項目を案件ごとに追加する。空の案件を量産しない。

| 項目 | 記載内容 |
| --- | --- |
| 案件名・確認日 | 何について、いつ調べたか |
| 状態 | 未確認 / 調査済み・判断待ち / 採用した対応 / 解決済み |
| 目的・期待動作 | 人間が決めた仕様と、成立させたい条件 |
| 現状・根拠 | 確認したAPI、ソース、使用例、再現条件、未確認事項 |
| 結論 | 既存機能で解決 / Application実装で対応 / エンジン対応が必要と推定 / 判断不能 |
| 不足内容 | 必要な入力・出力・動作条件 |
| Application側の案 | 実現範囲、制約、追加の複雑さ、性能面の懸念 |
| エンジン側の案 | 想定する実装内容・変更領域・既存機能への影響。実際には変更しない |
| 暫定難易度 | 低・中・高、理由、検証負担、不確実性。AI評価であり担当者の見積もりではない |
| 制作への影響 | 止まる作業、すでに許可された独立作業 |
| 人間の判断 | 未決定 / 採用案 / 保留 / 仕様変更等。判断日と理由 |
| 担当者への相談 | 未判断 / 相談する / 相談しない。送信は別途明示的な指示が必要 |
| 検証結果 | 実施内容、結果、未検証範囲 |

元の調査結論と、その後の人間の判断を区別する。解決後は対応と検証結果を追記し、未検証のまま解決済みにしない。
