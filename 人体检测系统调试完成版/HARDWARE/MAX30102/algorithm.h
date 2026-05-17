#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

/* 移植自 Maxim MAXREFDES117# 官方参考算法 (algorithm.cpp)
 * 适配纯 C / Keil ARMCC V5 / GB2312 注释
 */

#include "stm32f10x.h"

#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif

#define FS              25               /* 采样率 25Hz (MAX30102 物理 100Hz, FIFO 4x 平均 -> 25Hz) */
#define BUFFER_SIZE     (FS * 5)         /* 缓冲 5 秒 = 125 样本 */
#define HR_FIFO_SIZE    7
#define MA4_SIZE        4                /* 4 点移动平均，请勿改          */
#define HAMMING_SIZE    5                /* Hamming 窗长，请勿改          */
#define MIN_PEAK_HEIGHT 0
#define MIN_DISTANCE    8
#define ALG_MAX(x,y)    ((x) > (y) ? (x) : (y))
#define ALG_MIN(x,y)    ((x) < (y) ? (x) : (y))

void maxim_heart_rate_and_oxygen_saturation(
        uint32_t *pun_ir_buffer , int32_t n_ir_buffer_length,
        uint32_t *pun_red_buffer,
        int32_t *pn_spo2 , int8_t *pch_spo2_valid,
        int32_t *pn_heart_rate, int8_t *pch_hr_valid);

void maxim_find_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
        int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
        int32_t n_size, int32_t n_min_height);
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
        int32_t n_min_distance);
void maxim_sort_ascend(int32_t *pn_x, int32_t n_size);
void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size);

#endif /* __ALGORITHM_H__ */