# フィールドの見た目(A2)

確定日: 2026-11-18。見た目は案B「夜の雪原と氷柱」。ルールに左右される部分(円、半径50、押し戻し)は「今のまま仮置き」で、変えない。

## 動き(ルール)は変えない
- 円形、半径50、端で押し戻して滑る、障害物なし。プレイヤーと敵が同じ範囲。`Field::ClampToField`と敵側の押し戻しには触らない。
- 障害物、当たり判定、フィールドの形の変更、フェーズの進み具合による見た目の変化は、このタスクでやらない。

## 構成
| 部品 | 場所 | 内容 |
| --- | --- | --- |
| 台地 | `Field` | 地面の円柱を厚さ12にして台地にする。上面はy=0。側面が崖になる。 |
| 縁 | `Field` | 台地の上の端の薄い段(`rimThickness_`、既定1.0)。色は淡い氷色。 |
| 雪原と氷柱 | `FieldScenery`(新設) | 円の外の見た目だけ。当たり判定なし。 |
| 空 | `iceNightSky.dds` | 夜空のキューブマップ。 |
| ライト | `GameScene`の`Light/lightColor` | 冷たい光 `(0.85, 0.92, 1.0)`。 |
| フォグ | `PostEffectGraph.json` | 距離フォグ。全シーンに効く。 |

## Field(初期値と形)
- 地面: 色 `(0.32, 0.38, 0.48)`、粗さ0.9、金属感0。厚み`groundThickness`=12。
- 縁: 色 `(0.45, 0.6, 0.8)`、粗さ0.3。`rimThickness`=1.0、上面は-0.05。
- 粗さと金属感は`DebugParameter`(グループ`Field`)に出している。`GetGroundThickness()`、`GetRimWidth()`を追加した。

## FieldScenery
- `GameScene::CreateGameObjects`で`Field`の直後に作る。`Field`と同じ管理クラスが持つので、ポインタはシーンが終わるまで有効。
- 雪原: `planeXZ.obj`(一辺2)を`半径×snowScale(6)`の拡大率で、台地の底(y=-厚み)に置く。色 `(0.18, 0.22, 0.3)`、粗さ0.95。
- 氷柱: `RegisterCylinderModel("IcePillar", 6, 0.6, 1.0, 1.0)`(上が細い六角柱)。`ModelComponent`は最大64本(`kMaxPillars`)を最初に作り、描く本数`pillarCount`(既定40)だけ更新と描画をする。遊んでいる途中で破棄しない。
- 並べ方(`Rebuild`): シード(既定2026)の乱数で、角度は等間隔に、間隔×`angleJitter`のずらしを足す。中心からの距離は`半径+縁の幅+bandInner(2)+乱数×bandWidth(6)`。底は雪原に置き、台地の上に出る高さ1.5〜6、太さ0.6〜1.5倍、外向きに最大15°傾け、Y回転はランダム。
- 高さを抑える理由: 手前(南)の端で自機を隠さないため(カメラ60°、距離25では、縁から4外の柱は約6.9の高さまで視線を遮らない)。
- 色 `(0.55, 0.75, 0.95)`、粗さ0.15、金属感0.1(レイトレーシングで映り込む)。
- 調整値は`DebugParameter`のグループ`FieldScenery`に全て出している。変更があったとき、または`Field`の半径・厚み・縁の幅が前回と違うときだけ並べ直す。

## 空
- `Project/Resources/Textures/Bg/iceNightSky.dds`。各面64px、`R16G16B16A16_FLOAT`、DX10ヘッダー(TEXTURECUBE、arraySize 6)、ミップ1つ、面の順は+X,-X,+Y,-Y,+Z,-Z。
- 色(リニア): 下 `(0.02, 0.03, 0.06)`、地平線 `(0.35, 0.42, 0.55)`、上 `(0.05, 0.08, 0.16)`。上下の方向でなめらかに補間する。
- 制作job(TD2_2): `C:\Users\syobo\nihonkougakuinn\InBox\TD2_2\AIRecords\AssetWork\Jobs\a2f1d0c3b7e84a9d8c2e5f61a3b79d04\`。`sources/makeSkyCube.mjs`(先頭の定数で色を変える)を`node sources/makeSkyCube.mjs`で実行し、`exports/iceNightSky.dds`を出す。ゲームのリポジトリにスクリプトは置かない。
- `GameScene`の`skyboxTextureHandle_`を`iceNightSky.dds`にした。元の`qwantani_moon_noon_puresky_1k.dds`は残している。

## ポストエフェクト
- `PostEffectGraph.json`(全シーン共通。C5のタイトルにも効く)にFog(id 30)を足し、Scene→Fog→HighLumMask、Fog→Bloomのシーン入力、の順につないだ。`slot`は受け側の入力番号。
- Fog: 色 `(0.08, 0.11, 0.2)`、startDistance 30、density 0.08、maxOpacity 0.95、skyOpacity 0.6。台地の上はカメラから約23〜30、下の雪原は約35以上で紺に沈む。他のノードの値は変えていない。

## 分かっている小さな問題(直していない)
- 氷塊・破片の地面はy=0の固定なので、円の外へ飛んだものは空中で跳ねる。
