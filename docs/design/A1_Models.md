# 自機と敵のモデル(A1)

確定日: 2026-11-11。ユーザーの言葉は「確定」(設計子a1-designと決めた設計)。
実装日: 2026-11-11(実装子)。ビルドまで確認済み。起動しての確認は、人間が行う。
仕様の正本は[GameSpec.md](GameSpec.md)、見た目の基準は[ArtDirection.md](ArtDirection.md)。

## 1. 自機(`PlayerView`)

- 形: 宙に浮いた正八面体の核(`hpCore.gltf`)と、それを囲む六角形の枠2つ(`hpFrame.gltf`)。新しいモデルは作っていない。HUDとは別の`ModelComponent`なので、HUDの色に影響しない。
  - 枠は2つとも水平(XZ平面)に寝かせた向きが初期(`hpFrame`の輪はXY平面なので、X軸まわりに90度回してから回す)。枠AはワールドのX軸、枠BはZ軸のまわりに回る。大きさは1.0倍と1.2倍。
  - 核は回さず、揺らさない。
- 質感と描画: ふつうのPBR(`IceMaterial`は使わない)。描画は従来の自機と同じ`DrawRaytracing`。色は核と枠で別の調整値(初期値はどちらも同じ水色`(0.4, 0.8, 1.0)`)。
- 浮かせ方: 描く高さだけを`liftHeight`上げる。`Player`の位置・当たり判定は変えない(`Draw`の時点の位置で行列を作るので、押し戻しも反映される)。見た目の基準の大きさは`viewScale`(自機の半径`radius`とは別)。
- 回転の速さ(倍率を目標へ指数で滑らかに近づける。追いつく速さ`spinFollowSpeed`):
  - チャージ: `chargeTimer_ / maxChargeTime_`で、1倍から`minChargeSpinScale`(フルチャージ)へ補間する。
  - 連鎖: 氷塊が1つでも飛んでいる(`kFlying`)間は`chainSpinScale`倍。チャージと重なったら掛け合わせる。
- 氷霧(`Player::FireIceMist`から`PlayPulse`): 核と枠をまとめて、1 → `pulseScale` → 1倍(sinの山)。時間は`pulseTime`。
- 点滅: 無敵の点滅は`Player::Draw`で3つまとめて飛ばす。回転・拡縮はゲーム時間(`FpsCounter::GetGameDeltaTime`)で動くので、ゲームオーバー中は止まる。

### 構造

| クラス | 役割 |
| --- | --- |
| `PlayerView`(新規) | 核と枠2つの`ModelComponent`、回転・拡縮の状態、調整値、描画。`IceAreaView`と同じ形(`Update` / `Draw` / `PlayPulse`) |
| `PlayerViewAssets`(新規) | 核と枠のモデル。`GameScene::LoadAssets`が`hpHudModels_`と同じモデルを渡す |
| `Player` | 変更。`PlayerView`を所有し、チャージの進み具合・氷霧の発生・連鎖中を渡す。コンストラクタは`Model*`の代わりに`PlayerViewAssets`を受け取る。旧`color_`と自機の`ModelComponent`は削除した |
| `IceShardManager` | 読み取り関数`GetFlyingCount()`を1つ追加 |
| `GameScene` | `StepIceShards`で`player_->SetFlyingShardCount(iceShardManager_->GetFlyingCount())`を毎フレーム渡す。`Player`は`IceShardManager`を知らない |

### 調整値(`DebugParameter`の`PlayerView`。初期値はすべて仮)

| グループ | 値と初期値 |
| --- | --- |
| Layout | `viewScale`0.6、`liftHeight`0.6、`frameScaleA`1.0、`frameScaleB`1.2 |
| Spin | `spinSpeedA`1.5、`spinSpeedB`1.1(ラジアン/秒)、`minChargeSpinScale`0.15、`chainSpinScale`2.5、`spinFollowSpeed`6 |
| Pulse | `pulseScale`1.3、`pulseTime`0.25(秒) |
| Color | `coreColor`、`frameColor` |

- `Player`の`DebugParameter`の`color`は使わなくなった(保存済みJSONにあれば無視される)。

## 2. 敵のモデル

- 制作: TD2_2の`AIRecords/AssetWork/Jobs/a1e4e7b3c2d94f10b8a6c5d7e9f01234/`。`sources/build_enemies.py`(BlenderをCLIで実行。MCPは使っていない)が、5種の`.gltf`とプレビュー画像を作る。`exports/enemy/`に出力、`sources/enemy/enemies.blend`に元データ。
- 置き場所: `Project/Resources/Models/Enemy/`(`enemyChase` `enemyFast` `enemyShooter` `enemyBeam` `enemyOmni`の`.gltf`と`.bin`)。`enemy`で始まる素材名は他にない(確認済み)。
- 共通: 白一色(材質`enemyWhite`)、テクスチャなし、1体1メッシュ、面は平ら、半径1の球の中、底(最下点)はモデルのy=-1(球の底の極に触れる)、前はエンジンの+Z(Blenderの-Y)で全種類そろえた。スクリプトが半径と底を検査する(違えば止まる)。

| 種類 | モデル | 形 | 三角形 |
| --- | --- | --- | --- |
| 迫る(chase) | `enemyChase` | 熾火の塊。低ポリゴンの球の頂点をランダムに内側へずらしたでこぼこ | 80 |
| 速い(fast) | `enemyFast` | 彗星型。丸い頭と、後ろ(+Y側)へ伸びる尾3本 | 266 |
| 射撃 | `enemyShooter` | 砲台虫。丸い胴体、前に短い円柱の砲口1本、左右に短い脚 | 300 |
| ビーム | `enemyBeam` | 一つ目。球の前面がくぼみ、中に円いレンズ、縁に輪 | 1054 |
| 全方位 | `enemyOmni` | 太陽型。胴体の横まわりに円錐のトゲ10本(1本が前) | 324 |

- `EnemyList.json`は`chase`の`modelName`を`enemyChase.gltf`、`fast`を`enemyFast.gltf`へ変えただけ。他の値は変えていない。射撃・ビーム・全方位のモデルは置いただけ(C1で使う)。氷の殻(`sphere.obj`)と殻の破片(`CellFractureCube8`)は変えていない。
- 向き: `Enemy`が`facingYaw_`を持ち、動いた(`moveDirection_`が0でない)フレームだけ`atan2(x, z)`で更新する。止まっている間と凍結中は直前の向きを保つ。`EnemyManager::Draw`が本体だけY軸まわりにこの角度で回す(殻は球なので向けない)。出現時は0(+Zを向く)。狙う向きはC1で決める。
- 動き(揺れや回転)は入れていない。

## 3. 確かめること(人間)

- 自機の浮き方、枠2つの交差した回転、核との重なり。
- チャージで回転が遅くなる、氷塊が飛ぶと速くなる、重なったときの速さ、氷霧の拡縮、無敵の点滅(3つ同時)。
- 氷塊の所持の塊(頭の上、高さ1.9)と、氷霧で大きくなった枠が重ならないか。
- 敵2種の形と、移動の向きへ向くか。凍ったとき(半径1.4倍の殻)に突き抜けないか。
- 敵が多いときの負荷(1体1メッシュ。三角形は80と266)。
