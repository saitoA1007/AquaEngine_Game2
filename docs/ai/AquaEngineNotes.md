# AquaEngineの確認済み知識と調査先

確認日: 2026-10-30。公式主要ガイド9ページと手元の主要ソースを読み取り調査した記録。ビルド・起動・操作・性能は未検証。将来のエンジン更新後も同じ挙動と仮定せず、実装時に使用中のソースを再確認する。

変更範囲などの規則は[AGENTS.md](../../AGENTS.md)を参照する。この文書は公式資料の代替ではない。

## 構成と入口

- DirectX 12を利用するC++のゲームエンジン。手元のプロジェクト設定はC++20、x64、Debug / Development / Release構成を持つ。
- `Project/Application/`がゲームコード、`Project/Resources/`が素材・設定、`Project/EngineSource/`がエンジン本体。
- 起動は`Project/main.cpp`、シーン登録は`Project/Application/Scene/Register/SetUpScenes.h`。
- シーンは`IScene`、ゲーム内オブジェクトは`IGameObject`を基礎とする。

参照: [公式導入](https://saitoa1007.github.io/AquaEngine/guide/getting_started.html)、[main.cpp](../../Project/main.cpp)、[シーン登録](../../Project/Application/Scene/Register/SetUpScenes.h)、[ビルド設定](../../Project/AquaEngine.vcxproj)。

## 更新順と寿命

- 通常のゲーム更新は、オブジェクト更新 → シーン更新 → 当たり判定の順。描画はシーン → オブジェクトの順。
- `AddObject`は所有権を管理クラスに保持し、非所有のポインタを返す。`Initialize()`は追加処理中に呼ばれる。
- オブジェクトの更新ループ中の追加は待機リストへ入り、その更新ループでは更新されない。ただし、描画や当たり判定まで必ず次フレームになるとは仮定しない。
- `Destroy()`による削除は管理クラスの更新処理で回収される。`RemoveObject`は管理クラスの走査中かどうかで挙動が異なる。衝突コールバックでの削除は走査対象・参照の寿命を追加確認する。
- シーン遷移では登録オブジェクトが削除される。シーンをまたぐ状態は別の所有場所を設計する。
- `SetUpdateOrder`の変更が即座に並び替えを起こすとは仮定しない。初期化時の指定と管理クラスのソート条件を確認する。

参照: [公式オブジェクトガイド](https://saitoa1007.github.io/AquaEngine/guide/game_object.html)、[公式シーンガイド](https://saitoa1007.github.io/AquaEngine/guide/scene.html)、[SceneSubsystem.cpp](../../Project/EngineSource/Core/System/SceneSubsystem.cpp)、[GameObjectManager.h](../../Project/EngineSource/Core/Objects/GameObjectManager.h)、[GameObjectManager.cpp](../../Project/EngineSource/Core/Objects/GameObjectManager.cpp)。

## 描画・入力・当たり判定

| 分野 | 確認した仕組みと注意点 |
| --- | --- |
| 描画 | `RenderQueue`に描画要求を登録する。モデル、アニメーション、インスタンシング、スプライト、文字等の入口がある。 |
| モデル | 同じ素材の`Model`は共有される。共有の初期設定と、`ModelComponent`の個体設定を区別する。位置等の変更後は行列・コンポーネントの更新が必要。 |
| 入力 | キーボード・マウス・パッドの直接取得と、名前付き入力コマンドがある。オブジェクトへの入力参照はシーンから渡す方法が公式ガイドに示されている。 |
| 当たり判定 | 球・AABB・OBB・線分。コライダーは生成・破棄で登録・解除される。位置は手動で更新する。双方の属性・マスクが条件を満たす場合に判定する。 |
| 接触処理 | 接触開始・接触中のコールバックがある。相手情報は`CollisionResult`から取得する。ダメージや押し戻しのゲーム仕様は別途決める。 |

参照: [描画ガイド](https://saitoa1007.github.io/AquaEngine/guide/rendering.html)、[入力ガイド](https://saitoa1007.github.io/AquaEngine/guide/input.html)、[衝突ガイド](https://saitoa1007.github.io/AquaEngine/guide/collision.html)、[RenderQueue.h](../../Project/EngineSource/Graphics/Core/RenderQueue.h)、[ModelComponent.h](../../Project/EngineSource/3D/Model/ModelComponent.h)、[Collider.cpp](../../Project/EngineSource/Collision/Collider.cpp)、[CollisionConfig.h](../../Project/Application/CollisionConfig.h)。

## リソースと調整値

| 種類 | 公式ガイドで確認した入口 |
| --- | --- |
| モデル | `Resources/Models/`、OBJ / glTF、ファイル名で取得 |
| 画像 | `Resources/Textures/`、PNG / DDS等、ファイル名でハンドル取得 |
| 音声 | `Resources/Sounds/`、WAV / MP3、`AudioManager`で再生 |
| アニメーション | `Resources/Animations/AnimationsRegister.json`へ登録し、登録名で取得 |
| フォント | Source内のフォントとcharsetからMSDFを生成し、Generated内のJSON・PNGを利用 |
| 調整値 | `DebugParameter`で登録・適用し、`Resources/Json/GameData/`へ保存 |

表中の`Resources/`は`Project/`からの相対パス。素材はファイル名で管理されるため、フォルダを分けても同名衝突に注意する。各種ローダーのサブフォルダ対応は、種類ごとに確認する。フォント生成や保存操作は書き込みを伴う。

参照: [リソースガイド](https://saitoa1007.github.io/AquaEngine/guide/resources.html)、[音声ガイド](https://saitoa1007.github.io/AquaEngine/guide/audio.html)、[調整値ガイド](https://saitoa1007.github.io/AquaEngine/guide/debug_parameter.html)、[ResourceSubsystem.cpp](../../Project/EngineSource/Core/System/ResourceSubsystem.cpp)、[DebugParameter.cpp](../../Project/EngineSource/Editor/DebugParameter.cpp)。

## Effectとエディタ操作

- `EffectAsset`にはトラック、発生方式、開始時間、継続時間、位置オフセット等があり、`ToJson` / `FromJson` / `Save` / `Load`の入口がある。保存先定数は`Resources/Json/Effects/`。
- トラックはパーティクル名・モデル名を参照する。EffectのJSONだけで関連アセットが揃うとは仮定しない。
- JSON直接生成の候補にはなるが、具体的なスキーマ、パーティクル設定との依存、保存先のサブフォルダ対応、実機プレビューは個別調査が必要。
- AquaEngine専用MCPや外部操作APIの利用可否は未確認。別エンジン向けツールの存在を対応の根拠にしない。
- 画面操作、データ生成、エンジン拡張を区別し、必要なら[機能不足の記録](FeatureGaps.md)で調査する。

参照: [EffectAsset.h](../../Project/EngineSource/3D/Effects/EffectAsset.h)、[EffectAsset.cpp](../../Project/EngineSource/3D/Effects/EffectAsset.cpp)、[EffectsManager.h](../../Project/EngineSource/3D/Effects/EffectsManager.h)。

## まだ確認していないこと

対象PCの実行要件・性能、具体的なゲーム仕様での大量生成負荷、全エディタ操作の保存副作用、Effect一式の生成・読み込み・再生、外部操作接続。これらは機能不足の確定事項ではない。
