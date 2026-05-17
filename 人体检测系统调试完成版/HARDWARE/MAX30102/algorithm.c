/* algorithm.c
 * 移植自 Maxim MAXREFDES117# 官方参考算法 (algorithm.cpp)
 * 由 C++ 改为纯 C，适配 Keil ARMCC V5 / GB2312 编码。
 *
 * 命名约定保持与 Maxim 原版一致：
 *   un_ / pun_   : uint32_t
 *   n_  / pn_    : int32_t
 *   an_          : int32_t 数组
 *   ch_ / pch_   : int8_t
 *   uch_ / puch_ : uint8_t
 *   auw_         : uint16_t 数组
 */

#include "algorithm.h"

/* Hamming 窗系数：Hamm = int16(512 * hamming(5)') */
static const uint16_t auw_hamm[31] = { 41, 276, 512, 276, 41 };

/* SpO2 查表：-45.060 * R^2 + 30.354 * R + 94.845 (R 范围 0..183) */
static const uint8_t uch_spo2_table[184] = {
    95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
    99, 99, 99, 99,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
   100,100,100,100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
    97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
    90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
    80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
    66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
    28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10,  9,  7,  6,  5,
     3,  2,  1
};

/* 大数组放到 .bss 段而非栈上 (BUFFER_SIZE=500, 共约 6KB) */
static int32_t an_dx[BUFFER_SIZE - MA4_SIZE];
static int32_t an_x [BUFFER_SIZE];
static int32_t an_y [BUFFER_SIZE];

/**
 * 主算法：基于 IR/RED 缓冲计算心率与 SpO2。
 *   - 心率：对 IR 做 4 点滑动平均 + 一阶差分 + Hamming 窗，
 *     在差分波形上检测峰值，峰间隔 → BPM。
 *   - SpO2：在原始 IR/RED 上找谷间最大值得到 AC/DC，求比值 R，
 *     再查 uch_spo2_table 得百分比。
 */
void maxim_heart_rate_and_oxygen_saturation(
        uint32_t *pun_ir_buffer , int32_t n_ir_buffer_length,
        uint32_t *pun_red_buffer,
        int32_t *pn_spo2 , int8_t *pch_spo2_valid,
        int32_t *pn_heart_rate, int8_t *pch_hr_valid)
{
    uint32_t un_ir_mean, un_only_once;
    int32_t  k, n_i_ratio_count;
    int32_t  i, s, m, n_exact_ir_valley_locs_count, n_middle_idx;
    int32_t  n_th1, n_npks, n_c_min;
    int32_t  an_ir_valley_locs[15];
    int32_t  an_exact_ir_valley_locs[15];
    int32_t  an_dx_peak_locs[15];
    int32_t  n_peak_interval_sum;

    int32_t  n_y_ac, n_x_ac;
    int32_t  n_spo2_calc;
    int32_t  n_y_dc_max, n_x_dc_max;
    int32_t  n_y_dc_max_idx = 0, n_x_dc_max_idx = 0;
    int32_t  an_ratio[5], n_ratio_average;
    int32_t  n_nume, n_denom;

    /* 1. 去除 IR 直流分量 */
    un_ir_mean = 0;
    for(k = 0; k < n_ir_buffer_length; k++) un_ir_mean += pun_ir_buffer[k];
    un_ir_mean = un_ir_mean / n_ir_buffer_length;
    for(k = 0; k < n_ir_buffer_length; k++) an_x[k] = pun_ir_buffer[k] - un_ir_mean;

    /* 2. 4 点滑动平均 */
    for(k = 0; k < BUFFER_SIZE - MA4_SIZE; k++)
    {
        n_denom = an_x[k] + an_x[k+1] + an_x[k+2] + an_x[k+3];
        an_x[k] = n_denom / (int32_t)4;
    }

    /* 3. 一阶差分 */
    for(k = 0; k < BUFFER_SIZE - MA4_SIZE - 1; k++)
        an_dx[k] = an_x[k+1] - an_x[k];

    /* 4. 2 点滑动平均 */
    for(k = 0; k < BUFFER_SIZE - MA4_SIZE - 2; k++)
        an_dx[k] = (an_dx[k] + an_dx[k+1]) / 2;

    /* 5. Hamming 窗滤波（对差分做翻转卷积，以便用峰值检测找谷点） */
    for(i = 0; i < BUFFER_SIZE - HAMMING_SIZE - MA4_SIZE - 2; i++)
    {
        s = 0;
        for(k = i; k < i + HAMMING_SIZE; k++)
            s -= an_dx[k] * auw_hamm[k - i];
        an_dx[i] = s / (int32_t)1146;   /* 1146 = sum(auw_hamm) */
    }

    /* 6. 自适应阈值 = 差分序列的平均绝对值 */
    n_th1 = 0;
    for(k = 0; k < BUFFER_SIZE - HAMMING_SIZE; k++)
        n_th1 += (an_dx[k] > 0) ? an_dx[k] : ((int32_t)0 - an_dx[k]);
    n_th1 = n_th1 / (BUFFER_SIZE - HAMMING_SIZE);

    /* 7. 峰值检测 */
    maxim_find_peaks(an_dx_peak_locs, &n_npks, an_dx,
                     BUFFER_SIZE - HAMMING_SIZE, n_th1, 4, 5);  /* min_dist=4 @25Hz = 160ms */

    /* 8. 心率 = 60 * Fs / 平均峰间隔。Fs=100, 100/平均 = bpm 因子。
     *    原代码用 6000 / 间隔。这里 Fs=100 时同样适用。 */
    n_peak_interval_sum = 0;
    if(n_npks >= 2)
    {
        for(k = 1; k < n_npks; k++)
            n_peak_interval_sum += (an_dx_peak_locs[k] - an_dx_peak_locs[k - 1]);
        n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1);
        *pn_heart_rate = (int32_t)(1500 / n_peak_interval_sum);  /* 60s * FS(25Hz) */
        *pch_hr_valid  = 1;
    }
    else
    {
        *pn_heart_rate = -999;
        *pch_hr_valid  = 0;
    }

    for(k = 0; k < n_npks; k++)
        an_ir_valley_locs[k] = an_dx_peak_locs[k] + HAMMING_SIZE / 2;

    /* 9. 准备 SpO2 计算：使用原始 IR/RED 信号 */
    for(k = 0; k < n_ir_buffer_length; k++)
    {
        an_x[k] = pun_ir_buffer[k];
        an_y[k] = pun_red_buffer[k];
    }

    /* 10. 在每个粗略谷点附近 ±5 范围搜索精确谷点 */
    n_exact_ir_valley_locs_count = 0;
    for(k = 0; k < n_npks; k++)
    {
        un_only_once = 1;
        m = an_ir_valley_locs[k];
        n_c_min = 16777216;     /* 2^24 */
        if(m + 5 < BUFFER_SIZE - HAMMING_SIZE && m - 5 > 0)
        {
            for(i = m - 5; i < m + 5; i++)
                if(an_x[i] < n_c_min)
                {
                    if(un_only_once > 0) un_only_once = 0;
                    n_c_min = an_x[i];
                    an_exact_ir_valley_locs[k] = i;
                }
            if(un_only_once == 0)
                n_exact_ir_valley_locs_count++;
        }
    }

    if(n_exact_ir_valley_locs_count < 2)
    {
        *pn_spo2        = -999;
        *pch_spo2_valid = 0;
        return;
    }

    /* 11. 对 IR/RED 再做 4 点滑动平均 */
    for(k = 0; k < BUFFER_SIZE - MA4_SIZE; k++)
    {
        an_x[k] = (an_x[k] + an_x[k+1] + an_x[k+2] + an_x[k+3]) / (int32_t)4;
        an_y[k] = (an_y[k] + an_y[k+1] + an_y[k+2] + an_y[k+3]) / (int32_t)4;
    }

    /* 12. 在相邻两个精确谷点之间寻找峰值，计算 AC/DC */
    n_ratio_average = 0;
    n_i_ratio_count = 0;
    for(k = 0; k < 5; k++) an_ratio[k] = 0;

    for(k = 0; k < n_exact_ir_valley_locs_count; k++)
    {
        if(an_exact_ir_valley_locs[k] > BUFFER_SIZE)
        {
            *pn_spo2        = -999;
            *pch_spo2_valid = 0;
            return;
        }
    }

    for(k = 0; k < n_exact_ir_valley_locs_count - 1; k++)
    {
        n_y_dc_max = -16777216;
        n_x_dc_max = -16777216;
        if(an_exact_ir_valley_locs[k+1] - an_exact_ir_valley_locs[k] > 10)
        {
            for(i = an_exact_ir_valley_locs[k]; i < an_exact_ir_valley_locs[k+1]; i++)
            {
                if(an_x[i] > n_x_dc_max) { n_x_dc_max = an_x[i]; n_x_dc_max_idx = i; }
                if(an_y[i] > n_y_dc_max) { n_y_dc_max = an_y[i]; n_y_dc_max_idx = i; }
            }
            /* 线性插值 DC，再相减得到 AC */
            n_y_ac = (an_y[an_exact_ir_valley_locs[k+1]] - an_y[an_exact_ir_valley_locs[k]])
                    * (n_y_dc_max_idx - an_exact_ir_valley_locs[k]);
            n_y_ac = an_y[an_exact_ir_valley_locs[k]] +
                     n_y_ac / (an_exact_ir_valley_locs[k+1] - an_exact_ir_valley_locs[k]);
            n_y_ac = an_y[n_y_dc_max_idx] - n_y_ac;

            n_x_ac = (an_x[an_exact_ir_valley_locs[k+1]] - an_x[an_exact_ir_valley_locs[k]])
                    * (n_x_dc_max_idx - an_exact_ir_valley_locs[k]);
            n_x_ac = an_x[an_exact_ir_valley_locs[k]] +
                     n_x_ac / (an_exact_ir_valley_locs[k+1] - an_exact_ir_valley_locs[k]);
            n_x_ac = an_x[n_y_dc_max_idx] - n_x_ac;

            n_nume  = (n_y_ac * n_x_dc_max) >> 7;
            n_denom = (n_x_ac * n_y_dc_max) >> 7;
            if(n_denom > 0 && n_i_ratio_count < 5 && n_nume != 0)
            {
                an_ratio[n_i_ratio_count] = (n_nume * 100) / n_denom;
                n_i_ratio_count++;
            }
        }
    }

    /* 13. 取比值中位数 */
    maxim_sort_ascend(an_ratio, n_i_ratio_count);
    n_middle_idx = n_i_ratio_count / 2;

    if(n_middle_idx > 1)
        n_ratio_average = (an_ratio[n_middle_idx - 1] + an_ratio[n_middle_idx]) / 2;
    else
        n_ratio_average = an_ratio[n_middle_idx];

    if(n_ratio_average > 2 && n_ratio_average < 184)
    {
        n_spo2_calc      = uch_spo2_table[n_ratio_average];
        *pn_spo2         = n_spo2_calc;
        *pch_spo2_valid  = 1;
    }
    else
    {
        *pn_spo2         = -999;
        *pch_spo2_valid  = 0;
    }
}

/**
 * 在 pn_x 中查找最多 n_max_num 个，高度 ≥ n_min_height 且相邻间距 ≥ n_min_distance 的峰位。
 */
void maxim_find_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
                      int32_t n_size, int32_t n_min_height,
                      int32_t n_min_distance, int32_t n_max_num)
{
    maxim_peaks_above_min_height(pn_locs, pn_npks, pn_x, n_size, n_min_height);
    maxim_remove_close_peaks(pn_locs, pn_npks, pn_x, n_min_distance);
    *pn_npks = ALG_MIN(*pn_npks, n_max_num);
}

/**
 * 找出所有高度大于 n_min_height 的峰值（含平顶处理）。
 */
void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
                                  int32_t n_size, int32_t n_min_height)
{
    int32_t i = 1, n_width;
    *pn_npks = 0;

    while(i < n_size - 1)
    {
        if(pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1])
        {
            n_width = 1;
            while(i + n_width < n_size && pn_x[i] == pn_x[i + n_width])
                n_width++;
            if(pn_x[i] > pn_x[i + n_width] && (*pn_npks) < 15)
            {
                pn_locs[(*pn_npks)++] = i;
                i += n_width + 1;
            }
            else
                i += n_width;
        }
        else
            i++;
    }
}

/**
 * 删除相互间距小于 n_min_distance 的峰，保留较高者。
 */
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x,
                              int32_t n_min_distance)
{
    int32_t i, j, n_old_npks, n_dist;

    maxim_sort_indices_descend(pn_x, pn_locs, *pn_npks);

    for(i = -1; i < *pn_npks; i++)
    {
        n_old_npks = *pn_npks;
        *pn_npks = i + 1;
        for(j = i + 1; j < n_old_npks; j++)
        {
            n_dist = pn_locs[j] - (i == -1 ? -1 : pn_locs[i]);
            if(n_dist > n_min_distance || n_dist < -n_min_distance)
                pn_locs[(*pn_npks)++] = pn_locs[j];
        }
    }
    maxim_sort_ascend(pn_locs, *pn_npks);
}

/**
 * 插入排序（升序）。
 */
void maxim_sort_ascend(int32_t *pn_x, int32_t n_size)
{
    int32_t i, j, n_temp;
    for(i = 1; i < n_size; i++)
    {
        n_temp = pn_x[i];
        for(j = i; j > 0 && n_temp < pn_x[j-1]; j--)
            pn_x[j] = pn_x[j-1];
        pn_x[j] = n_temp;
    }
}

/**
 * 按 pn_x[pn_indx[i]] 降序，对索引数组 pn_indx 做插入排序。
 */
void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size)
{
    int32_t i, j, n_temp;
    for(i = 1; i < n_size; i++)
    {
        n_temp = pn_indx[i];
        for(j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
            pn_indx[j] = pn_indx[j-1];
        pn_indx[j] = n_temp;
    }
}