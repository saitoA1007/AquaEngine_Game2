# C2 成長と進行の設計(レベル、強化、フェーズ)

確定: 2026-11-08(人間が「確定、起動して」と回答)。仕様の正本は[GameSpec.md](GameSpec.md)、構造の正本は[Architecture.md](Architecture.md)。

- 人間が採用した内容だけを書く。数値(上昇量、経験値、出現数など)は仮で、人間が触って調整する。
- 対象外: ボスの中身(C4)、無限モード、常時表示のUIと経験値玉(C3)、ゲームオーバー(C3)、タイトルとリザルト(C5)、BGMの実装、後半用の強い敵の追加、強化の見た目の作り込み。

## 1. クラスと責務

| クラス | 種類 | 責務 |
| --- | --- | --- |
| `PhaseDirector` | `IGameObject`(`GameObjectManager`で生成。毎フレームの処理は持たない) | 経験値、レベル、フェーズ、未処理のレベルアップ数の管理。フェーズ表を持つ。ボス開始の通知(コールバック。C4がつなぐ)。`DebugUpdate`でImGuiの編集画面を出す |
| `PhaseTable` / `PhaseTableIO` | 構造体と読み書き | フェーズ表のJSONの読み込み、検証、保存。`EnemyDefinitionIO`と同じ形で、`JsonSerializer`を使う |
| `PhaseTableEditor` | `USE_IMGUI`のときだけ | フェーズ表の表示、編集、保存。`EnemyListEditor`と同じ位置づけ |
| `PlayerUpgrade` | 関数のまとまり | 強化の3項目の名前と説明、1回あたりの上昇量(`DebugParameter`)、何回取ったか、`PlayerStats`への反映 |
| `UpgradeSelectUI` | `IGameObject` | 強化選択の画面の表示、カーソル移動、Aでの決定。結果をコールバックで返す |
| `PlayerStats` | 既存の構造体を拡張 | `moveSpeed`に加えて、`rangeScale`(初期値1)と`powerScale`(初期値1) |

依存の向き:

- `PhaseDirector`は`UpgradeSelectUI`と`PlayerStats`を知らない。`GameScene`が間をつなぐ。
- `EnemyManager`は`PhaseDirector`を知らない。`GameScene`がフェーズ変更のときに`EnemyManager::SetSpawnRule(...)`で出現ルールを渡す。

## 2. レベルとフェーズ

- レベルとフェーズは常に同じ番号にする(レベル1 = フェーズ1、…、レベル10 = フェーズ10)。上限は10。
- フェーズ5と9はボーナス、10はボス。ボスの中身はC4。C2はボス開始の通知と、雑魚の出現ルールだけを持つ。

## 3. 更新の流れ

1. `EnemyManager`は撃破の通知`EnemyDefeatInfo`に`exp`(敵一覧の`exp`)を載せる。`GameScene`の撃破のラムダが、`IceShardManager::Drop`と`PhaseDirector::AddExp`を呼ぶ。
2. `AddExp`は経験値を足し、そのフェーズの`expToNext`を超えるたびに、未処理のレベルアップを1つ増やして必要量を引く。余りは繰り越し、1回で複数のレベル分が入ってもよい。ボスのフェーズ中は無視する。選択中に呼ばれた分も加算する。未処理の分も数え、レベル10を超える分は数えない。
3. `GameScene::Update`は、未処理のレベルアップがあり、選択画面が閉じていれば、次を行う。
    - 選択画面を開く
    - `Player::SetInputEnabled(false)`を呼ぶ
    - `PadVibration::Stop()`を呼ぶ。止めている間は`PadVibration::Update`を呼ばない
    - `UpdateTimeScale()`で0にする
4. 選ばれたら、次を行う。
    - `PlayerUpgrade`で`PlayerStats`へ反映する
    - `PhaseDirector::ConfirmLevelUp()`でレベルとフェーズを1つ進める
    - 新しい出現ルールを`EnemyManager`へ渡す
    - フェーズ10に入ったら、ボス開始の通知を出す
5. 未処理が残っていれば、続けて次の選択画面を開く。なければ選択画面を閉じ、入力を戻し、タイムスケールを1にする。
6. `GameScene::Initialize`では、タイムスケールを1に戻し、レベルとフェーズを1から始める。エンジンのリセットで`Initialize`がもう一度呼ばれる既存の仕組みに注意する。

## 4. 止め方

- `FpsCounter::SetTimeScale(0)`で、ゲームの世界(敵、氷塊、プレイヤーの移動、破片、氷霧のパーティクル)を止める。敵の破片も止める。`ShatterEffectManager`は今のまま、ゲーム用の時間を使う。
- 選択画面の動きは`FpsCounter::GetDeltaTime()`(止まらない時間)で進める。BGMは止まらない(音はタイムスケールの影響を受けない。BGMの実装は範囲外)。
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
    - 現在のレベル、経験値、フェーズ、未処理のレベルアップ数の表示
    - 指定したフェーズへ飛ぶボタン(デバッグ用)

## 7. 強化

- 項目は範囲、威力、移動速度の3つ。毎回3つすべてを並べ、1つ選ぶ。
- 範囲: `Player`の氷エリアの通常半径とフルチャージ半径に`rangeScale`を掛けて使う。氷霧と氷塊の範囲は、氷霧の情報から来るので同じく広がる。
- 威力: 単押しとチャージの威力に`powerScale`を掛けて使う。
- 移動速度: `moveSpeed`に足す。
- 1回あたりの上昇量は`DebugParameter`で調整する。`Player`の保存済みの調整値は書き換えない。
- 選択画面:
    - 画面を暗くし、横並びに3枚のカードを出す。カードには名前、短い説明、何回取ったかを書く。
    - 左右の入力(既存の`MoveLeft`/`MoveRight`のコマンド)でカーソルを動かす。一度中立に戻すまで、次の移動をしない。
    - Aで決定する。決定用のコマンドは`PressType::kTrigger`で登録し、キーボードはSpace、パッドはAにする。
    - 開いてから一定時間(調整値、既定0.4秒、実時間)はAを受け付けない。開いた時点でAを押していたら、一度離すまで受け付けない。(A7-1で、この時間はカードが組み上がるまでの時間に置き換わった。Aを一度離すまで受け付けない処理は残る。)
    - 文字は`Text`(フォントは源直ゴシック。`FontManager`から取得)、板は`Sprite`。描画は`RenderQueue`へ登録する。
    - 見た目(破片が集まって組み上がるカード、枠を回る光、文字の影、区切り線)は[A7_UpgradeSelectUI.md](A7_UpgradeSelectUI.md)。

## 8. C3との境界

- C3の受け持ち: ゲームオーバーの条件、経験値玉(`ExpOrbManager`)、HPやレベルの常時表示(HUD)、`EnemyDefeatInfo.chainId`。
- 先に実装した側が作るもの: `Player::SetInputEnabled(bool)`、`GameScene::UpdateTimeScale()`、`EnemyDefeatInfo::exp`。
- 経験値は、`GameScene`が撃破の通知から`AddExp`を呼ぶ。C3が経験値玉を入れたら、玉が届いたときに`AddExp`を呼ぶ形へ変える。
- C3が読む関数の名前と形は変えない: `int32_t GetLevel() const`、`int32_t GetPhase() const`、`float GetExpRatio() const`、`bool IsBonusPhase() const`、`bool IsBossPhase() const`、`void AddExp(int32_t amount)`。
