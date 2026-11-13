# A7 HIT表示の見た目(U2)

確定日: 2026-11-13。ユーザーの言葉: 「とりあえず確定。HIT!は一番気持ちいいUIにしたいから、もっと演出を凝る可能性はある」。
今後の演出追加に備え、表示を作る・動かす・置く処理を関数に分けておく(先回りの抽象化はしない)。

## 見た目(`ComboPopupView`)

- 数字: 大きな白い数字。数が多いほど大きい(`baseSize`/`growPerHit`/`maxSize`)、増えた瞬間に弾む(`punchScale`/`punchTime`)、一定時間で消える(`holdTime`/`fadeTime`)は、数字の大きさとして従来のまま使う。
- 「HIT!」: 数字の右下に小さな水色(0.4 / 0.8 / 1.0。HPアイコンの核と同じ)。大きさは数字の大きさに対する割合(`hitSizeRatio`)。数字の右下の角から「HIT!」の左下の角へのずらしも、数字の大きさに対する割合(`hitOffsetRatio`)。数字の大きさは`Text::GetSize()`で取る。
- 縁取り(`OutlinedText`): 数字と「HIT!」の両方に付ける。同じ文字を暗い紺色で円周上の8方向にずらして重ね、右下に影を1つ付ける。縁の太さ(`outlineRatio`)と影のずれ(`shadowRatio`)は文字の大きさに対する割合。
- 氷の破裂: 数字の後ろに、氷が砕けた形の画像(`ComboBurst.png`)を水色の半透明で置く。大きさは数字の大きさに対する割合(`burstSizeRatio`)。数が増えた瞬間に、数字の弾みと同じタイミングで大きく弾み(`burstPunchScale`)、向きがシード固定の乱数で前の向きから少し変わる(`burstAngleRange`)。向きは弾みの間に回って変わる。表示が消えるときは一緒に消える。スプライトは文字より下に描かれる(エンジンの保証ではなく、現在の描画の順)。
- 色: 数字、「HIT!」、縁、影、破裂の色は`DebugParameter`(`numberColor`/`hitColor`/`outlineColor`/`shadowColor`/`burstColor`)。

## 画面の端

- 毎フレーム(`UpdatePopup`)、表示の外形(数字、「HIT!」、縁と影、破裂、弾みで大きくなった分)を数字の中心からの相対位置で測り、全体が画面の内側に収まる位置へ寄せる。
- 画面の外で砕いた敵の場合も表示は出し、その敵にいちばん近い内側の位置に置く。
- `screenMargin`は「表示の外側から画面の縁までの距離」(初期値は仮に16px)。旧仕様(文字の中心から端までの距離。80px)から意味が変わった。
- 外形が画面より大きいときは、その軸は画面の中央に揃える。

## 変えないこと

1つの連鎖につき表示1つ(新しく砕けた敵の位置へ移る)、同時に出せるのは8つまで、氷霧で倒した分は数えない、`SetVisible`、動きはゲームの時間で進める。

## 画像

- `Project/Resources/Textures/UI/Combo/ComboBurst.png`: 256×256、白一色。名前は`Project/Resources`全体で重ならないことを確認済み(`Effect/HitEffect.png`とは別)。
- TD2_2の`AIRecords/AssetWork/Jobs/c4b8d2e6a1f94c7e8d305b92e7f61a3d/sources/build_combo_burst.py`でBlender(CLI)から作る。乱数のシードは固定。
- 画像は大きくしない(エンジンは起動時にPNGをCPUでBC7へ圧縮する)。中身が変わらない画像は書き直さない(キャッシュが作り直しになる)。

## 実装の構成

- `OutlinedText`(`Project/Application/Game/UI/`): 影1つ+縁8つ+本体の`Text`を持ち、位置・大きさ・色を反映して、影、縁、本体の順に描画キューへ出す。
- `ComboPopupView`: 表示1つが`OutlinedText`2つ(数字、「HIT!」)と`Sprite`1つ(破裂)を持つ。8表示で`Text`は160個になる(起動時に作り、使い回す)。
- `GameScene::LoadAssets`が`ComboBurst.png`のハンドルを取り、`ComboPopupView`へ渡す。

## 未確認(人間が触って決める)

数字と「HIT!」の位置関係、縁取りの見た目(角ばり)、破裂の弾みと回り方、画面の端と画面の外で砕いたときの見切れ、8つ同時に出たときの重さ。
