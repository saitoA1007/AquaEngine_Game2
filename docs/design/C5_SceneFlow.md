# C5 タイトルとゲームの終わり方

確定日: 2026-11-18(人間の言葉は「確定」)。仕様の正本は[GameSpec.md](GameSpec.md)、構造の正本は[Architecture.md](Architecture.md)。この文書は、C5の確定設計の全文。

- 背景: ゲーム性(ルール)を人間が見直し中。ルールの変更に耐えられるよう、ルールで変わるところ(終わりの判定)は1か所にまとめる。
- 見た目は仮(新しい画像は作らない)。

## 1. 流れ

- 起動 → `Title` →(決定でスタート)→ `Game` →(ゲームオーバーかクリア)→ その場の選択肢 →「やり直す」で`Game`、「タイトルへ」で`Title`。
- リザルト画面は作らない。値も出さない。シーンをまたいで持つデータもない。
- 使うか分からない抽象化は作らない。

## 2. タイトル

- `TitleScene`(`IScene`): コンストラクタで入力(`Decide`)の登録と`white2x2.png`の取得。`Initialize`の最初に`gameObjectManager_->ClearAll()`(デバッグのリセットは`Initialize`をもう一度呼ぶだけで、二重にならないようにする)。描画に必要なカメラを設定し、`TitleView`を`GameObjectManager`で生成する。`Decide`が押されたら`isFinished_ = true`。`NextSceneName()`は`"Game"`。`GetTransitionEffect()`は既存の`Fade`。
- `TitleView`(`IGameObject`): 暗い青の全画面の板(`white2x2.png`に色を掛けた`Sprite`)、仮の作品名、「Aでスタート」。文字は`OutlinedText`(源直ゴシック EMG 2 - Medium)。位置・大きさ・色は`DebugParameter`。点滅などの演出はない。
- 作品名は未定。今は仮の文字を定数(`kProvisionalTitle`)で置いている。
- `SetUpScenes.h`: `TitleScene`を`"Title"`で登録し、起動シーンを`"Title"`にした。

## 3. ゲームの終わり方

- `GameScene::CheckGameEnd`(旧`CheckGameOver`): HPが0 → ゲームオーバー、デバッグキーF8(`USE_IMGUI`の構成だけ登録・判定)→ クリア。ボス(C4)がまだないための仮のクリア。**ルールで変わる終わりの条件は、この関数だけに置く。**
- 終わったあとの処理は`GameScene::EndGame(GameEndType)`で共通: `isGameEnded_`を立てる(旧`isGameOver_`)、`Player::SetInputEnabled(false)`、`PadVibration::Stop()`、`GameOverView::Open(終わり方)`。`IsGameStopped()`は`isGameEnded_`を見る。
- 終わり方は`GameOverView.h`の`GameEndType { kGameOver, kClear }`。
- `GameOverView::Open(終わり方)`で見出しを「GAME OVER」(赤)か「CLEAR」(水色)に切り替える。待ち時間(`retryWaitTime`、既定1秒)のあとに「やり直す」と「タイトルへ」を縦に並べる。最初は「やり直す」が選ばれている。選んでいる項目と選んでいない項目は色で区別する(`selectedColor`、`unselectedColor`)。上端と下端では止まる。
- `GameOverView::GetChoice()`が選ばれた行き先(`GameEndChoice { kNone, kRetry, kTitle }`)を返す(旧`IsRetryRequested`の置き換え)。決定した後は入力を受け付けない。
- `GameScene::UpdateGameEnd`が行き先を`nextSceneName_`(`"Game"`か`"Title"`)に入れて`isFinished_`を立てる。`NextSceneName()`は`nextSceneName_`を返す。`Initialize`で`"Game"`に戻す。
- 見出し、時間を止めること、UIが止まらない時間で動くことは、C3のまま(`UpdateTimeScale`)。

### 入力

- `MenuUp`/`MenuDown`: W・S、↑・↓、十字キー。いずれも押した瞬間。
- `PadStickBinding`には押した瞬間の設定がないため、スティックは`GameOverView`が前のフレームの値と比べ、しきい値(`stickThreshold`、既定0.5)を越えた瞬間だけを1段の入力にする。
- `Decide`: Space、パッドのA。押した瞬間。旧`Retry`の置き換え。`InputCommand`は全シーンで共有され、`RegisterCommand`は同じ名前を上書きするため、`TitleScene`と`GameScene`の両方で同じ内容を登録している。
- `DebugClear`: F8(`USE_IMGUI`のみ)。

## 4. 決めなかったこと(別のタスク)

- リザルト画面とスコア: 作らない。
- ポーズ: 未定。
- 無限モードの入口: 未定。
- オリジナルのシーン遷移: 人間は暗転ではなくオリジナルで作りたい。今は既存の`Fade`のまま別タスクへ移ってよい。
- 作品名: 未定。

## 5. 調べて分かったこと

- 今の`Fade`は何も描かない。`Fade.cpp`の処理がコメントアウトされ、エンジンの`SceneManager::Draw`も遷移の描画(`sceneTransition_->Draw()`)を呼ばない。見た目は、シーンが切り替わる時間だけ`Update`が止まる動き。
- 遷移中もオブジェクトの更新と描画は続き、シーンの`Update`だけが止まる。そのため、決定後のフェードの間もUIは見え続ける。
- シーン間で値を渡す仕組みはエンジンにない(`SceneManager`は次のシーン名だけを受け取る)。
- シーンの切り替えは、前のシーンの破棄 → 新しいシーンの生成 → `Initialize` → `Update`1回の順。`GameScene`のデストラクタがタイムスケールを1へ戻し、`PadVibration`の破棄が振動を止める。

## 6. 確認が必要なこと(人間の操作確認)

- 氷霧のチャージは`IceMistCharge`(Space、AのPush)。タイトルや選択でAを押したまま次のシーンに入ると、`Player`が入力を受けているため、チャージから氷霧が出る可能性がある。直す仕組みは足していない。起こるかどうかは実機で確認する。
