#!/usr/bin/env python3
"""map.pcd の高さスライスから 2D 地図 (map.png + map.yaml) を作る。

z が [min_height, max_height] の点が落ちたセルを占有 (黒)、それ以外を自由 (白) にする。
点がまばらで壁に穴が空くので、モルフォロジーの closing で隙間を埋めてから書き出す。
原点は map.pcd と同じ map 座標系に合わせる (ndt_omp の位置推定とずれないように)。

例:
  ./scripts/pcd_to_map.py map/map.pcd src/siranui_drive/autonomous_drive/map
"""
import argparse
import pathlib

import cv2
import numpy as np


def load_pcd_xyz(path):
    data = pathlib.Path(path).read_bytes()
    header_end = data.index(b'DATA ')
    header = data[:header_end].decode('ascii').splitlines()
    line_end = data.index(b'\n', header_end) + 1
    fields = {}
    for line in header:
        key, *values = line.split()
        if key in ('FIELDS', 'SIZE', 'TYPE', 'COUNT', 'POINTS'):
            fields[key] = values
    data_type = data[header_end:line_end].split()[1].decode('ascii')
    names = fields['FIELDS']
    if data_type == 'ascii':
        cloud = np.loadtxt(data[line_end:].decode('ascii').splitlines(), ndmin=2)
        return cloud[:, [names.index(c) for c in 'xyz']]
    if data_type != 'binary':
        raise RuntimeError(f'unsupported PCD DATA type: {data_type}')
    sizes = [int(s) for s in fields['SIZE']]
    counts = [int(c) for c in fields.get('COUNT', ['1'] * len(names))]
    types = fields['TYPE']
    dtype = np.dtype([
        (name, {'F': 'f', 'I': 'i', 'U': 'u'}[t] + str(s), (c,) if c > 1 else ())
        for name, s, t, c in zip(names, sizes, types, counts)
    ])
    num_points = int(fields['POINTS'][0])
    cloud = np.frombuffer(data[line_end:], dtype=dtype, count=num_points)
    return np.stack([cloud[c] for c in 'xyz'], axis=1).astype(np.float64)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('pcd', help='入力 PCD (map 座標系、地面 z=0)')
    parser.add_argument('out_dir', help='map.png / map.yaml の出力先')
    parser.add_argument('--resolution', type=float, default=0.05, help='セルサイズ [m]')
    parser.add_argument('--min-height', type=float, default=0.1, help='障害物とみなす高さ下限 [m]（地面を除外）')
    parser.add_argument('--max-height', type=float, default=1.0, help='障害物とみなす高さ上限 [m]')
    parser.add_argument('--margin', type=float, default=1.0, help='点群の外側に足す自由領域 [m]')
    parser.add_argument('--close', type=float, default=0.15, help='壁の隙間を埋める closing のカーネル幅 [m] (0=無効)')
    parser.add_argument('--min-points', type=int, default=1, help='占有とみなすセル内の最少点数（ノイズ除去）')
    args = parser.parse_args()

    xyz = load_pcd_xyz(args.pcd)
    min_xy = xyz[:, :2].min(axis=0) - args.margin
    max_xy = xyz[:, :2].max(axis=0) + args.margin
    # origin をセル境界にそろえておくと yaml の数字が読みやすい
    min_xy = np.floor(min_xy / args.resolution) * args.resolution
    width, height = np.ceil((max_xy - min_xy) / args.resolution).astype(int)

    sliced = xyz[(xyz[:, 2] >= args.min_height) & (xyz[:, 2] <= args.max_height)]
    ix = np.floor((sliced[:, 0] - min_xy[0]) / args.resolution).astype(int)
    iy = np.floor((sliced[:, 1] - min_xy[1]) / args.resolution).astype(int)
    counts = np.zeros((height, width), dtype=np.int32)
    np.add.at(counts, (iy, ix), 1)
    occupied = (counts >= args.min_points).astype(np.uint8)

    kernel_cells = int(round(args.close / args.resolution))
    if kernel_cells > 1:
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kernel_cells, kernel_cells))
        occupied = cv2.morphologyEx(occupied, cv2.MORPH_CLOSE, kernel)

    # 画像の行 0 が地図の上端 (y 最大) なので上下反転して書き出す
    image = np.where(occupied > 0, 0, 254).astype(np.uint8)[::-1]
    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(out_dir / 'map.png'), image)
    (out_dir / 'map.yaml').write_text(
        'image: map.png\n'
        f'resolution: {args.resolution}\n'
        f'origin: [{min_xy[0]:.3f}, {min_xy[1]:.3f}, 0.0]\n'
        'negate: 0\n'
        'occupied_thresh: 0.65\n'
        'free_thresh: 0.196\n')
    print(f'{len(xyz)} points ({len(sliced)} in z=[{args.min_height}, {args.max_height}]) -> '
          f'{width}x{height} cells, {int(occupied.sum())} occupied, origin=({min_xy[0]:.3f}, {min_xy[1]:.3f})')


if __name__ == '__main__':
    main()
