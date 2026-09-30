# ndt_omp

3D 点群の scan-to-map NDT による自己位置推定。ia-amr-ros の `src/localization/ndt_omp`（ROS1）の ROS 2 移植。
このノードが所有する TF は **`map -> odom` ただ一つ**。

| TF | 担当 |
|---|---|
| `map -> odom` | このノード（事前地図に対する NDT） |
| `odom -> base_link` | 車輪オドメトリ（`odrive_can_driver`） |
| `base_link -> mid360` | `tf_publisher`（`config/transforms.yaml`） |

`base_link -> mid360` は実際の取り付け傾きを正しく持っている必要がある。点群はマッチング前に TF 経由で base_link に変換される。

## ファイル構成

| パス | 内容 |
|---|---|
| `src/ndt_omp_node.cpp` | ノード本体。通常編集するのはここだけ |
| `include/ndt_omp/ndt_scan_matcher.hpp` | pclomp NDT の派生クラス。マッチ品質の算出 |
| `include/pclomp/`, `src/pclomp/` | koide3/ndt_omp のベンダーコード。**編集しない** |
| `msg/NdtStatus.msg` | 毎スキャンの診断メッセージ |
| `config/ndt_omp.yaml` | パラメータ（siranui 向け） |

起動は `autonomous_drive.launch.xml` から常に `launch/ndt_omp.launch.xml` を include する。

## 1 スキャンの処理フロー

1. base_link へ変換（TF が無ければ破棄）
2. voxel downsample（`min_scan_points` 未満でも odom lookup まで進めて `too_few_points` で hold）
3. `odom -> base_link` をスキャンスタンプで lookup（最新時刻へのフォールバックは持たない。引けなければ `no_odom_tf`）
4. 初期姿勢の消化。最初の accept までは `step_trans` / `step_rot` ゲートを止める
5. guess = `map -> odom` × `odom -> base_link`（車輪オドメトリのみ。IMU は使わない）
6. ゲート A: `max_linear_speed_skip`（既定 0 = 無効）
7. NDT align
8. ゲート B: `not_converged` / `fitness` / `inlier_ratio` / `trans_probability` / `step_trans` / `step_rot`
9. accept なら `map -> odom` を更新。reject なら値を変えずスタンプだけ打ち直す

`/initialpose` は何度送ってもよく、受け取るたびに次のスキャンで `map -> odom` を上書きする。

## 設計上の制約

- フィルタを持たない。状態は「最後に accept した `map -> odom`」だけ。
- reject が連続すると guess のデッドレコニング区間が延び、step ゲートで弾き続ける自己強化ループに入りうる。監視すべきは reject の連続長。
- 大域再測位は無い。初期姿勢が誤っていれば `~/status` の `reason` が出続けるだけ。
- undistortion はしない。
- `getFitnessScore()` は使えない（target kd-tree を構築しない）。品質は `NdtScanMatcher::computeMatchStats()` で地図ボクセルから読む。`fitness` は ndt_resolution 付近で飽和するため、判別の主役は `inlier_ratio`。`ndt_resolution` を変えたら `fitness_max`（= 0.5 × resolution）も合わせる。

## 入出力

| 種別 | 既定 | 備考 |
|---|---|---|
| 入力 | `/mid360/points` | `points_topic` |
| 入力 | `/initialpose` | `PoseWithCovarianceStamped`（base_link の map 上の姿勢）。`specify_init_pose:=true` なら `init_pos_*` / `init_ori_*` を使う |
| 出力 | `map -> odom` TF | スタンプはスキャン時刻 + `transform_tolerance`（既定 0.3 s） |
| 出力 | `/localization/odom` | map frame での base_link 姿勢。twist は空 |
| 出力 | `~/status` | `NdtStatus`。accept/reject に関わらず毎スキャン |
| 出力 | `/aligned_points` | 判定に関わらず publish（購読者がいるときだけ） |
| 出力 | `/globalmap_view` | 地図の間引き版（transient_local） |

## デバッグ

`/ndt_omp_node/status` を記録して見る。`accepted` と `reason` で reject の連続長を、`fitness` / `inlier_ratio` / `step_trans` / `step_rot` で閾値の妥当性を確認する。
