# HPアイコン(3D)

確定日: 2026-11-08(人間の言葉は「いいと思う!」「実装を開始したいです」)。同日、欠片の描き方を、1個ずつのモデルから破片モデル1つ(`FractureInstance`)へ変えた(人間の言葉は「破片個別モデルを消して一つのモデルを破片にする形で実装して」「エンジンの修正はしてもらえることになったので、まだ反映はされていないけどその前提で進めて」)。仕様の正本は[GameSpec.md](GameSpec.md)、見た目の基準は[ArtDirection.md](ArtDirection.md)。この文書は、C3のHPアイコン(`HpIconView`、丸の画像)を3Dの表示へ置き換える設計の全文。

## 1. 見た目と動き

- 左上に、HPの数だけ六角形の枠を並べる。残っているHPの枠の中では、正八面体の「冷気の核」がまっすぐ立って回る。
- 影(面ごとの明暗)は付けない。光を使わず、平らな色で描く(UIの2D感を残すため)。色はすべてコードで掛け、`DebugParameter`で調整する。
- 被弾すると、減った分の核が8個の欠片に砕ける。欠片は白く光らせず、核の色から失った色へ、だんだん暗くなる(`shatterTime`)。枠も同じ時間で、失った色になる。
  - 欠片の色は全アイコンで共通。直近に砕けた核に合わせて変わるので、前に積もった欠片も、新しく砕けた瞬間だけ一緒に明るくなってから暗くなる(人間の判断、2026-11-08)。
- 欠片は枠の中だけで物理的に動く。重力で落ち、六角形の内側の辺と、欠片どうしで跳ね返り、底に積もって残る。これが失った状態の見た目。
- 演出は止まらない時間で進める。ゲームオーバー中も欠片は動く。

## 2. クラスと依存

| クラス | 役割 |
| --- | --- |
| `GameHud` | 変更。`HpIconView`の代わりに`HpHudView`を所有する。コンストラクタでカメラ、モデル、白いテクスチャを受け取る |
| `HpHudView` | 新規。枠・核・欠片の置き方、色の変化、描画。`SetHp`、`PlayDamage`、`Update`、`Draw`の呼ばれ方は旧`HpIconView`と同じ |
| `HpHudModels` | 新規の構造体。枠・核・破片のモデル |
| `HpShardPile` | 新規。アイコン1つ分の、欠片の2Dの物理 |

- `GameScene::LoadAssets`がモデル(`hpFrame.gltf`、`hpCore.gltf`、`hpCoreShard.gltf`)を取得し、`GameHud`へ`gameCamera_->GetCamera()`と`whiteTextureHandle_`と一緒に渡す。
- 旧`HpIconView`と、HPにしか使っていなかった`circle.png`の取得は削除した。
- `HpHudView`は`Camera`を読むだけで、ゲームのクラスを知らない(`ComboPopupView`と同じ)。

## 3. 画面への置き方

- 画面の位置(px)を、`Camera::GetVPMatrix()`の逆行列でワールドへ戻し、カメラの前向きに`distance`だけ離れた所へ置く。カメラのすぐ前なので、ゲーム世界の物に隠れない(ニアクリップは0.1)。
- アイコンの高さ(px)は、画面の2点をワールドへ戻した距離で、ワールドの大きさに直す。視野角は使わない。
- 向きは、カメラの右・上・前の向き(ビュー行列の逆行列)をそのまま使い、画面と平行にする。
- 核は、HUDの面の上で、画面の縦の軸(モデルのY軸)まわりに回す。

## 4. 描画

- 枠と核: 氷ゲージ(`EnemyGaugeView`)と同じ`SubmitInstancing`。このシェーダー(`Particle.PS.hlsl`)は光を使わず、インスタンスごとの色をそのまま出す。モデルごとに1回の描画。
- 欠片: 破片モデル`hpCoreShard.gltf`(`_cell`の破壊グループが1つ、8欠片)を、アイコンごとの`FractureInstance`で`SubmitFracture`する。欠片の中心と大きさは、読み込み時の`FractureChunkInfo`(`centroid`、`aabb`)から取る。
  - 破片の描画は、`SubmitFracture`で渡したマテリアルを使わず、モデル自身のマテリアルを使う(`SceneRenderManager`が渡していない)。そのため、モデルのマテリアルの光を切り(`SetEnableLighting(false)`)、色は毎フレームそのマテリアルへ書く。欠片の色が全アイコンで共通なのはこのため。
  - `Fracture.PS.hlsl`は、エンジン制作者の修正(2026-11-08、コミット`077851e`)で、`enableLighting`が0なら光を計算しなくなった([FeatureGaps.md](../ai/FeatureGaps.md))。

## 5. 欠片の物理(`HpShardPile`)

- 枠の面の上の2D(核のモデル座標のX・Y、モデル単位)で計算する。奥行き(Z)は、枠の面へ寄せていく。
- 割れた瞬間: 核の今の回転で、欠片を割れる前の位置に置く。中心から外向き(`burstSpeed`)と上向き(`burstUp`)に飛ばし、ランダムに回す(`maxAngularSpeed`)。
- 毎フレーム: 重力(`gravity`)で落とす。六角形の内側の6本の辺と、欠片どうしを、円として当てる(半径は欠片の大きさ×`radiusScale`)。跳ね返り(`restitution`)、辺での摩擦(`friction`)、回転の減り方(`angularDamping`)を付ける。触れていて十分に遅い欠片は止める(`sleepSpeed`)。
- 回転は見た目だけなので、オイラー角を一定の速さで進める(`FractureChunkState`がオイラー角で持つため)。
- 1フレームは2回に分けて計算し、当たりは4回くり返して解く。1回で進める時間は1/30秒まで(すり抜けを防ぐ)。
- HPが増えたら、核を戻して欠片を消す(今の仕様に回復はないが、値の整合を保つため)。初回は演出せず、失った分は暗い枠だけにする。

## 6. 調整値(`DebugParameter`の`HpHudView`)

| グループ | 値 |
| --- | --- |
| Layout | `origin`、`iconSize`(枠の高さのpx)、`spacing`、`distance`、`spinSpeed` |
| Color | `coreColor`、`frameColor`、`lostCoreColor`、`lostFrameColor`、`shatterTime` |
| Shard | `gravity`、`burstSpeed`、`burstUp`、`maxAngularSpeed`、`angularDamping`、`restitution`、`friction`、`radiusScale`、`depthSettle`、`sleepSpeed` |

初期値は仮。実装後に触って調整する。

## 7. 素材

- `Project/Resources/Models/HpHud/`: `hpFrame.gltf`(枠)、`hpCore.gltf`(核)、`hpCoreShard.gltf`(核を8個に分けた破片モデル。ノード名は`HpCore_cell`〜)。どれも白一色で、テクスチャはない。
- 制作元: TD2_2の`AIRecords/AssetWork/Jobs/bd0dc7f6181745e789b2c887f6a74d6e/`(`sources/build_hud.py`、`sources/hud/hpHud.blend`)。
- 一時的に使った「欠片1個ずつのモデル(`hpCoreShard0`〜`7.gltf`)と欠片の表(`HpShardShapes.h`)」は削除した。

## 8. 確かめること

- 割れる前の8個の欠片が、核とぴったり重なるか。
- 欠片が、枠や核と同じ平らな色で描かれるか。
- ポストエフェクトがHUDにかかるか。画面の隅で、透視による傾きが目立つか。
- 見た目と動きの採否は人間が決める。
