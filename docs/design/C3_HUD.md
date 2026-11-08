# C3 HUD・被弾・ゲームオーバー・連鎖数

確定日: 2026-11-08(人間の言葉は「この設計で確定」)。同日、人間の判断で一部を廃止し、「この変更で確定」(2026-11-08)。仕様の正本は[GameSpec.md](GameSpec.md)、構造の正本は[Architecture.md](Architecture.md)。この文書は、C3の確定設計の全文。

- 見た目は仮素材(新しい素材は追加しない)。正式な画像は別のタスクで作る。
- リザルト、タイトル、スコアはC5。氷塊の所持数は表示しない。

## 1. 常時表示(HUD)

- 左上にHPアイコン(初期HPは3。最大HPは`Player`の`initialHp_`)。右上に「PHASE 4」の文字。フェーズの色はボーナスでもボスでも同じ。
- 位置、大きさ、色は`DebugParameter`で調整できる。

## 2. 被弾の表現

- HPアイコン: 失ったアイコンが一瞬大きく弾けてから暗くなる。
- 無敵時間のあいだ、自機を点滅させる。`Player::Draw`で、点滅の間隔(調整値`blinkInterval`)ごとに描画を飛ばす。
- 振動: `PadVibration::PlayDamage()`。強さ(`damageStrength`)と時間(`damageTime`)は調整値。
- 流れ: `Player::TakeDamage`が、実際にHPが減ったかを`bool`で返す。`GameScene::StepEnemies`で、減ったときに`GameHud::PlayDamage()`と`PadVibration::PlayDamage()`を呼ぶ。

## 3. HPが0のとき(最低限の終わり方)

- `GameScene`がゲームオーバーの状態(`isGameOver_`)に入る。
- ゲームを止める処理は`GameScene::UpdateTimeScale()`の1か所。条件は`IsGameStopped()`(強化の選択中、またはゲームオーバー中)で、止めるなら`FpsCounter::SetTimeScale(0)`、それ以外は1。`Initialize`とデストラクタで1に戻す。
- `Player::SetInputEnabled(false)`でチャージを取り消し、氷霧を出さず、入力を受け付けない。止まっている間にAを離して氷霧が出るのを防ぐ。C2の強化の選択も同じ関数を使う。
- 止めている間は、`GameScene`が`PadVibration::Stop()`を呼び、`PadVibration::Update`を呼ばない。
- `GameOverView`が「GAME OVER」を出す。待ち時間(`retryWaitTime`、既定1秒。止まらない`FpsCounter::GetDeltaTime`で数える)の後に「Aで再開」を出し、入力コマンド`Retry`(Space、パッドのA。押した瞬間)を受け付ける。`GameScene`が`isFinished_ = true`にし、フェードを挟んでGameシーンを最初からやり直す。
- `GameScene::NextSceneName()`は`"Game"`(仮)。`"Title"`は登録されておらず、シーンが終わると`assert`で止まるため。C5で変える。
- UIの演出は止まらない時間(`GetDeltaTime`)で進める。連鎖数はゲームの時間(`GetGameDeltaTime`)で動くため、止まっている間は一緒に止まる。

## 4. 連鎖数(`ComboPopupView`)

- 1回の氷霧と、その氷霧から飛んだ氷塊で砕いた数を「12 HIT」と数え上げる。
- `EnemyDefeatInfo::chainId`: 氷霧で倒したときは`IceMistInfo::id`。氷塊の連鎖は、発射元の氷霧のIDを`IceShardManager`の`Volley`に持たせ、`EnemyManager::ApplyShardHit`の`chainId`引数で渡す。
- 表示は1つの連鎖につき1つ。新しく砕けた敵の画面位置(`Camera::GetVPMatrix()`で1280×720へ変換)へ移って数が増え、数が大きいほど文字を大きくする。一定時間(`holdTime`)増えなければ消える。同時に出せる表示は8つまで(`maxPopups`、上限8)。文字はSDFの`Text`(源直ゴシック)。
- 1体だけ砕いたときも「1 HIT」から出す。
- 表示が上限まで使われているときは、新しい連鎖の表示は出さない(数えない)。

## 5. クラス、依存、更新の流れ

| クラス | 役割 |
| --- | --- |
| `GameHud`(`IGameObject`) | HPアイコンとフェーズを描く。ゲームのクラスを知らず、`GameScene`から`HudInput`を受け取って表示する。`HpIconView`を所有する |
| `HudInput`(構造体) | HP、最大HP、フェーズ |
| `HpIconView` | HPアイコンの並びと被弾の演出 |
| `GameOverView`(`IGameObject`) | 「GAME OVER」と「Aで再開」、再開の入力 |
| `ComboPopupView`(`IGameObject`) | 連鎖数の表示 |

- すべて`GameObjectManager`で生成し、`GameScene`が組み立てる。どのクラスも`Player`、`EnemyManager`、`PhaseDirector`を直接知らない。`ComboPopupView`は座標の変換のために`Camera`を読むだけ。
- `GameScene::Update`の最後(`UpdateHud`)で`HudInput`を作って`GameHud`へ渡す。`GameHud`は描画の直前に反映するため、1フレームの遅れなく表示できる。エンジンによる更新と手動の呼び出しは重複させない。
- 撃破の通知: `SetDefeatCallback`の受け口は1つだけ。`GameScene`のラムダの中で、氷塊(`Drop`)、経験値(`PhaseDirector::AddExp`。撃破した瞬間に入る)、連鎖数(`AddHit`)の順に渡す。
- 新しいファイルは`Project/Application/Game/UI/`。既存の共有UI部品`Project/Application/UI/`は変更しない。

## 6. 仮の素材

- HPアイコン: `circle.png`を水色に塗る。失った分は暗い色。
- 文字: 「源直ゴシック EMG 2 - Medium」。
- テクスチャ名は`GameScene::LoadAssets`で取得する(`circle.png`)。正式な画像への差し替えは、ここと各クラスの引数で行う。

## 7. 廃止した案

2026-11-08、人間の判断(「レベルと経験値バーはいらないかも」「経験値の曲線の動きで入っていく演出もとりあえずなくしましょう」)で廃止した。実装は削除済み。

- Lvの文字と経験値バー(HUD左上)。
- 経験値玉(`ExpOrbManager`): 撃破位置から光る玉を出して弾けさせ、ベジェ曲線でLvバーへ飛ばし、届いたときに経験値を入れる。経験値は撃破した瞬間に入る形に戻した。
