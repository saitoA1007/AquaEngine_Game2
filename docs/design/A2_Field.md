# フィールドの見た目(A2)

確定日: 2026-11-18(修正方針の確定も同日)。見た目は「平らな雪原、オーロラの夜、結晶、高い壁」。ルールに左右される部分(円、半径50、押し戻し)は「今のまま仮置き」で、変えない。

## 動き(ルール)は変えない
- 円形、半径50、端で押し戻して滑る、障害物なし。`Field::ClampToField`と敵側の押し戻しには触らない。
- 障害物、当たり判定、フィールドの形の変更、フェーズによる見た目の変化は、このタスクでやらない。

## 構成
| 部品 | 場所 | 内容 |
| --- | --- | --- |
| 床・境目の壁 | `Field` | 円盤の床(y=0)と、円周に並べた紺の壁。範囲と押し戻しも持つ。 |
| 雪原と結晶 | `FieldScenery` | 円の外の見た目だけ。当たり判定なし。 |
| 空 | `iceNightSky.dds` | オーロラの夜空のキューブマップ(各面128px)。 |
| ライト | `GameScene`の`lightColor_` | `(0.8, 0.95, 1.0)`。少し緑がかった月光。 |
| フォグ | なし | 廃止。`PostEffectGraph.json`からFogを消した。 |

## 修正の理由(プレイ確認の問題)
- エンジンの`Mesh::CreateCylinder`は側面だけでふたがなく、地面の上面がなかった。
- HPアイコンは3Dで`DefaultPass`に描かれるため、Fog(レイトレの深度)で暗くなっていた。2Dと文字は`UIPass`でFogの後に合成される。
- 映る地面が狭く(自機の後ろ約6〜前約8、横±9〜12)、空は映り込みと環境光にだけ出る。空が暗いと全体が暗くなる。

## Field
- 床: `GameScene`が`RegisterRingModel("FieldDisc", 64, 1.0, 0.0)`で登録(内側0で円盤)。Ringは法線+Zなので、X軸を-90°回して法線を+Yにする(このエンジンの行列は行ベクトルで、-90°で+Zが+Yになる)。大きさは半径、y=0。色`(0.6, 0.68, 0.8)`、粗さ0.6、金属感0。
- 壁: `cube.obj`(±1)を最大128個(`kMaxWallSegments`)作っておき、`wallSegmentCount`(既定64)だけ描く。中心は半径+幅/2の円上、接線方向の長さは`2π(半径+幅/2)/個数×(1+wallOverlap)`(重なり既定0.1)。高さ`wallHeight`0.6(氷エリアの板の上面0.22より高い)、幅`wallWidth`1.0、底はy=0。色`(0.08, 0.12, 0.28)`、粗さ0.3。値が変わったときだけ並べ直す。Y回転は-角度でローカルXが半径方向になる。
- `DebugParameter`グループ`Field`: radius、wallSegmentCount、wallHeight、wallWidth、wallOverlap、床と壁の色・粗さ・金属感。
- 台地(`groundThickness`、`rimThickness`、`GetGroundThickness()`、`GetRimWidth()`)とCylinderモデルの受け取りは廃止した。`GetWallWidth()`を追加。

## FieldScenery
- 雪原: `planeXZ.obj`を`半径×snowScale(4)`に広げ、y=`snowOffsetY`(-0.01。床とのちらつき防止)に置く。色`(0.45, 0.52, 0.65)`、粗さ0.9。
- 結晶のモデル: `Project/Resources/Models/FieldCrystal/iceCrystalA〜D.gltf`(+`.bin`)。先の尖った六角の結晶が3〜7本の塊、閉じた形、原点は底の中心、高さ約1、白一色。色はコードで掛ける。
- 制作job(TD2_2): `C:\Users\syobo\nihonkougakuinn\InBox\TD2_2\AIRecords\AssetWork\Jobs\eb79b0b7c4e941549a3424b52cfac96a\`(名前「A2 氷の結晶」)。`sources/build_crystals.py`をBlender CLIで実行して`exports/`に出す(乱数はシード固定。形の調整値は先頭の定数)。
- 並べ方(`Rebuild`、シード2026): 円周上に`clusterCount`(10)か所の群れを、等間隔+`angleJitter`のずらしで置く。1群れに`crystalsPerCluster`(2〜5)個。1個ごとに、種類(4種ランダム。種類ごとの上限48で、満杯なら次の種類)、壁の外側からの距離(`bandInner`1.5〜+`bandWidth`6.5)、群れの中心からの横のずれ(`clusterSpread`3)、Y回転、外向きの傾き(最大`maxTiltDegree`15°)、高さ(`heightRange`1.5〜6)を決める。
- 傾き: 拡縮→Y回転→外向きの傾き→移動を行列で合成する。傾ける軸は(外向きz, 0, -外向きx)。`worldTransform_.UpdateWorldMatrix`で行列を直接入れ、`Update`では上書きしない。
- 高さの上限: 壁の外側からの距離×`occlusionRatio`(1.5)。自機が隠れないようにする(カメラ60°では、外側の距離dで視線は高さ約1.73dを通る)。
- 色`(0.55, 0.78, 0.95)`、粗さ0.15、金属感0.1。`ModelComponent`は種類ごとに48個を最初に作り、使う数だけ描く。
- 調整値は全て`DebugParameter`のグループ`FieldScenery`。調整値の変更、または`Field`の半径・壁の幅の変更で並べ直す。

## マテリアル(2026-11-18確定。壁・自機・敵は変えない)
- 動機: 「すべて単色で物足りない」。結晶に氷マテリアル、床と外の雪原に雪の画像とノーマルマップを使う。
- 結晶: `FieldScenery`が`GameEngine::IceMaterial`を1つ持ち、全結晶・全メッシュに`SetBufferMaterial(kDefalutMaterial, ...)`、`SetHitGroup(1)`、`SetInstanceMask(kRayMaskIce)`を設定(`IceDemo`と同じ)。初期値は`IceMaterial`の初期値に、色`(0.55, 0.78, 0.95)`、粗さ0.15。調整値は`DebugParameter`のグループ`FieldScenery`、サブグループ`CrystalIce`(color、roughness、ior、chip/edge/micro、bubble、rim)。従来の`crystalColor/Roughness/Metallic`は廃止。
- (床と壁は下の「床と境目の作り直し」で置き換え済み)
- 床の円盤: Ringをやめ、`Resources/Models/FieldCrystal/fieldDisc.gltf`(半径1、64分割、法線+Y、UVは真上から投影 u=x*0.5+0.5, v=z*0.5+0.5)を使う。`GameScene`が`GetNameByModel("fieldDisc.gltf")`で取得。回転は不要で、scaleは`{半径, 1, 半径}`。
- 雪の画像: `Resources/Textures/Field/fieldSnowAlbedo.png`(白〜淡い青のムラ)、`fieldSnowNormal.png`(風紋の凹凸)。256px、継ぎ目なく繰り返せる。エンジンは画像をすべてsRGBで読むため、ノーマルマップはリニア→sRGBの逆補正をかけて保存してある。`GameScene`でハンドルを取得し0でないことをassertして`Field`・`FieldScenery`へ渡す。
- 貼り方: `materialData_->textureHandle`/`normalTextureHandle`と`uvTransform`の拡大=「面の一辺の長さ ÷ `snowTileSize`(既定8、`Field`と`FieldScenery`それぞれの`DebugParameter`)」。床は直径、外の雪原は`2×半径×snowScale`。色(床`(0.6,0.68,0.8)`、外`(0.45,0.52,0.65)`)は画像に掛ける。
- 制作job(TD2_2): `...\Jobs\5c3e8a1d7b4f4e2a9d6c0f1b2a3e4d58\`(名前「A2 雪の画像」)。`sources/makeSnowTextures.mjs`(`node`で実行。シード固定、値は先頭の定数)、`sources/build_field_disc.py`(Blender CLI)。`exports/`を上記の場所へコピーする。
- 初回起動で画像のBC7圧縮が走る(256pxで1枚約16秒、2回目からキャッシュ)。`Textures/Cache/`は消さない。
- 確認済み: Debugビルド成功。未確認: 起動・見た目(ノーマルマップの向き、継ぎ目、床の明るさとブルーム)。
## 床と境目の作り直し(2026-11-18、「床も境目も単色でダサい」を受けて確定。壁は廃止)
- 方向: 床=凍った湖(反射)、境目=氷の塊を円周に並べる。紺の壁(cube.obj)はなくなった。「何度も壁打ちで調整する」前提。
- 床: `fieldDisc.gltf`に**通常マテリアル**でひび割れの画像`Textures/Field/fieldIceAlbedo.png`/`fieldIceNormal.png`(512px、周期的Voronoiの境目の線を白く、ノーマルは溝。リニア→sRGB逆補正済み)を貼る。`uvTransform`の拡大=直径÷`crackTileSize`(既定10)。色`(0.9,0.95,1.0)`(画像がすでに氷の青なので薄く掛ける)、粗さ0.08、金属感0.35(`Field`のDebugParameter)。
- 床に通常マテリアルを選んだ理由: `chsObject.hlsl`は反射レイ+IBLで、粗さを下げ金属感を上げると空(オーロラ)が映り、不透明で床の下が見えない。氷マテリアル(`ChsIceObject.hlsl`)は屈折レイで床の下(外の雪原)が透け、反射もフレネルの下限0.3の固定。床には不向き。
- 境目: `Resources/Models/FieldCrystal/iceRockA〜C.gltf`(凸包の角張った岩。底y=0、高さ1、平面の広がり1、白一色)。`Field`が`IceMaterial`を1つ持ち全塊に適用(結晶と同じ初期値、DebugParameterのサブグループ`RockIce`)。最大256個(`kMaxRocks`)を作り、`rockCount`(既定160)だけ描く。種類は固定シードで割り当て、並べ方は`rockSeed`。
- 並べ方: 中心半径=半径+幅/2−`rockInset`(0.2)±`rockRadialJitter`(0.3)、角度は等間隔から少しずらす。幅`rockWidthRange`(1.5〜2)、高さ`rockHeightRange`(1〜1.5)、水平軸まわりの傾き最大`rockMaxTiltDegree`(12°)、底を高さ×`rockEmbedRatio`(0.08)埋める。値が変わったときだけ並べ直す。`GetWallWidth()`=最大幅−inset+ジッタ(`FieldScenery`の結晶の帯の始まりに使う)。`wallSegmentCount`など壁の調整値は廃止。
- 制作job(TD2_2): `...\Jobs\7d2b9e4f1c3a4b6e8a5d0c9f2e1b3a76\`(名前「A2 凍った湖と氷塊」)。`sources/makeIceCrackTextures.mjs`(node)、`sources/build_ice_rocks.py`(Blender CLI)。外の雪原は従来の`fieldSnow*.png`のまま。
- 確認済み: Debugビルド成功(2026-11-18、Opusの実装子が再確認)。未確認: 起動・見た目(反射、ひびの見え方、塊の並びと高さ、自機が隠れないか、初回の画像圧縮時間)。

## 空
- `Project/Resources/Textures/Bg/iceNightSky.dds`。各面128px、`R16G16B16A16_FLOAT`、キューブ、ミップ1つ。
- 色(リニア): 上`(0.06, 0.10, 0.22)`、地平線`(0.25, 0.35, 0.5)`、下`(0.15, 0.2, 0.3)`。空の上半分に、緑`(0.2, 0.9, 0.5)`と水色`(0.2, 0.7, 0.9)`のオーロラの帯(明るさ約0.6)。方位角に沿ってsinの重ね合わせで波打たせる。
- 制作job(TD2_2): `...\Jobs\a2f1d0c3b7e84a9d8c2e5f61a3b79d04\`。`sources/makeSkyCube.mjs`(先頭の定数で色と帯の形を変える)を`node sources/makeSkyCube.mjs`で実行し、`exports/iceNightSky.dds`を出して`Textures/Bg/`へ上書きコピーする。
- `GameScene`の`skyboxTextureHandle_`は`iceNightSky.dds`。

## ポストエフェクト
- Fogは使わない(`PostEffectGraph.json`はA2の前のつなぎ方に戻した)。C5の`TitleScene::DisableFog/RestoreFog`はFogがなければ早期リターンする。

## 分かっている小さな問題(直していない)
- 氷塊・破片の地面はy=0の固定なので、円の外へ飛んだものは空中で跳ねる。
