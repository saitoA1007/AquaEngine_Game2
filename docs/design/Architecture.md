# 構造設計

最終更新: 2026-11-08。仕様の正本は[GameSpec.md](GameSpec.md)。

- 人間が採用した構造だけを書く。未決定のものは「未決定事項」に分ける。
- 実装の許可は別途。この文書の存在は実装の許可を意味しない。

## 1. クラスの役割

| クラス | 役割 |
| --- | --- |
| `GameScene` | 全体の組み立て、入力の登録。`GameCamera`を持ち、更新後のプレイヤー位置を渡す |
| `GameCamera` | ゲーム専用のカメラ。距離・俯角・追従率の調整値と指数補間を持ち、ターゲットを追従する固定角度の見下ろしカメラとして`Camera`を更新する。`IGameObject`ではなく`GameScene`が所有する |
| `Player` | 移動、HP、チャージ、氷エリアと氷霧の発生 |
| `EnemyManager` | 全敵の更新、描画、凍結、破砕、スポーン。`Player`を知らず、`EnemyInput`を受け取って`EnemyOutput`を返す |
| `EnemyGrid` | 敵の位置を入れる均一グリッド。範囲検索を提供する。`EnemyManager`が所有し、B3の氷霧とB4の連鎖も同じ検索を使う |
| `EnemyGaugeView` | 凍りかけの敵の頭に出す氷ゲージの仮表示。平面をカメラと同じ向きに傾けてラスタのインスタンシングで描く(背景と塗りで2回の描画)。`EnemyManager`が所有し、差し替え可能。表示数の上限は既定128(近い敵を優先)。全員表示ではフレームのスパイクが出た |
| `IceAreaView` | 氷エリアの範囲を示す仮の描画。`Player`が所有し、差し替え可能。氷の煙パーティクルへ置き換える予定 |
| `EnemyIO.h` | `EnemyInput`(プレイヤーの位置、半径、氷エリア半径)と`EnemyOutput`(受けたダメージ、プレイヤーへの押し戻し量)をまとめた構造体のヘッダー |
| `IceShardManager` | 氷塊のドロップ、回収、連鎖の発射 |
| `PhaseDirector` | 経験値、レベル、フェーズ、未処理のレベルアップ数、フェーズ表。`IGameObject`で毎フレームの処理は持たない。ボス開始の通知を出す。`PlayerStats`と強化画面は知らない |
| `PhaseTable` / `PhaseTableIO` / `PhaseTableEditor` | フェーズ表の構造体、JSONの読み書きと検証、ImGuiの編集画面(`USE_IMGUI`のみ) |
| `PlayerUpgrade` | 強化の3項目の名前と説明、1回あたりの上昇量(`DebugParameter`)、取った回数、`PlayerStats`への反映。`GameScene`が所有する |
| `UpgradeSelectUI` | 強化選択の画面。`IGameObject`。止まらない時間で動き、決定をコールバックで返す |
| `GameHud` | 常時表示のUI(HPアイコン、フェーズ)。`IGameObject`。ゲームのクラスを知らず、`GameScene`から`HudInput`を受け取って表示する。`HpHudView`を所有する。止まらない時間で動く |
| `HudInput` | `GameHud`へ渡す表示用の値(HP、最大HP、フェーズ)の構造体 |
| `HpHudView` / `HpHudModels` | 3DのHPアイコン(六角形の枠と回る核)と被弾の演出(核が砕けて欠片が枠に積もる)。画面に固定するため`Camera`を読む。枠と核は`SubmitInstancing`、欠片は破片モデルとアイコンごとの`FractureInstance`で描く。`GameHud`が所有する([HUD_HpIcon3D.md](HUD_HpIcon3D.md)) |
| `HpShardPile` | アイコン1つ分の欠片の2Dの物理。`HpHudView`が所有する |
| `GameOverView` | 「GAME OVER」と「Aで再開」の表示、再開の入力(`Retry`)。`IGameObject`。止まらない時間で動く |
| `ComboPopupView` | 連鎖数の表示(「HIT 12!」)。氷塊で倒したときだけ数える。発射元の氷霧のIDごとに1つ、同時に最大8つ。`IGameObject`。ゲームの時間で動く |
| `EnemyShotManager` | 敵の弾とビーム。敵とは独立して動く |
| `ShatterEffectManager` | 破片の`FractureInstance`の事前確保と使い回し、飛ばす処理と描画 |
| `PlayerStats` | 移動速度、範囲の倍率、威力の倍率を持つ構造体。`GameScene`が持ち、強化前の移動速度(調整値)と`PlayerUpgrade`の回数から毎フレーム求め直す |

## 2. 敵の構造

- `IGameObject`になるのは`EnemyManager`だけ。`EnemyManager`は`GameObjectManager`を通して生成する。
- 個々の敵は`IGameObject`ではない普通のクラスで、`EnemyManager`が配列で持つ。
- 種類ごとに上限数を事前に確保して使い回す。上限はV1の負荷テストの結果で決める。
- 基底クラス`Enemy`が共通のデータと凍結の処理を持ち、派生クラスが行動と専用データを持つ。
- 種類ごとにモデルを分けられる。モデルは敵一覧のモデル名で指定する。
- 敵の当たり判定はエンジンの`Collider`を使わない。総当たりのため敵の数に向かない。距離判定と`EnemyGrid`で行う。
- 動いている敵どうしは押し合わない。凍った敵との重なりだけを解消する。
- 負荷の目安(RTX 4060 Laptop、Release、1280×720。プレイ会PCでの値ではない): 通常の敵は4000体まで、凍結は500体まで60fps。
- 破片は1体あたり15個まで。目標は同時50体の破砕。B3の計測(Release、64体を一斉に破砕)で、破片の更新を並列化して25ms超のフレームは減ったが、敵が中心に集まった状態で氷霧を使うと一瞬のカクつきが残る。その状況が起きにくい前提で進める(詳細は[GameSpec.md](GameSpec.md))。
- 個体の見た目は種類ごとに1つのマテリアルを共有し、色は敵一覧の値で決める。

### 基底クラスの流れ

- 凍っていれば残り時間だけ進める。凍っていなければ`UpdateBehavior`を呼ぶ。
- 派生クラスは`OnSpawn`、`UpdateBehavior`、`OnFrozen`、`OnReleased`を実装する。

### 共通データ

| 一覧の値(`EnemyDefinition`) | 個体の値(`Enemy`) |
| --- | --- |
| ID、表示名 | 定義への参照 |
| 派生クラスの種類 | 有効かどうか |
| モデル名、見た目の大きさ | 位置、向き |
| 当たりの半径 | 状態(通常、凍結) |
| 移動速度 | 氷ゲージ |
| 氷ゲージの閾値 | 凍結の残り時間 |
| 接触ダメージ | 最後に凍った氷霧のID |
| 経験値、氷塊の数 | 砕かれた回数 |
| 倒すのに必要な砕く回数 | 見た目のアニメ用の時間 |
| 上限数(事前確保する個体数) | |
| 色 | |
| 追加の値(名前と数値の組) | |

### 派生クラス

| クラス | 対象 | 追加の値 | 個体の値 |
| --- | --- | --- | --- |
| `ChaseEnemy` | 迫る敵、速い敵 | なし | なし |
| `ShooterEnemy` | 射撃の敵 | 発射間隔、弾速、避け始める距離 | 発射タイマー |
| `BeamEnemy` | ビームの敵 | 狙う時間、固定時間、発射時間、クールタイム、ビーム幅 | 状態、タイマー、固定した方向 |
| `WanderEnemy` | 全方位の敵 | 発射間隔、弾数、弾速 | 発射タイマー、移動方向 |
| `BossEnemy` | ボス | ボスの内容を決めてから | ボスの内容を決めてから |

- 種類ごとの固有の調整値は、敵一覧の「名前と数値の組」で持つ。読み込み時に足りない名前があればエラーを出す。

## 3. 敵一覧とフェーズ表

- データはJSONで持ち、`JsonSerializer`で読み書きする。
- 敵一覧の保存先は`Resources/Json/EnemyData/`。フェーズ表の保存先は`Resources/Json/PhaseData/PhaseTable.json`。`Resources/Json/GameData/`は`GameParamEditor`が読むので使わない。
- フェーズ表は10行(5行目と9行目がボーナス、10行目がボス)。フェーズごとに、次のレベルまでの経験値、スポーンの間隔、1回に出す数、フェーズ全体の最大数、敵のIDごとの重みを持つ。敵一覧のIDが重みに存在するかを読み込み時に検証し、失敗したら例外を出す。編集画面で保存すると、今のフェーズの出現ルールにすぐ反映する。
- `USE_IMGUI`の中で、一覧の表示、行の追加と削除、保存ができる画面をアプリ側に作る。エンジンの変更はしない。
- Release構成では読み込みだけが動く。
- プレイヤーなどの単一の値は`DebugParameter`で調整する。

## 3.5 敵の更新の流れ

- 氷ゲージは0から閾値までの単位で、氷エリア内で進み、閾値に達すると凍る。エリア外では徐々に減り、解放時は0に戻す。
- `GameScene::Update`が、更新後の`Player`の値から`EnemyInput`を作り、`EnemyManager`の更新関数へ渡して`EnemyOutput`を受け取り、`Player`へ反映する。構造体はこの関数のローカル変数で、`EnemyManager`は保持しない。
- 敵の更新はこの呼び出しで行い、`EnemyManager`の`IGameObject::Update`では行わない(重複を避ける)。シーン更新はオブジェクト更新の後なので、移動後のプレイヤー位置をそのフレームの敵が使う。
- 毎フレーム、グリッドを作り直し、スポーン、個体の更新(凍結の進行と行動)、凍った敵との重なりの解消、プレイヤーとの処理の順に行う。
- 動く敵が凍った敵を避ける動きは、押し出しと壁の接線方向への滑りで行う。向きは個体ごとに固定する。
- 凍った敵はプレイヤーを押し戻す。動く敵は接触ダメージと無敵時間だけで、プレイヤーを押さない。

## 4. 氷霧の処理

1. `Player`が氷霧の情報(中心、半径、威力)を出す。`Player`は敵の中身を知らない。
2. `EnemyManager`が凍結と破砕を判定する。同じ1発で凍った敵は砕かない。
3. `IceShardManager`が氷塊を飛ばす。

- `Player`はコールバックを呼ぶだけで、`GameScene`がその先で`EnemyManager`と`IceShardManager`を順に呼ぶ。処理の順番はこの呼び出しで決まり、更新順には頼らない。

## 4.5 氷塊の処理

- `IceShardManager`は`IGameObject`で、`GameObjectManager`が所有して描画する。氷塊は512個を事前確保して使い回す。更新は`UpdateShards`で行い、`IGameObject::Update`では行わない(調整値の反映のみ)。
- `EnemyManager`を知っているのは`IceShardManager`だけ。`EnemyManager`は`SetDefeatCallback`で撃破を通知し、`GameScene`が`IceShardManager::Drop`へ結ぶ(循環依存を避ける)。撃破は氷霧でも氷塊でも通知する。解放(倒れない)では通知せず、氷塊は出ない。
- 敵は出現ごとの連番(serial)を持つ。標的は「ポインタとserial」の組(`EnemyTarget`)で、プール再利用で別の敵になったことを`EnemyManager::GetTargetPosition`が検出する。
- 氷塊の状態: 地面(寿命で消える) → 引き寄せ(チャージ中、範囲内) → 所持(プレイヤーの周りを周回) → 発射待ち → 飛行 → 砕けて消える。
- 発射(`Launch`): 氷霧の範囲内の敵から、1個目は移動方向の円錐内の最寄り(なければ全体の最寄り。静止中は最寄り)、以降は直前の敵の最寄りで、連鎖の順番を1つ作る。持っている全部の氷塊が、時間差で飛び始め、その順番に全敵を回る。回り終えた氷塊は砕けて消える。範囲内に敵がいなければ消費しない。
- 着弾(`EnemyManager::ApplyShardHit`): 凍っていない敵は氷ゲージを大きく進め、凍っている敵は砕く。氷霧と違い、同じ連鎖で凍った敵も砕ける。標的が倒されていれば時間をかけずに飛ばす。
- 順番: 氷霧の敵への適用(撃破の氷塊は地面に落ちる)、氷塊の発射、`GameScene::Update`の`StepEnemies`、`StepIceShards`。
- `Player`は`IsCharging()`と、氷霧の通知に添える移動方向を提供する。

## 5. 破砕と氷の殻

- `EnemyManager`は「この位置で砕けた(撃破か解放か)」を`ShatterEffectManager`へ伝える。
- 撃破と解放の違いは、オレンジの破片を一緒に飛ばすかどうかだけ。
- 凍結中の氷の殻の描画は`EnemyManager`が持つ。

## 6. 経験値と強化

- `EnemyManager`が撃破を`EnemyDefeatInfo`(位置、氷塊の数、経験値、連鎖ID)のコールバックで通知する。`GameScene`のラムダが、氷塊(`IceShardManager::Drop`)、経験値(`PhaseDirector::AddExp`。撃破した瞬間に入る)、連鎖数(`ComboPopupView::AddHit`)の順に渡す。
- 連鎖IDは、氷霧で倒したときは0で、`ComboPopupView`は数えない。氷塊は`IceShardManager`の`Volley`が発射元の氷霧のIDを持ち、`EnemyManager::ApplyShardHit`の引数で渡す。
- `PhaseDirector`がレベルアップを判定し、未処理の数を持つ。`GameScene`が選択画面(`UpgradeSelectUI`)を開き、選ばれた結果を`PlayerUpgrade`経由で`PlayerStats`へ書き、`PhaseDirector::ConfirmLevelUp`でフェーズを進める。`PhaseDirector`は`PlayerStats`と`UpgradeSelectUI`を知らない。
- フェーズが変わるたびに、`GameScene`が`EnemyManager::SetSpawnRule`で出現ルールを渡す。`EnemyManager`は`PhaseDirector`を知らない。
- `Player`は範囲の倍率、威力の倍率、移動速度を`PlayerStats`から読む。
- 選択中は`GameScene::UpdateTimeScale`(`FpsCounter::SetTimeScale`を呼ぶ唯一の場所)でタイムスケールを0にし、`Player::SetInputEnabled(false)`でチャージと氷霧を止め、`PadVibration`を止める。`UpgradeSelectUI`は止まらない時間で動く。
- ゲームオーバー中も同じ扱い。`GameScene::IsGameStopped()`(選択中またはゲームオーバー中)が`UpdateTimeScale`の条件で、振動の更新の可否にも使う。

## 6.5 HUDとゲームオーバー

- `GameScene::Update`の最後(`UpdateHud`)で、`Player`と`PhaseDirector`の値から`HudInput`を作って`GameHud`へ渡す。オブジェクトの更新はシーンの更新より先に走るが、`GameHud`は描画の直前に値を反映するため遅れない。`GameHud`、`ComboPopupView`は`Player`、`EnemyManager`、`PhaseDirector`を知らない。`ComboPopupView`は座標の変換のために`Camera`を読むだけ。
- 被弾: `Player::TakeDamage`が実際に減ったかを返し、`GameScene::StepEnemies`が`GameHud::PlayDamage()`と`PadVibration::PlayDamage()`を呼ぶ。無敵時間中の点滅は`Player::Draw`で描画を飛ばす。
- HPが0: `GameScene::CheckGameOver`が`isGameOver_`をtrueにし、`Player::SetInputEnabled(false)`、`PadVibration::Stop()`、`GameOverView::Open()`を行う。`GameOverView`が再開の入力を返すと、`GameScene`が`isFinished_`をtrueにする。`NextSceneName()`は仮に`"Game"`を返し、フェードのあとGameシーンが作り直される(C5で変える)。
- 強化の選択中は、`GameScene::UpdateHud`が`GameHud`と`ComboPopupView`へ`SetVisible(false)`を渡し、画面のUIを描かない(状態は保つ)。ゲーム世界の中の表示は変えない。
- 詳細は[C3_HUD.md](C3_HUD.md)。

## 7. カメラ

- `GameCamera`は`Update(ターゲット位置)`で追従する。初回の呼び出しは補間せず目標位置へ合わせる。
- 補間は指数補間で、フレームレートに依存しない。
- 補間なしで位置を置く`Snap`は作らない。リトライや演出で必要になった時点で追加を判断する。

## 8. 未決定事項

- `BossEnemy`の内容。
