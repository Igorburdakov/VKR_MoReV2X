#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VEHICLE_COUNT="${VEHICLE_COUNT:-80}"
SIM_END="${SIM_END:-100}"
ROUTE_LOOPS="${ROUTE_LOOPS:-40}"

python3 "${SCRIPT_DIR}/generate_highway_geometry.py" --out-dir "${SCRIPT_DIR}"

netconvert \
  --node-files "${SCRIPT_DIR}/highway_nodes.nod.xml" \
  --edge-files "${SCRIPT_DIR}/highway_edges.edg.xml" \
  --type-files "${SCRIPT_DIR}/highway_types.typ.xml" \
  --output-file "${SCRIPT_DIR}/map.net.xml" \
  --no-turnarounds true \
  --offset.disable-normalization true \
  --geometry.remove false \
  --junctions.corner-detail 5

python3 "${SCRIPT_DIR}/generate_highway_routes.py" \
    --total "${VEHICLE_COUNT}" \
    --sim-end "${SIM_END}" \
    --loops "${ROUTE_LOOPS}" \
    --output "${SCRIPT_DIR}/cars_highway${VEHICLE_COUNT}.rou.xml"

echo "done."
