# 氷霧・連鎖・砕けるエフェクト(A3)

確定日: 2026-11-10。ユーザーの言葉: 「広がる速さは1.8倍の時の速さで。設計を確定します」。
経緯でのユーザーの言葉: 「Aで進めたいです」(氷霧の案A)、「既存で始める」(画像はsmoke.png)、「推奨案でいいです」(連鎖と破片)、「リング状にするよりも頭の上でぐるぐる塊が回っているようにしたい」、「塊の大きさは0.3くらいでいいよ」、「両方入れます。方向の数は32」(上がる冷気と画面の光)。
実装日: 2026-11-11(実装子)。起動しての確認は、人間が行う。

## 1. 氷霧の発動演出(`IceMistEffect`)
- Aを離した瞬間、中心から32方向へ霧が吹き出し、範囲の縁で減速して止まり、そのまま薄れて消える。`ParticleBehavior`(非ループ、ゲームの時間)。
  - 霧は地面と平行な板(非ビルボード、加算)で、高さ`burstHeight`(0.25。氷エリアの板の上面0.22の少し上)。出し始める位置は、中心から半径×`startAreaRatio`(0.9)までに面積が均等になるよう散らし、各粒は外向きに「縁までの残り」だけ流れて縁で止まる(中心にも霧が出る)。初速は約45で固定のため、流れる距離から寿命が決まる(距離=初速÷3×止まるまでの時間)。止まるまでの時間は、単押し(半径4)で最大約0.27秒、最大チャージ(半径8)で最大約0.53秒。
  - 方向ごとに`SetDirection`で32方向を設定する(エンジンのソースで確認済み)。
  - チャージの度合いは半径から求める(`tapRadius`=4、`maxRadius`=8)。強化の範囲で基準を超えると頭打ち。
  - チャージに応じて、1方向あたりの粒の数(3→6)と最終の大きさ(1.4→2.2)が増える。
- 同時に、範囲の中から薄い冷気が上がる(`IceMistRise.json`。ビルボードのまま、高さ0.3、粒は小さめ、加算。旧`IceMistParticle.json`の上昇を土台に、1回の粒の数を70→28、色の濃さを下げた)。旧`IceMistParticle.json`は使わなくなった(削除はしていない)。
- 画像は既存の`smoke.png`。色はJSONで水色。新しい画像は作らない。
- 画面の光: 全画面の2Dスプライト(`white2x2.png`に色を掛ける)が、最大の濃さ0.18から0.25秒で透明になる。ゲームの時間で進む。`IceMistEffect`はHUDより先に作られるので、HUDの下に描かれる(強化の選択画面・ゲームオーバーよりも下)。
- 地面の円の発光とチャージ中の表示は、A8の`IceAreaView`が担当(触っていない)。

## 2. 氷塊(`IceShardManager`)
- 所持中: プレイヤーの頭の上(高さ1.9)の半径0.3の球の中で、塊になって回る。1個ずつ違う傾きの軌道を、違う速さ(基準±50%)・向き(ランダムに逆)・軌道半径(球の半径の35〜100%)で回る。塊全体も縦軸でゆっくり自転する。数が増えても塊の大きさは変えない。
- 引き寄せの途中は直線のまま。行き先が塊の中の各氷塊の位置になる。
- 連鎖で飛ぶ間: 共有のパーティクル(`IceShardTrail.json`、加算、`star.png`、0.2〜0.3秒で縮んで消える)を、飛んでいる各氷塊の位置から毎フレーム1粒ずつ出す。同時数の上限1024。
- 着弾時: 当たった位置で氷の粒が6粒弾ける(`IceShardHit.json`、加算、`star.png`、0.25〜0.4秒)。生きている敵に当たったときも、飛んでいる間に倒された敵の位置へ着いたときも出る。
- 氷塊の形は球のまま。

## 3. 敵が砕けるとき(`ShatterEffectManager`)
- 破片モデルを、立方体8分割(`CellFractureCube8.gltf`)から、球の殻(外半径1、厚み0.3)を12個に割った`Models/IceShellShard12/IceShellShard12.gltf`に変えた。Blender 5.1のCell Fracture(頂点から12個)で作成。ノード名は`IceShellShard12_cell`〜`.011`、破壊グループは1つ。`.glb`ではない。`CellFractureCube8`は削除していない(他で使われていない)。
- 動き: 重力(`gravity`既定30)で落ち、地面で跳ね返り(`restitution`0.35、`restSpeed`以下は止まる)、地面にある間は摩擦(`groundFriction`)で滑りと回転が弱まり、寿命の後半で縮んで消える。地面はy=0。
- 霜の粒の追加、撃破と解放の見分けは今回やらない。

## 4. 調整値(`DebugParameter`)
- 「IceMistEffect」: Burst(speedMultiplier、tapArriveTime、maxArriveTime、tapRadius、maxRadius、tapEndSize、maxEndSize、tapCount、maxCount、lifeVariation、burstHeight、upwardTilt)、Rise(riseCount、riseHeight)、Flash(flashColor、flashMaxAlpha、flashTime)。
- 「IceShard」Orbit: orbitRadius(塊の半径。意味を「リングの半径」から変更)、orbitHeight(塊の中心の高さ。同上)、orbitSpeed、orbitSpeedVariation、orbitSpinSpeed、orbitMinRadiusRatio。保存済みのJSON(`IceShard.json`)は存在せず、値はコードの初期値のまま。
- 「ShatterEffect」Motion: gravity、restitution、groundFriction、restSpeed を追加。保存済みの値(lifetime=1.0、速度3〜7など)はそのまま。
- パーティクルの見た目(`IceMistBurst`、`IceMistRise`、`IceShardTrail`、`IceShardHit`)は、`Resources/Json/GameData/`の同名JSONとエンジンのパーティクルのエディタで調整する。

## 5. 負荷の確認
- 検証はビルド(DebugとRelease)まで。ゲームは起動していない。
- 目標: 50体の同時破砕で60fps。計測の手順は、実装子の報告に書いた(F9で`ShatterEffectManager::StartBenchmark`を実行、`ShatterBenchmark.log`を確認)。破片が8個から12個に増えたため、悪化した場合は`kMaxChunks`を下げるか、モデルの破片数を減らす。
