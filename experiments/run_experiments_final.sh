#!/usr/bin/env bash

set -euo pipefail

ALL_GROUPS=(G1 G2 G3)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
MOREV2X_DIR="${REPO_DIR}/morev2x"
TRACE="${REPO_DIR}/assets/highway_oval_80ue_dense/precomputed/sumo_ns2_trace_highway80_100s.tcl"

VEHICLES=80
MCS=13
CHANNEL_BW=20
TX_POWER=23
SAVING_PERIOD=1.0
SIM_TIME=100

RESULTS_BASE="${MOREV2X_DIR}/results/final_$(date +%Y%m%d_%H%M%S)"
mkdir -p "${RESULTS_BASE}"

cd "${MOREV2X_DIR}"

run() {
    local label="$1" mu="$2" subch="$3" rri="$4" cam="$5" harq="$6" tx="$7" pkeep="$8" seed="$9"
    local out="${RESULTS_BASE}/${label}/run${seed}/"
    mkdir -p "${out}"
    echo "[$(date '+%H:%M:%S')] ${label} run${seed}"
    ./waf --run "HIGHWAY_fcd \
        --mobilityTrace=${TRACE} \
        --Vehicles=${VEHICLES} \
        --mcs=${MCS} \
        --Numerology=${mu} \
        --ChannelBW=${CHANNEL_BW} \
        --SubChannel=${subch} \
        --ueTxPower=${tx} \
        --pKeep=${pkeep} \
        --RRI=${rri} \
        --CAMInterval=${cam} \
        --SavingPeriod=${SAVING_PERIOD} \
        --harq=${harq} \
        --runNo=${seed} \
        --simTime=${SIM_TIME} \
        --OutputPath=${out}" \
        > "${out}/log.txt" 2>&1
}

group_G1() {
echo "=== G1: numerology * pKeep * CBR ==="
run G1_mu0_pK0_CBR50  0 100 100 135 1 23 0.0 1
run G1_mu0_pK0_CBR50  0 100 100 135 1 23 0.0 2
run G1_mu0_pK0_CBR50  0 100 100 135 1 23 0.0 3
run G1_mu0_pK0_CBR95  0 100 100 100 2 23 0.0 1
run G1_mu0_pK0_CBR95  0 100 100 100 2 23 0.0 2
run G1_mu0_pK0_CBR95  0 100 100 100 2 23 0.0 3
run G1_mu0_pK4_CBR50  0 100 100 135 1 23 0.4 1
run G1_mu0_pK4_CBR50  0 100 100 135 1 23 0.4 2
run G1_mu0_pK4_CBR50  0 100 100 135 1 23 0.4 3
run G1_mu0_pK4_CBR95  0 100 100 100 2 23 0.4 1
run G1_mu0_pK4_CBR95  0 100 100 100 2 23 0.4 2
run G1_mu0_pK4_CBR95  0 100 100 100 2 23 0.4 3
run G1_mu0_pK8_CBR50  0 100 100 135 1 23 0.8 1
run G1_mu0_pK8_CBR50  0 100 100 135 1 23 0.8 2
run G1_mu0_pK8_CBR50  0 100 100 135 1 23 0.8 3
run G1_mu0_pK8_CBR95  0 100 100 100 2 23 0.8 1
run G1_mu0_pK8_CBR95  0 100 100 100 2 23 0.8 2
run G1_mu0_pK8_CBR95  0 100 100 100 2 23 0.8 3
run G1_mu1_pK0_CBR50  1 50 50 67 1 23 0.0 1
run G1_mu1_pK0_CBR50  1 50 50 67 1 23 0.0 2
run G1_mu1_pK0_CBR50  1 50 50 67 1 23 0.0 3
run G1_mu1_pK0_CBR95  1 50 50 50 2 23 0.0 1
run G1_mu1_pK0_CBR95  1 50 50 50 2 23 0.0 2
run G1_mu1_pK0_CBR95  1 50 50 50 2 23 0.0 3
run G1_mu1_pK4_CBR50  1 50 50 67 1 23 0.4 1
run G1_mu1_pK4_CBR50  1 50 50 67 1 23 0.4 2
run G1_mu1_pK4_CBR50  1 50 50 67 1 23 0.4 3
run G1_mu1_pK4_CBR95  1 50 50 50 2 23 0.4 1
run G1_mu1_pK4_CBR95  1 50 50 50 2 23 0.4 2
run G1_mu1_pK4_CBR95  1 50 50 50 2 23 0.4 3
run G1_mu1_pK8_CBR50  1 50 50 67 1 23 0.8 1
run G1_mu1_pK8_CBR50  1 50 50 67 1 23 0.8 2
run G1_mu1_pK8_CBR50  1 50 50 67 1 23 0.8 3
run G1_mu1_pK8_CBR95  1 50 50 50 2 23 0.8 1
run G1_mu1_pK8_CBR95  1 50 50 50 2 23 0.8 2
run G1_mu1_pK8_CBR95  1 50 50 50 2 23 0.8 3
run G1_mu2_pK0_CBR50  2 20 25 34 1 23 0.0 1
run G1_mu2_pK0_CBR50  2 20 25 34 1 23 0.0 2
run G1_mu2_pK0_CBR50  2 20 25 34 1 23 0.0 3
run G1_mu2_pK0_CBR95  2 20 25 25 2 23 0.0 1
run G1_mu2_pK0_CBR95  2 20 25 25 2 23 0.0 2
run G1_mu2_pK0_CBR95  2 20 25 25 2 23 0.0 3
run G1_mu2_pK4_CBR50  2 20 25 34 1 23 0.4 1
run G1_mu2_pK4_CBR50  2 20 25 34 1 23 0.4 2
run G1_mu2_pK4_CBR50  2 20 25 34 1 23 0.4 3
run G1_mu2_pK4_CBR95  2 20 25 25 2 23 0.4 1
run G1_mu2_pK4_CBR95  2 20 25 25 2 23 0.4 2
run G1_mu2_pK4_CBR95  2 20 25 25 2 23 0.4 3
run G1_mu2_pK8_CBR50  2 20 25 34 1 23 0.8 1
run G1_mu2_pK8_CBR50  2 20 25 34 1 23 0.8 2
run G1_mu2_pK8_CBR50  2 20 25 34 1 23 0.8 3
run G1_mu2_pK8_CBR95  2 20 25 25 2 23 0.8 1
run G1_mu2_pK8_CBR95  2 20 25 25 2 23 0.8 2
run G1_mu2_pK8_CBR95  2 20 25 25 2 23 0.8 3
}

group_G2() {
echo "=== G2: txPower * pKeep * CBR (mu=0) ==="
run G2_tx17_pK0_CBR50  0 100 100 135 1 17 0.0 1
run G2_tx17_pK0_CBR50  0 100 100 135 1 17 0.0 2
run G2_tx17_pK0_CBR50  0 100 100 135 1 17 0.0 3
run G2_tx17_pK0_CBR95  0 100 100 100 2 17 0.0 1
run G2_tx17_pK0_CBR95  0 100 100 100 2 17 0.0 2
run G2_tx17_pK0_CBR95  0 100 100 100 2 17 0.0 3
run G2_tx17_pK4_CBR50  0 100 100 135 1 17 0.4 1
run G2_tx17_pK4_CBR50  0 100 100 135 1 17 0.4 2
run G2_tx17_pK4_CBR50  0 100 100 135 1 17 0.4 3
run G2_tx17_pK4_CBR95  0 100 100 100 2 17 0.4 1
run G2_tx17_pK4_CBR95  0 100 100 100 2 17 0.4 2
run G2_tx17_pK4_CBR95  0 100 100 100 2 17 0.4 3
run G2_tx17_pK8_CBR50  0 100 100 135 1 17 0.8 1
run G2_tx17_pK8_CBR50  0 100 100 135 1 17 0.8 2
run G2_tx17_pK8_CBR50  0 100 100 135 1 17 0.8 3
run G2_tx17_pK8_CBR95  0 100 100 100 2 17 0.8 1
run G2_tx17_pK8_CBR95  0 100 100 100 2 17 0.8 2
run G2_tx17_pK8_CBR95  0 100 100 100 2 17 0.8 3
run G2_tx20_pK0_CBR50  0 100 100 135 1 20 0.0 1
run G2_tx20_pK0_CBR50  0 100 100 135 1 20 0.0 2
run G2_tx20_pK0_CBR50  0 100 100 135 1 20 0.0 3
run G2_tx20_pK0_CBR95  0 100 100 100 2 20 0.0 1
run G2_tx20_pK0_CBR95  0 100 100 100 2 20 0.0 2
run G2_tx20_pK0_CBR95  0 100 100 100 2 20 0.0 3
run G2_tx20_pK4_CBR50  0 100 100 135 1 20 0.4 1
run G2_tx20_pK4_CBR50  0 100 100 135 1 20 0.4 2
run G2_tx20_pK4_CBR50  0 100 100 135 1 20 0.4 3
run G2_tx20_pK4_CBR95  0 100 100 100 2 20 0.4 1
run G2_tx20_pK4_CBR95  0 100 100 100 2 20 0.4 2
run G2_tx20_pK4_CBR95  0 100 100 100 2 20 0.4 3
run G2_tx20_pK8_CBR50  0 100 100 135 1 20 0.8 1
run G2_tx20_pK8_CBR50  0 100 100 135 1 20 0.8 2
run G2_tx20_pK8_CBR50  0 100 100 135 1 20 0.8 3
run G2_tx20_pK8_CBR95  0 100 100 100 2 20 0.8 1
run G2_tx20_pK8_CBR95  0 100 100 100 2 20 0.8 2
run G2_tx20_pK8_CBR95  0 100 100 100 2 20 0.8 3
run G2_tx23_pK0_CBR50  0 100 100 135 1 23 0.0 1
run G2_tx23_pK0_CBR50  0 100 100 135 1 23 0.0 2
run G2_tx23_pK0_CBR50  0 100 100 135 1 23 0.0 3
run G2_tx23_pK0_CBR95  0 100 100 100 2 23 0.0 1
run G2_tx23_pK0_CBR95  0 100 100 100 2 23 0.0 2
run G2_tx23_pK0_CBR95  0 100 100 100 2 23 0.0 3
run G2_tx23_pK4_CBR50  0 100 100 135 1 23 0.4 1
run G2_tx23_pK4_CBR50  0 100 100 135 1 23 0.4 2
run G2_tx23_pK4_CBR50  0 100 100 135 1 23 0.4 3
run G2_tx23_pK4_CBR95  0 100 100 100 2 23 0.4 1
run G2_tx23_pK4_CBR95  0 100 100 100 2 23 0.4 2
run G2_tx23_pK4_CBR95  0 100 100 100 2 23 0.4 3
run G2_tx23_pK8_CBR50  0 100 100 135 1 23 0.8 1
run G2_tx23_pK8_CBR50  0 100 100 135 1 23 0.8 2
run G2_tx23_pK8_CBR50  0 100 100 135 1 23 0.8 3
run G2_tx23_pK8_CBR95  0 100 100 100 2 23 0.8 1
run G2_tx23_pK8_CBR95  0 100 100 100 2 23 0.8 2
run G2_tx23_pK8_CBR95  0 100 100 100 2 23 0.8 3
}

group_G3() {
echo "=== G3: RRI * pKeep * CBR (mu=0) ==="
run G3_rri100_pK0_CBR50  0 100 100 135 1 23 0.0 1
run G3_rri100_pK0_CBR50  0 100 100 135 1 23 0.0 2
run G3_rri100_pK0_CBR50  0 100 100 135 1 23 0.0 3
run G3_rri100_pK0_CBR95  0 100 100 100 2 23 0.0 1
run G3_rri100_pK0_CBR95  0 100 100 100 2 23 0.0 2
run G3_rri100_pK0_CBR95  0 100 100 100 2 23 0.0 3
run G3_rri100_pK4_CBR50  0 100 100 135 1 23 0.4 1
run G3_rri100_pK4_CBR50  0 100 100 135 1 23 0.4 2
run G3_rri100_pK4_CBR50  0 100 100 135 1 23 0.4 3
run G3_rri100_pK4_CBR95  0 100 100 100 2 23 0.4 1
run G3_rri100_pK4_CBR95  0 100 100 100 2 23 0.4 2
run G3_rri100_pK4_CBR95  0 100 100 100 2 23 0.4 3
run G3_rri100_pK8_CBR50  0 100 100 135 1 23 0.8 1
run G3_rri100_pK8_CBR50  0 100 100 135 1 23 0.8 2
run G3_rri100_pK8_CBR50  0 100 100 135 1 23 0.8 3
run G3_rri100_pK8_CBR95  0 100 100 100 2 23 0.8 1
run G3_rri100_pK8_CBR95  0 100 100 100 2 23 0.8 2
run G3_rri100_pK8_CBR95  0 100 100 100 2 23 0.8 3
run G3_rri50_pK0_CBR50   0 100 50 67 1 23 0.0 1
run G3_rri50_pK0_CBR50   0 100 50 67 1 23 0.0 2
run G3_rri50_pK0_CBR50   0 100 50 67 1 23 0.0 3
run G3_rri50_pK0_CBR95   0 100 50 50 2 23 0.0 1
run G3_rri50_pK0_CBR95   0 100 50 50 2 23 0.0 2
run G3_rri50_pK0_CBR95   0 100 50 50 2 23 0.0 3
run G3_rri50_pK4_CBR50   0 100 50 67 1 23 0.4 1
run G3_rri50_pK4_CBR50   0 100 50 67 1 23 0.4 2
run G3_rri50_pK4_CBR50   0 100 50 67 1 23 0.4 3
run G3_rri50_pK4_CBR95   0 100 50 50 2 23 0.4 1
run G3_rri50_pK4_CBR95   0 100 50 50 2 23 0.4 2
run G3_rri50_pK4_CBR95   0 100 50 50 2 23 0.4 3
run G3_rri50_pK8_CBR50   0 100 50 67 1 23 0.8 1
run G3_rri50_pK8_CBR50   0 100 50 67 1 23 0.8 2
run G3_rri50_pK8_CBR50   0 100 50 67 1 23 0.8 3
run G3_rri50_pK8_CBR95   0 100 50 50 2 23 0.8 1
run G3_rri50_pK8_CBR95   0 100 50 50 2 23 0.8 2
run G3_rri50_pK8_CBR95   0 100 50 50 2 23 0.8 3
run G3_rri20_pK0_CBR50   0 100 20 27 1 23 0.0 1
run G3_rri20_pK0_CBR50   0 100 20 27 1 23 0.0 2
run G3_rri20_pK0_CBR50   0 100 20 27 1 23 0.0 3
run G3_rri20_pK0_CBR95   0 100 20 20 2 23 0.0 1
run G3_rri20_pK0_CBR95   0 100 20 20 2 23 0.0 2
run G3_rri20_pK0_CBR95   0 100 20 20 2 23 0.0 3
run G3_rri20_pK4_CBR50   0 100 20 27 1 23 0.4 1
run G3_rri20_pK4_CBR50   0 100 20 27 1 23 0.4 2
run G3_rri20_pK4_CBR50   0 100 20 27 1 23 0.4 3
run G3_rri20_pK4_CBR95   0 100 20 20 2 23 0.4 1
run G3_rri20_pK4_CBR95   0 100 20 20 2 23 0.4 2
run G3_rri20_pK4_CBR95   0 100 20 20 2 23 0.4 3
run G3_rri20_pK8_CBR50   0 100 20 27 1 23 0.8 1
run G3_rri20_pK8_CBR50   0 100 20 27 1 23 0.8 2
run G3_rri20_pK8_CBR50   0 100 20 27 1 23 0.8 3
run G3_rri20_pK8_CBR95   0 100 20 20 2 23 0.8 1
run G3_rri20_pK8_CBR95   0 100 20 20 2 23 0.8 2
run G3_rri20_pK8_CBR95   0 100 20 20 2 23 0.8 3
}

all_groups() {
    declare -F | awk '{print $3}' | grep '^group_' | sed 's/^group_//'
}

if [[ $# -eq 0 ]]; then
    echo "No groups specified, running all: $(all_groups | tr '\n' ' ')"
    for g in $(all_groups); do
        "group_${g}"
    done
else
    for g in "$@"; do
        if declare -F "group_${g}" > /dev/null 2>&1; then
            "group_${g}"
        else
            echo "unknown group: $g (available: $(all_groups | tr '\n' ' '))" >&2
        fi
    done
fi

echo "Final results in ${RESULTS_BASE}"
