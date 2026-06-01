#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
from pathlib import Path

STRAIGHT_LEN = 300.0
R_OUTER = 120.0
R_INNER = 90.0
LANES = 3
LANE_WIDTH = 4.0
SPEED_STRAIGHT = 38.89
SPEED_ARC = 27.78
ARC_POINTS = 24

X_LEFT = -STRAIGHT_LEN / 2.0
X_RIGHT = STRAIGHT_LEN / 2.0


def arc_shape(cx, cy, r, theta_start, theta_end, n):
    pts = []
    for i in range(n + 1):
        t = theta_start + (theta_end - theta_start) * i / n
        x = cx + r * math.cos(t)
        y = cy + r * math.sin(t)
        pts.append(f"{x:.2f},{y:.2f}")
    return " ".join(pts)


def gen_nodes():
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<nodes>',
        f'    <node id="n_out_nw" x="{X_LEFT}"  y="{R_OUTER}"  type="priority"/>',
        f'    <node id="n_out_ne" x="{X_RIGHT}" y="{R_OUTER}"  type="priority"/>',
        f'    <node id="n_out_se" x="{X_RIGHT}" y="{-R_OUTER}" type="priority"/>',
        f'    <node id="n_out_sw" x="{X_LEFT}"  y="{-R_OUTER}" type="priority"/>',
        f'    <node id="n_in_nw"  x="{X_LEFT}"  y="{R_INNER}"  type="priority"/>',
        f'    <node id="n_in_ne"  x="{X_RIGHT}" y="{R_INNER}"  type="priority"/>',
        f'    <node id="n_in_se"  x="{X_RIGHT}" y="{-R_INNER}" type="priority"/>',
        f'    <node id="n_in_sw"  x="{X_LEFT}"  y="{-R_INNER}" type="priority"/>',
        '</nodes>',
    ]
    return "\n".join(lines) + "\n"


def edge(eid, src, dst, etype, shape=None):
    s = f'    <edge id="{eid}" from="{src}" to="{dst}" type="{etype}"'
    if shape:
        s += f' shape="{shape}"'
    s += '/>'
    return s


def gen_edges():
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<edges>',
        edge("e_out_top", "n_out_nw", "n_out_ne", "straight"),
        edge("e_out_right", "n_out_ne", "n_out_se", "arc",
             arc_shape(X_RIGHT, 0.0, R_OUTER, math.pi / 2, -math.pi / 2, ARC_POINTS)),
        edge("e_out_bot", "n_out_se", "n_out_sw", "straight"),
        edge("e_out_left", "n_out_sw", "n_out_nw", "arc",
             arc_shape(X_LEFT, 0.0, R_OUTER, -math.pi / 2, -3 * math.pi / 2, ARC_POINTS)),
        edge("e_in_top", "n_in_ne", "n_in_nw", "straight"),
        edge("e_in_left", "n_in_nw", "n_in_sw", "arc",
             arc_shape(X_LEFT, 0.0, R_INNER, math.pi / 2, 3 * math.pi / 2, ARC_POINTS)),
        edge("e_in_bot", "n_in_sw", "n_in_se", "straight"),
        edge("e_in_right", "n_in_se", "n_in_ne", "arc",
             arc_shape(X_RIGHT, 0.0, R_INNER, -math.pi / 2, math.pi / 2, ARC_POINTS)),
        '</edges>',
    ]
    return "\n".join(lines) + "\n"


def gen_types():
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<types>\n'
        f'    <type id="straight" priority="10" numLanes="{LANES}" '
        f'speed="{SPEED_STRAIGHT}" width="{LANE_WIDTH}"/>\n'
        f'    <type id="arc"      priority="10" numLanes="{LANES}" '
        f'speed="{SPEED_ARC}"      width="{LANE_WIDTH}"/>\n'
        '</types>\n'
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default=".")
    args = parser.parse_args()
    d = Path(args.out_dir)
    d.mkdir(parents=True, exist_ok=True)
    (d / "highway_nodes.nod.xml").write_text(gen_nodes(), encoding="utf-8")
    (d / "highway_edges.edg.xml").write_text(gen_edges(), encoding="utf-8")
    (d / "highway_types.typ.xml").write_text(gen_types(), encoding="utf-8")
    print(f"wrote nodes/edges/types to {d}/")


if __name__ == "__main__":
    main()
