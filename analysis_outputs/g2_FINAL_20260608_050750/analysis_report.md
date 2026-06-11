# MoReV2X HPC article-like analysis: G2
Generated: 2026-06-08T10:46:35
## Что посчитано
- PRR/CLR/PLR/HDR/NSR по `ReceivedLog.txt` с каноническим `lossType` mapping.
- PIR и AoI по успешным приёмам `decoded==1` или `lossType==9`.
- Article-like distance bands: 0–250 m и 250–500 m.
- Отдельные срезы CBR50 и CBR95; ось X — η=1/(1−pKeep).
- Для текущего G2 цвет линии означает txPower; для G1 будет numerology µ; для G3 — RRI.

## Быстрый вывод по минимальному AoI
- CBR50, 0_250m: min E[AoI]=8168 ms at tx20 / P=0.8 / η=5.00.
- CBR50, 250_500m: min E[AoI]=8245 ms at tx23 / P=0.4 / η=1.67.
- CBR95, 0_250m: min E[AoI]=8088 ms at tx20 / P=0.8 / η=5.00.
- CBR95, 250_500m: min E[AoI]=8174 ms at tx23 / P=0.4 / η=1.67.

## Матрица рекомендаций (Pareto-минимумы)
| CBR | band | критерий | best value | varied | pKeep | η | AoI_mean | PAoI_p95 | V(200ms) | CLR_broad | CBR p50/p95 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 50 | 0_250m | min_E[AoI] | 8168 | tx20 | 0.8 | 5.00 | 8168 | 200.0 | 0.6202 | 0.0033 | 0.480/0.550 |
| 50 | 0_250m | min_PAoI_p95 | 200.0 | tx17 | 0.0 | 1.00 | 8182 | 200.0 | 0.6220 | 0.0087 | 0.440/0.510 |
| 50 | 0_250m | min_V(A_th=200ms) | 0.6197 | tx23 | 0.8 | 5.00 | 8184 | 200.0 | 0.6197 | 0.0014 | 0.500/0.560 |
| 50 | 0_250m | min_CLR_broad | 0.0014 | tx23 | 0.8 | 5.00 | 8184 | 200.0 | 0.6197 | 0.0014 | 0.500/0.560 |
| 50 | 250_500m | min_E[AoI] | 8245 | tx23 | 0.4 | 1.67 | 8245 | 200.0 | 0.6076 | 0.0204 | 0.490/0.560 |
| 50 | 250_500m | min_PAoI_p95 | 200.0 | tx23 | 0.4 | 1.67 | 8245 | 200.0 | 0.6076 | 0.0204 | 0.490/0.560 |
| 50 | 250_500m | min_V(A_th=200ms) | 0.6071 | tx23 | 0.8 | 5.00 | 8249 | 200.0 | 0.6071 | 0.0198 | 0.500/0.560 |
| 50 | 250_500m | min_CLR_broad | 0.0198 | tx23 | 0.8 | 5.00 | 8249 | 200.0 | 0.6071 | 0.0198 | 0.500/0.560 |
| 95 | 0_250m | min_E[AoI] | 8088 | tx20 | 0.8 | 5.00 | 8088 | 100.0 | 0.6162 | 0.0050 | 0.910/0.940 |
| 95 | 0_250m | min_PAoI_p95 | 100.0 | tx17 | 0.4 | 1.67 | 8092 | 100.0 | 0.6162 | 0.0097 | 0.900/0.940 |
| 95 | 0_250m | min_V(A_th=200ms) | 0.6162 | tx20 | 0.4 | 1.67 | 8095 | 100.0 | 0.6162 | 0.0049 | 0.920/0.950 |
| 95 | 0_250m | min_CLR_broad | 0.0034 | tx23 | 0.8 | 5.00 | 8095 | 100.0 | 0.6162 | 0.0034 | 0.920/0.950 |
| 95 | 250_500m | min_E[AoI] | 8174 | tx23 | 0.4 | 1.67 | 8174 | 100.0 | 0.6013 | 0.0265 | 0.920/0.950 |
| 95 | 250_500m | min_PAoI_p95 | 100.0 | tx20 | 0.8 | 5.00 | 8183 | 100.0 | 0.6023 | 0.0556 | 0.910/0.940 |
| 95 | 250_500m | min_V(A_th=200ms) | 0.6013 | tx23 | 0.4 | 1.67 | 8174 | 100.0 | 0.6013 | 0.0265 | 0.920/0.950 |
| 95 | 250_500m | min_CLR_broad | 0.0251 | tx23 | 0.0 | 1.00 | 8184 | 130.0 | 0.6014 | 0.0251 | 0.950/0.980 |

Подробная версия — `recommendation_matrix.csv`.

## Рекомендованные графики для диплома
### Основные (line vs η)
1. `*_aoi_mean_ms_vs_eta.svg`: главный график AoI vs persistence.
2. `*_paoi_p95_ms_vs_eta.svg`, `*_paoi_p99_ms_vs_eta.svg`: хвост AoI (safety-критично).
3. `*_vth_aoi_100ms_vs_eta.svg`, `*_vth_aoi_200ms_vs_eta.svg`: V(A_th) — доля времени, когда AoI>A_th.
4. `*_pir_mean_ms_vs_eta.svg`: PIR / регулярность обновлений.
5. `*_prr_vs_eta.svg`, `*_clr_vs_eta.svg`, `*_plr_vs_eta.svg`, `*_clr_broad_vs_eta.svg`: механизм деградации.
6. `*_prr_conditional_audible_vs_eta.svg`: PRR при условии «пакет слышим» (отделяет дальность от MAC).
7. `*_blackout_gt_2rri_ratio_vs_eta.svg`: safety-oriented риск пропуска обновлений.

### Защитные (новые)
8. `*_{prr,clr,plr,clr_broad,nsr,hdr,prr_conditional}_vs_distance.svg`: KPI vs distance с шагом 50 m до 500 m, **без аггрегации по pKeep** — прямой ответ на вопрос «как зависит от расстояния».
9. `*_pareto_aoi_vs_paoi95.svg`: Pareto-фронт mean-vs-tail (визуально показывает, где compromise).
10. `*_heatmap_aoi_mean_ms.svg` и `*_heatmap_paoi_p95_ms.svg`: 2D-поверхность pKeep × varied_value.
11. `*_loss_mix.svg`: stacked-bar декомпозиция success/HDR/NSR/PLR/CLR — даёт ответ «какая компонента доминирует».
12. `*_cbr_actual_vs_target.svg`: калибровочный график — проверяет, что фактический CBR (TS 38.215 §5.1.30) попадает в target 50/95 %.
13. `*_ccdf_paoi.svg`: CCDF PAoI — log-y хвост распределения для нескольких репрезентативных ячеек.

## Диагностические ограничения
- Если доступен только seed 1, CI95 в `per_cell_metrics.csv` будет нулевым.
- В G2 меняется txPower при фиксированной numerology µ=0; numerology-persistence coupling полностью раскрывается на G1.
- CCDF PAoI использует не более CCDF_MAX_SAMPLES сэмплов IPG на ячейку×band (равномерное прореживание), чтобы не раздувать память.
- В отличие от VaN3Twin, этот расчёт использует детальные MoReV2X PHY/MAC логи, а не только NS_LOG/TraceSource.
