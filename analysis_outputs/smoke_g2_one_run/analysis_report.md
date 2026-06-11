# MoReV2X HPC article-like analysis: G2
Generated: 2026-06-06T19:34:02
## Что посчитано
- PRR/CLR/PLR/HDR/NSR по `ReceivedLog.txt` с каноническим `lossType` mapping.
- PIR и AoI по успешным приёмам `decoded==1` или `lossType==9`.
- Article-like distance bands: 0–250 m и 250–500 m.
- Отдельные срезы CBR50 и CBR95; ось X — η=1/(1−pKeep).
- Для текущего G2 цвет линии означает txPower; для G1 будет numerology µ; для G3 — RRI.

## Быстрый вывод по минимальному AoI
- CBR50, 0_250m: min E[AoI]=8228 ms at tx17 / P=0.0 / η=1.00.
- CBR50, 250_500m: min E[AoI]=7600 ms at tx17 / P=0.0 / η=1.00.

## Рекомендованные графики для диплома
1. `*_aoi_mean_ms_vs_eta.svg`: главный график AoI vs persistence.
2. `*_pir_mean_ms_vs_eta.svg`: Peak-AoI/PIR регулярность обновлений.
3. `*_prr_vs_eta.svg` + `*_clr_vs_eta.svg` + `*_plr_vs_eta.svg`: механизм деградации AoI.
4. `*_blackout_gt_2rri_ratio_vs_eta.svg`: safety-oriented риск пропуска обновлений.

## Диагностические ограничения
- Доступны только короткие seed-1 результаты, поэтому CI95 в `per_cell_metrics.csv` будет нулевым до появления seed 2/3.
- В G2 меняется txPower при фиксированной numerology µ=0; numerology-persistence coupling полностью раскрывается на G1.
- В отличие от VaN3Twin, этот расчёт использует детальные MoReV2X PHY/MAC логи, а не только NS_LOG/TraceSource.
