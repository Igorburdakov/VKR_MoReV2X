#!/usr/bin/env bash
# Параллельный запуск самых тяжёлых конфигураций (G1 mu=2, G3 RRI=20).
# Эти конфигурации генерируют максимальное число событий и считаются
# в 5-20× дольше обычных (mu=0, RRI=100).
#
# Использование:
#   bash experiments/run_heavy_parallel.sh          # по умолчанию 4 параллельных
#   bash experiments/run_heavy_parallel.sh 8        # 8 параллельных процессов
#   SEEDS="1 2 3" bash experiments/run_heavy_parallel.sh 6

set -euo pipefail

MAX_PARALLEL="${1:-4}"
SEEDS="${SEEDS:-1 2 3}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
MOREV2X_DIR="${REPO_DIR}/morev2x"
TRACE="${REPO_DIR}/assets/highway_oval_80ue_dense/precomputed/sumo_ns2_trace_highway80_100s.tcl"

VEHICLES=80
MCS=13
CHANNEL_BW=20
SAVING_PERIOD=1.0
SIM_TIME=100

RESULTS_BASE="${MOREV2X_DIR}/results/heavy_$(date +%Y%m%d_%H%M%S)"
mkdir -p "${RESULTS_BASE}"

cd "${MOREV2X_DIR}"
RUNNING=0

# Семафор: ждать если уже MAX_PARALLEL процессов запущено
throttle() {
    while (( RUNNING >= MAX_PARALLEL )); do
        wait -n 2>/dev/null || true
        RUNNING=$((RUNNING - 1))
    done
}

run_parallel() {
    local label="$1" mu="$2" subch="$3" rri="$4" cam="$5" harq="$6" tx="$7" pkeep="$8" run_no="$9"
    local out="${RESULTS_BASE}/${label}/run${run_no}/"
    mkdir -p "${out}"

    throttle

    (
        echo "[$(date '+%H:%M:%S')] START ${label} run${run_no}"
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
            --runNo=${run_no} \
            --simTime=${SIM_TIME} \
            --OutputPath=${out}" \
            > "${out}/log.txt" 2>&1
        echo "[$(date '+%H:%M:%S')] DONE  ${label} run${run_no}"
    ) &
    RUNNING=$((RUNNING + 1))
}

echo "=== Heavy parallel runs (max ${MAX_PARALLEL} jobs) ==="
echo "Seeds: ${SEEDS}"
echo "Results: ${RESULTS_BASE}"
echo ""

# --- G1 mu=2: numerology=2, SubChannel=20, RRI=25, все seeds ---
echo "=== G1 mu=2 (numerology=2, harq=1/2) ==="
for s in ${SEEDS}; do
    run_parallel G1_mu2_pK0_CBR50  2 20 25 34 1 23 0.0 "${s}"
    run_parallel G1_mu2_pK0_CBR95  2 20 25 25 2 23 0.0 "${s}"
    run_parallel G1_mu2_pK4_CBR50  2 20 25 34 1 23 0.4 "${s}"
    run_parallel G1_mu2_pK4_CBR95  2 20 25 25 2 23 0.4 "${s}"
    run_parallel G1_mu2_pK8_CBR50  2 20 25 34 1 23 0.8 "${s}"
    run_parallel G1_mu2_pK8_CBR95  2 20 25 25 2 23 0.8 "${s}"
done

# --- G3 RRI=20: самый частый интервал передачи ---
echo "=== G3 RRI=20 (mu=0, SubChannel=100, RRI=20ms) ==="
for s in ${SEEDS}; do
    run_parallel G3_rri20_pK0_CBR50  0 100 20 27 1 23 0.0 "${s}"
    run_parallel G3_rri20_pK0_CBR95  0 100 20 20 2 23 0.0 "${s}"
    run_parallel G3_rri20_pK4_CBR50  0 100 20 27 1 23 0.4 "${s}"
    run_parallel G3_rri20_pK4_CBR95  0 100 20 20 2 23 0.4 "${s}"
    run_parallel G3_rri20_pK8_CBR50  0 100 20 27 1 23 0.8 "${s}"
    run_parallel G3_rri20_pK8_CBR95  0 100 20 20 2 23 0.8 "${s}"
done

# --- G1 mu=1 CBR95: средне-тяжёлые ---
echo "=== G1 mu=1 CBR95 (numerology=1, harq=2) ==="
for s in ${SEEDS}; do
    run_parallel G1_mu1_pK0_CBR95  1 50 50 50 2 23 0.0 "${s}"
    run_parallel G1_mu1_pK4_CBR95  1 50 50 50 2 23 0.4 "${s}"
    run_parallel G1_mu1_pK8_CBR95  1 50 50 50 2 23 0.8 "${s}"
done

# --- G3 RRI=50: средне-тяжёлые ---
echo "=== G3 RRI=50 (mu=0, RRI=50ms) ==="
for s in ${SEEDS}; do
    run_parallel G3_rri50_pK0_CBR95  0 100 50 50 2 23 0.0 "${s}"
    run_parallel G3_rri50_pK4_CBR95  0 100 50 50 2 23 0.4 "${s}"
    run_parallel G3_rri50_pK8_CBR95  0 100 50 50 2 23 0.8 "${s}"
done

echo ""
echo "Waiting for all ${RUNNING} remaining jobs..."
wait
echo "=== ALL DONE: ${RESULTS_BASE} ==="

