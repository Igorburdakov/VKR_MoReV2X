#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

OUTER_LOOP = ["e_out_top", "e_out_right", "e_out_bot", "e_out_left"]
INNER_LOOP = ["e_in_top", "e_in_left", "e_in_bot", "e_in_right"]

VTYPE_TYPE2 = (
    '    <vType id="Type2" accel="2.20" decel="4.50" emergencyDecel="7.50" '
    'minGap="2.50" speedDev="0.10" speedFactor="1.00" maxSpeed="38.89" '
    'length="4.80" width="2.00" height="1.60" '
    'vClass="passenger" color="0.1,0.4,1.0"/>'
)

SPAWN_WINDOW_S = 5.0


def render(total, sim_end, loops):
    lines = [
        f'<!-- vehicles: {total} -->',
        '<routes>',
        '',
        f'    <route id="loop_outer_cw"  edges="{" ".join(OUTER_LOOP)}" repeat="{loops}"/>',
        f'    <route id="loop_inner_ccw" edges="{" ".join(INNER_LOOP)}" repeat="{loops}"/>',
        '',
        VTYPE_TYPE2,
        '',
    ]
    if total <= 0:
        return "\n".join(lines + ['</routes>', ''])

    n_out = total // 2
    n_in = total - n_out
    depart_step = SPAWN_WINDOW_S / float(total)

    specs = []
    veh_idx = 1
    for i in range(n_out):
        specs.append((depart_step * (i + 1), f"veh{veh_idx}", "loop_outer_cw"))
        veh_idx += 1
    for j in range(n_in):
        specs.append((depart_step * (j + 1) + depart_step / 2.0,
                      f"veh{veh_idx}", "loop_inner_ccw"))
        veh_idx += 1
    specs.sort(key=lambda s: s[0])
    for depart, vid, rid in specs:
        lines.append(
            f'    <vehicle id="{vid}" type="Type2" depart="{depart:.2f}" '
            f'route="{rid}" departLane="best" departSpeed="max"/>'
        )
    lines.append('</routes>')
    lines.append('')
    return "\n".join(lines)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--total", type=int, default=80)
    p.add_argument("--sim-end", type=float, default=100.0)
    p.add_argument("--loops", type=int, default=40)
    p.add_argument("--output", default="cars_highway80.rou.xml")
    args = p.parse_args()
    Path(args.output).write_text(render(args.total, args.sim_end, args.loops),
                                 encoding="utf-8")
    print(f"wrote {args.total} vehicles to {args.output}")


if __name__ == "__main__":
    main()
