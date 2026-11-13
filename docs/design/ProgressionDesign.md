# C2 成長と進行の設計(フェーズ)

確定: 2026-11-08(人間が「確定、起動して」と回答)。2026-11-13に強化をなくした(G1。[GameSpec.md](GameSpec.md)の「置き換えた判断」)。仕様の正本は[GameSpec.md](GameSpec.md)、構造の正本は[Architecture.md](Architecture.md)。

- 人間が採用した内容だけを書く。数値(経験値、出現数など)は仮で、人間が触って調整する。
- 対象外: ボスの中身(C4)、無限モード、常時表示のUIと経験値玉(C3)、ゲームオーバー(C3)、タイトルとリザルト(C5)、BGMの実装、後半用の強い敵の追加。
- 強化(範囲、威力、移動速度の選択)はなくした。範囲、威力、移動速度は固定の値で、`Player`の`DebugParameter`と`PlayerStats::moveSpeed`で人間が調整する。「レベル」という言葉も使わず、フェーズだけで数える。

## 1. クラスと責務

| クラス | 種類 | 責務 |
| --- | --- | --- |
| `PhaseDirector` | `IGameObject`(`GameObjectManager`で生成。毎フレームの処理は持たない) | 経験値とフェーズの管理。フェーズ表を持つ。経験値が必要量を超えたらその場でフェーズを進める。ボス開始の通知(コールバック。C4がつなぐ)。`DebugUpdate`でImGuiの編集画面を出す |
| `PhaseTable` / `PhaseTableIO` | 構造体と読み書き | フェーズ表のJSONの読み込み、検証、保存。`EnemyDefinitionIO`と同じ形で、`JsonSerializer`を使う |
| `PhaseTableEditor` | `USE_IMGUI`のときだけ | フェーズ表の表示、編集、保存。`EnemyListEditor`と同じ位置づけ |
| `PlayerStats` | 既存の構造体 | `moveSpeed`だけを持つ。範囲と威力は`Player`の調整値 |

依存の向き:

- `PhaseDirector`は`PlayerStats`を知らない。
- `EnemyManager`は`PhaseDirector`を知らない。`GameScene`がフェーズ変更のときに`EnemyManager::SetSpawnRule(...)`で出現ルールを渡す。

## 2. フェーズ

- フェーズは1から10。フェーズ5と9はボーナス、10はボス。ボスの中身はC4。C2はボス開始の通知と、雑魚の出現ルールだけを持つ。

## 3. 更新の流れ

1. `EnemyManager`は撃破の通知`EnemyDefeatInfo`に`exp`(敵一覧の`exp`)を載せる。`GameScene`の撃破のラムダが、`IceShardManager::Drop`と`PhaseDirector::AddExp`を呼ぶ。
2. `AddExp`は経験値を足し、そのフェーズの`expToNext`を超えるたびに、その場でフェーズを1つ進めて必要量を引く。余りは繰り越し、1回で複数フェーズ分が入ったときは続けて進める。ボスのフェーズ中は無視する。フェーズ10を超える分は数えない。
3. フェーズが進んだら、`AddExp`の中で出現ルールの変更の通知(`SetRuleChangedCallback`。`GameScene`が`ApplySpawnRule`を呼び、新しい出現ルールを`EnemyManager`へ渡す)を1回出し、フェーズ10に入ったらボス開始の通知を出す。ゲームは止まらない。
4. ゲームの世界を止めるのはゲームオーバーのときだけ(`GameScene::IsGameStopped`、`UpdateTimeScale`、`Player::SetInputEnabled(false)`、`PadVibration::Stop()`)。
5. `GameScene::Initialize`では、タイムスケールを1に戻し、フェーズを1から始める。エンジンのリセットで`Initialize`がもう一度呼ばれる既存の仕組みに注意する。

## 4. 止め方

- ゲームオーバー中だけ、`FpsCounter::SetTimeScale(0)`で、ゲームの世界(敵、氷塊、プレイヤーの移動、破片、氷霧のパーティクル)を止める。`ShatterEffectManager`は今のまま、ゲーム用の時間を使う。
- 表示の動きは`FpsCounter::GetDeltaTime()`(止まらない時間)で進める。BGMは止まらない(BGMの実装は範囲外)。
- `FpsCounter::SetTimeScale`を呼ぶのは`GameScene::UpdateTimeScale()`の1か所だけにする。

## 5. 出現(`EnemyManager`)

- 全体共通の調整値`spawnInterval`と`spawnPerInterval`をなくし、フェーズ表の値を使う。`spawnDistance`、凍結の値、敵一覧の値は全体共通のまま。
- 出現ルール: 間隔、1回に出す数、フェーズ全体の最大数`maxEnemies`、種類ごとの重み。重みは敵のIDで受け取り、`SetSpawnRule`の中で種類の番号に変換する。
- 間隔ごとに、`spawnPerInterval`体を出そうとする。1フレームに溜まった分を一度に出さない既存の制限は残す。
- 使用中の敵の総数が`maxEnemies`に達していたら出さない。「最大数」はフェーズ全体で同時に存在する数で、種類ごとの最大数は持たない。プールの上限は敵一覧の`poolSize`のまま。
- 種類は重みで抽選する。重みが0の種類と、プールに空きがない種類は外す。候補がなければ出さない。
- フェーズが変わっても、出ている敵は消さない。
- ボーナス専用の凍結処理は作らない。ボーナスは、一発で凍る設定の敵を多く出すことで表す。敵の調整は人間が敵一覧で行う。
- 敵一覧(`EnemyDefinition`の配列)を読む`const`の関数(`GetDefinitions`)を持つ。フェーズ表の検証と編集画面で敵のIDを使うため。

## 6. フェーズ表

- 保存先は`Resources/Json/PhaseData/PhaseTable.json`。`Resources/Json/EnemyData/`と`Resources/Json/GameData/`は使わない。
- 形:

```json
{ "phases": [
  { "type": "Normal", "expToNext": 10, "spawnInterval": 1.0, "spawnPerInterval": 3, "maxEnemies": 60,
    "weights": { "chase": 3.0, "fast": 1.0 } }
] }
```

- 10行にする。5行目と9行目は`"Bonus"`、10行目は`"Boss"`。ボスの`expToNext`は使わない。
- 検証の内容:
    - 10行あること
    - `type`が3種類のどれかであること
    - 重みのIDが敵一覧に存在すること
    - 重み、`expToNext`、1回に出す数、最大数が0以上であること
    - 間隔が正の値であること
- 検証に失敗したら、敵一覧と同じく例外を出す。敵一覧にあっても重みに書いていない種類は、重み0として扱う。Release構成では読み込みだけ。
- 初期の中身は仮。間隔1.0秒、1回3体(元の保存値)をもとにし、敵`chase`と`fast`の重みを置く。ボーナスは1回に出す数と最大数を大きめにする。`expToNext`は1プレイ5〜8分を目安に仮で置く。数値は人間が調整する。
- 編集画面で行うこと:
    - フェーズの選択と、各値の編集
    - 重みの編集。敵一覧の全種類を並べる
    - 保存。保存すると、今のフェーズの出現ルールにすぐ反映する
    - 現在のフェーズと経験値の表示
    - 指定したフェーズへ飛ぶボタン(デバッグ用)

## 7. C3との境界

- C3の受け持ち: ゲームオーバーの条件、経験値玉(`ExpOrbManager`)、HPやフェーズの常時表示(HUD)、`EnemyDefeatInfo.chainId`。
- 先に実装した側が作るもの: `Player::SetInputEnabled(bool)`、`GameScene::UpdateTimeScale()`、`EnemyDefeatInfo::exp`。
- 経験値は、`GameScene`が撃破の通知から`AddExp`を呼ぶ。C3が経験値玉を入れたら、玉が届いたときに`AddExp`を呼ぶ形へ変える。
- C3が読む関数の名前と形は変えない: `int32_t GetPhase() const`、`float GetExpRatio() const`、`bool IsBonusPhase() const`、`bool IsBossPhase() const`、`void AddExp(int32_t amount)`。(`GetLevel`は強化とともになくした。)
