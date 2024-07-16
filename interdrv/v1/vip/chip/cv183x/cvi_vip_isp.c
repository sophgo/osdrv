/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_isp.c
 * Description: video pipeline isp driver
 */

#include <linux/streamline_annotate.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif
#include "cvi_vip_isp.h"
#include "cvi_vip_isp_proc.h"
#include "cvi_vip_vi_proc.h"
#include "tee_cv_private.h"

#define ISP_TUN_CFG(_name) \
	{\
		struct cvi_vip_isp_##_name##_config *cfg;\
		cfg = (struct cvi_vip_isp_##_name##_config *)ext_ctrls[i].ptr;\
		ispblk_##_name##_tun_cfg(ctx, cfg);\
		rc = 0;\
	}

#define ISP_RUNTIME_TUN(_name) \
	{\
		struct cvi_vip_isp_##_name##_config *cfg;\
		cfg = &post_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg);\
	}


#define VI_DBG_PROC_NAME "cvitek/vi_dbg"
#define VI_SHARE_MEM_SIZE     (0x2000)

//#define ISP_PERF_MEASURE
#ifdef ISP_PERF_MEASURE
#define ISP_MEASURE_FRM 150
#define STOUS 1000000

struct isp_postraw_measure {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 time;
#else
	struct timeval time;
#endif
	u32 cnt;
};

struct isp_perf_chk {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 sof_time[ISP_MEASURE_FRM];
	struct timespec64 preraw_eof[ISP_MEASURE_FRM];
#else
	struct timeval sof_time[ISP_MEASURE_FRM];
	struct timeval preraw_eof[ISP_MEASURE_FRM];
#endif
	struct isp_postraw_measure postraw_str[ISP_MEASURE_FRM];
	struct isp_postraw_measure postraw_eof[ISP_MEASURE_FRM];
	u32 postraw_trg_cnt;
	u8 sof_end;
	u8 preraw_end;
	u8 postraw_end;
};

static struct isp_perf_chk time_chk;
#endif //ISP_PERF_MEASURE

struct ip_info ip_info_list[IP_INFO_ID_MAX] = {
	//Preraw_0
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRERAW0, sizeof(struct REG_PRE_RAW_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG0, sizeof(struct REG_ISP_CSI_BDG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP0, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP1, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC0, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC1, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCR0, sizeof(struct REG_ISP_LSCR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCR1, sizeof(struct REG_ISP_LSCR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AEHIST0, sizeof(struct REG_ISP_AE_HIST_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AEHIST1, sizeof(struct REG_ISP_AE_HIST_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AWB0, sizeof(struct REG_ISP_AWB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AWB1, sizeof(struct REG_ISP_AWB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AF, sizeof(struct REG_ISP_AF_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AF_GAMMA, sizeof(struct REG_ISP_AF_GAMMA_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_GMS, sizeof(struct REG_ISP_GMS_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG0, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG1, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LMP0, sizeof(struct REG_ISP_LMAP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP0, sizeof(struct REG_ISP_RGBMAP_T)},
	//Preraw_1
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRERAW1_R1, sizeof(struct REG_PRE_RAW_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG1_R1, sizeof(struct REG_ISP_CSI_BDG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP0_R1, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP1_R1, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC0_R1, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC1_R1, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCR2_R1, sizeof(struct REG_ISP_LSCR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCR3_R1, sizeof(struct REG_ISP_LSCR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AEHIST0_R1, sizeof(struct REG_ISP_AE_HIST_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AEHIST1_R1, sizeof(struct REG_ISP_AE_HIST_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AWB0_R1, sizeof(struct REG_ISP_AWB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AWB1_R1, sizeof(struct REG_ISP_AWB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AF_R1, sizeof(struct REG_ISP_AF_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AF_GAMMA_R1, sizeof(struct REG_ISP_AF_GAMMA_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_GMS_R1, sizeof(struct REG_ISP_GMS_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG0_R1, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG1_R1, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LMP2_R1, sizeof(struct REG_ISP_LMAP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP2_R1, sizeof(struct REG_ISP_RGBMAP_T)},
	//Rawtop
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAWTOP, sizeof(struct REG_RAW_TOP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC2, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BLC3, sizeof(struct REG_ISP_BLC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DPC0, sizeof(struct REG_ISP_DPC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DPC1, sizeof(struct REG_ISP_DPC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG2, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG3, sizeof(struct REG_ISP_WBG_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCM0, sizeof(struct REG_ISP_LSC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSCM1, sizeof(struct REG_ISP_LSC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AWB4, sizeof(struct REG_ISP_AWB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_HDRFUSION, sizeof(struct REG_ISP_FUSION_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_HDRLTM, sizeof(struct REG_ISP_LTM_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BNR, sizeof(struct REG_ISP_BNR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP2, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP3, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_MANR, sizeof(struct REG_ISP_MM_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_FPN0, sizeof(struct REG_ISP_FPN_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_FPN1, sizeof(struct REG_ISP_FPN_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WBG4, sizeof(struct REG_ISP_WBG_T)},
	//Rgbtop
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBTOP, sizeof(struct REG_ISP_RGB_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CFA, sizeof(struct REG_ISP_CFA_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CCM, sizeof(struct REG_ISP_CCM_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_GAMMA, sizeof(struct REG_ISP_GAMMA_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_HSV, sizeof(struct REG_ISP_HSV_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DHZ, sizeof(struct REG_ISP_DHZ_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_R2Y4, sizeof(struct REG_ISP_CSC_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBDITHER, sizeof(struct REG_ISP_RGBDITHER_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBEE, sizeof(struct REG_ISP_RGBEE_T)},
	//Yuvtop
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YUVTOP, sizeof(struct REG_YUV_TOP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_444422, sizeof(struct REG_ISP_444_422_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_UVDITHER, sizeof(struct REG_ISP_YUV_DITHER_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YNR, sizeof(struct REG_ISP_YNR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CNR, sizeof(struct REG_ISP_CNR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_EE, sizeof(struct REG_ISP_EE_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YCURVE, sizeof(struct REG_ISP_YCUR_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP4, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP5, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CROP6, sizeof(struct REG_ISP_CROP_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DCI, sizeof(struct REG_ISP_DCI_T)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_ISPTOP, sizeof(struct REG_ISP_TOP_T)},
};

uint8_t af_gamma_data[] = {
0,   20,   28,   33,   38,   42,   46,   49,   52,   55,   58,   61,   63,   66,   68,   70,
72,   74,   76,   78,   80,   82,   83,   85,   87,   88,   90,   92,   93,   95,   96,   98,
99,  100,  102,  103,  104,  106,  107,  108,  110,  111,  112,  113,  114,  116,  117,  118,
119,  120,  121,  122,  124,  125,  126,  127,  128,  129,  130,  131,  132,  133,  134,  135,
136,  137,  138,  139,  140,  141,  141,  142,  143,  144,  145,  146,  147,  148,  149,  150,
150,  151,  152,  153,  154,  155,  155,  156,  157,  158,  159,  159,  160,  161,  162,  163,
163,  164,  165,  166,  166,  167,  168,  169,  169,  170,  171,  172,  172,  173,  174,  175,
175,  176,  177,  177,  178,  179,  180,  180,  181,  182,  182,  183,  184,  184,  185,  186,
186,  187,  188,  188,  189,  190,  190,  191,  192,  192,  193,  193,  194,  195,  195,  196,
197,  197,  198,  198,  199,  200,  200,  201,  201,  202,  203,  203,  204,  204,  205,  206,
206,  207,  207,  208,  209,  209,  210,  210,  211,  211,  212,  213,  213,  214,  214,  215,
215,  216,  217,  217,  218,  218,  219,  219,  220,  220,  221,  221,  222,  223,  223,  224,
224,  225,  225,  226,  226,  227,  227,  228,  228,  229,  229,  230,  230,  231,  231,  232,
232,  233,  233,  234,  234,  235,  235,  236,  236,  237,  237,  238,  238,  239,  239,  240,
240,  241,  241,  242,  242,  243,  243,  244,  244,  245,  245,  246,  246,  247,  247,  248,
248,  249,  249,  250,  250,  250,  251,  251,  252,  252,  253,  253,  254,  254,  255,  255,
255
};

uint16_t ycur_data[] = {
0,    4,    8,   12,   16,   20,   24,   28,   32,   36,   40,   44,   48,   52,   56,   60,
64,   68,   72,   76,   80,   84,   88,   92,   96,  100,  104,  108,  112,  116,  120,  124,
128,  132,  136,  140,  144,  148,  152,  156,  160,  164,  168,  172,  176,  180,  184,  188,
192,  196,  200,  204,  208,  212,  216,  220,  224,  228,  232,  236,  240,  244,  248,  252,
1023,
};

uint16_t ltm_d_lut[] = {
0,    172,  317,  422,  489,  538,  581,  622,  661,  697,  731,  764,  795,
826,  854,  881,  907,  932,  957,  980,  1004, 1027, 1050, 1072, 1093, 1112,
1129, 1146, 1163, 1180, 1197, 1214, 1230, 1247, 1263, 1279, 1295, 1311, 1327,
1343, 1358, 1373, 1389, 1404, 1419, 1433, 1448, 1463, 1477, 1492, 1506, 1520,
1534, 1548, 1561, 1575, 1589, 1602, 1615, 1628, 1641, 1654, 1667, 1680, 1693,
1705, 1718, 1730, 1742, 1754, 1766, 1778, 1790, 1802, 1813, 1825, 1836, 1848,
1859, 1870, 1881, 1892, 1903, 1914, 1925, 1936, 1946, 1957, 1967, 1977, 1988,
1998, 2008, 2018, 2028, 2038, 2048, 2058, 2067, 2077, 2087, 2096, 2106, 2115,
2125, 2134, 2144, 2153, 2163, 2173, 2182, 2192, 2202, 2211, 2221, 2230, 2240,
2250, 2259, 2269, 2278, 2288, 2298, 2307, 2317, 2326, 2336, 2345, 2355, 2364,
2374, 2383, 2393, 2402, 2412, 2421, 2431, 2440, 2449, 2459, 2468, 2478, 2487,
2496, 2505, 2515, 2524, 2533, 2542, 2551, 2560, 2569, 2578, 2587, 2596, 2605,
2614, 2623, 2632, 2641, 2650, 2658, 2667, 2676, 2684, 2693, 2701, 2710, 2718,
2727, 2735, 2743, 2752, 2760, 2768, 2776, 2784, 2792, 2800, 2808, 2816, 2824,
2832, 2839, 2847, 2854, 2862, 2870, 2877, 2884, 2892, 2899, 2906, 2913, 2920,
2927, 2934, 2941, 2948, 2954, 2961, 2968, 2974, 2981, 2987, 2993, 3000, 3006,
3012, 3018, 3024, 3030, 3035, 3041, 3047, 3052, 3058, 3063, 3068, 3073, 3079,
3084, 3088, 3093, 3098, 3103, 3107, 3112, 3116, 3121, 3125, 3129, 3133, 3137,
3141, 3145, 3148, 3152, 3155, 3159, 3162, 3165, 3168, 3171, 3174, 3177, 3179,
3182, 3184, 3186, 3189, 3191, 3193, 3195, 3196, 3198, 4001,
};

uint16_t ltm_b_lut[] = {
0,    115,  230,  346,  458,  575,  694,  813,  927,  1033, 1129, 1211, 1275,
1321, 1363, 1402, 1438, 1473, 1505, 1536, 1565, 1592, 1617, 1641, 1663, 1684,
1703, 1722, 1739, 1755, 1771, 1786, 1800, 1813, 1827, 1840, 1853, 1866, 1879,
1891, 1904, 1916, 1928, 1940, 1951, 1963, 1974, 1986, 1997, 2007, 2018, 2029,
2039, 2049, 2059, 2069, 2079, 2089, 2098, 2108, 2117, 2126, 2135, 2144, 2153,
2162, 2170, 2178, 2187, 2195, 2203, 2211, 2219, 2227, 2234, 2242, 2249, 2257,
2264, 2271, 2278, 2285, 2292, 2299, 2306, 2312, 2319, 2326, 2332, 2339, 2345,
2351, 2357, 2364, 2370, 2376, 2382, 2388, 2394, 2400, 2405, 2411, 2417, 2423,
2429, 2434, 2440, 2445, 2451, 2457, 2462, 2468, 2473, 2479, 2484, 2490, 2495,
2501, 2507, 2512, 2518, 2523, 2529, 2534, 2540, 2545, 2551, 2557, 2562, 2568,
2573, 2579, 2584, 2589, 2595, 2600, 2605, 2610, 2616, 2621, 2626, 2631, 2636,
2641, 2646, 2651, 2656, 2661, 2665, 2670, 2675, 2680, 2685, 2689, 2694, 2699,
2703, 2708, 2712, 2717, 2721, 2726, 2730, 2735, 2739, 2743, 2748, 2752, 2756,
2761, 2765, 2769, 2773, 2778, 2782, 2786, 2790, 2794, 2798, 2802, 2807, 2811,
2815, 2819, 2823, 2827, 2831, 2835, 2839, 2843, 2846, 2850, 2854, 2858, 2862,
2866, 2870, 2874, 2877, 2881, 2885, 2889, 2893, 2897, 2900, 2904, 2908, 2912,
2915, 2919, 2923, 2927, 2931, 2934, 2938, 2942, 2946, 2949, 2953, 2957, 2961,
2964, 2968, 2972, 2976, 2979, 2983, 2987, 2991, 2994, 2998, 3002, 3006, 3010,
3013, 3017, 3021, 3025, 3029, 3032, 3036, 3040, 3044, 3048, 3052, 3056, 3060,
3064, 3067, 3071, 3075, 3079, 3083, 3087, 3091, 3095, 3100, 3104, 3108, 3112,
3116, 3120, 3124, 3128, 3132, 3136, 3140, 3144, 3148, 3152, 3156, 3160, 3164,
3168, 3173, 3177, 3181, 3185, 3189, 3193, 3197, 3201, 3205, 3209, 3213, 3217,
3221, 3225, 3229, 3233, 3237, 3241, 3245, 3249, 3253, 3257, 3261, 3265, 3269,
3273, 3277, 3281, 3285, 3289, 3293, 3297, 3301, 3305, 3309, 3313, 3317, 3321,
3325, 3329, 3333, 3337, 3341, 3345, 3349, 3353, 3357, 3361, 3365, 3369, 3373,
3377, 3381, 3385, 3389, 3393, 3397, 3401, 3405, 3409, 3413, 3417, 3421, 3425,
3429, 3433, 3437, 3441, 3445, 3449, 3453, 3457, 3461, 3465, 3469, 3472, 3476,
3480, 3484, 3488, 3492, 3496, 3500, 3504, 3508, 3512, 3516, 3520, 3524, 3528,
3532, 3536, 3539, 3543, 3547, 3551, 3555, 3559, 3563, 3567, 3571, 3575, 3579,
3583, 3586, 3590, 3594, 3598, 3602, 3606, 3610, 3614, 3618, 3622, 3625, 3629,
3633, 3637, 3641, 3645, 3649, 3653, 3657, 3660, 3664, 3668, 3672, 3676, 3680,
3684, 3687, 3691, 3695, 3699, 3703, 3707, 3711, 3714, 3718, 3722, 3726, 3730,
3734, 3738, 3741, 3745, 3749, 3753, 3757, 3761, 3764, 3768, 3772, 3776, 3780,
3784, 3787, 3791, 3795, 3799, 3803, 3806, 3810, 3814, 3818, 3822, 3826, 3829,
3833, 3837, 3841, 3845, 3848, 3852, 3856, 3860, 3863, 3867, 3871, 3875, 3879,
3882, 3886, 3890, 3894, 3897, 3901, 3905, 3909, 3913, 3916, 3920, 3924, 3928,
3931, 3935, 3939, 3943, 3946, 3950, 3954, 3958, 3961, 3965, 3969, 3972, 3976,
3980, 3984, 3987, 3991, 3995, 3999, 4002, 4006, 4010, 4013, 4017, 4021, 4025,
4028, 4032, 4036, 4039, 4043, 4047, 4050, 4054, 4058, 4061, 4065, 4069, 4073,
4076, 4080, 4084, 4087, 4091, 4095,
};

uint16_t ltm_g_lut[] = {
0,    48,   96,   142,  187,  231,  273,  313,  352,  389,  425,  458,  490,
520,  549,  577,  605,  632,  659,  686,  714,  741,  769,  796,  824,  851,
878,  905,  932,  959,  986,  1013, 1039, 1066, 1092, 1118, 1144, 1170, 1196,
1222, 1247, 1272, 1297, 1322, 1347, 1371, 1395, 1420, 1443, 1467, 1490, 1513,
1536, 1558, 1581, 1603, 1625, 1647, 1668, 1690, 1711, 1732, 1754, 1775, 1796,
1817, 1839, 1860, 1880, 1901, 1922, 1942, 1962, 1982, 2001, 2020, 2039, 2057,
2075, 2092, 2110, 2126, 2142, 2158, 2174, 2190, 2205, 2220, 2234, 2249, 2263,
2277, 2290, 2304, 2317, 2330, 2342, 2355, 2367, 2380, 2392, 2405, 2417, 2429,
2441, 2453, 2465, 2477, 2489, 2500, 2511, 2522, 2533, 2544, 2555, 2565, 2575,
2585, 2595, 2605, 2614, 2623, 2632, 2641, 2649, 2658, 2665, 2673, 2681, 2688,
2695, 2703, 2709, 2716, 2723, 2729, 2736, 2742, 2748, 2754, 2759, 2765, 2770,
2776, 2781, 2786, 2791, 2796, 2801, 2806, 2811, 2816, 2821, 2826, 2831, 2836,
2841, 2846, 2851, 2856, 2861, 2866, 2871, 2876, 2881, 2885, 2890, 2895, 2900,
2905, 2910, 2914, 2919, 2924, 2928, 2933, 2938, 2942, 2947, 2952, 2956, 2961,
2965, 2970, 2974, 2979, 2983, 2988, 2992, 2996, 3001, 3005, 3009, 3013, 3018,
3022, 3026, 3030, 3034, 3038, 3042, 3046, 3050, 3054, 3058, 3062, 3066, 3069,
3073, 3077, 3080, 3084, 3088, 3091, 3095, 3098, 3102, 3105, 3108, 3112, 3115,
3118, 3121, 3124, 3127, 3130, 3133, 3136, 3139, 3142, 3145, 3148, 3150, 3153,
3156, 3158, 3161, 3163, 3166, 3168, 3170, 3173, 3175, 3177, 3179, 3181, 3183,
3185, 3187, 3189, 3190, 3192, 3194, 3195, 3197, 3198, 3200, 3210, 3221, 3232,
3242, 3252, 3262, 3272, 3282, 3291, 3300, 3310, 3319, 3327, 3336, 3345, 3353,
3361, 3369, 3377, 3385, 3393, 3400, 3408, 3415, 3422, 3429, 3436, 3443, 3450,
3456, 3462, 3469, 3475, 3481, 3487, 3493, 3498, 3504, 3509, 3515, 3520, 3525,
3530, 3535, 3540, 3545, 3549, 3554, 3559, 3563, 3567, 3572, 3576, 3580, 3584,
3588, 3592, 3595, 3599, 3603, 3606, 3610, 3613, 3617, 3620, 3624, 3627, 3630,
3633, 3636, 3639, 3642, 3645, 3648, 3651, 3654, 3656, 3659, 3662, 3665, 3667,
3670, 3673, 3675, 3678, 3680, 3683, 3685, 3688, 3690, 3693, 3695, 3698, 3700,
3703, 3705, 3707, 3710, 3712, 3715, 3717, 3719, 3721, 3724, 3726, 3728, 3730,
3733, 3735, 3737, 3739, 3741, 3743, 3745, 3747, 3749, 3751, 3753, 3755, 3757,
3759, 3761, 3763, 3765, 3767, 3769, 3771, 3772, 3774, 3776, 3778, 3780, 3781,
3783, 3785, 3787, 3788, 3790, 3792, 3793, 3795, 3797, 3798, 3800, 3801, 3803,
3805, 3806, 3808, 3809, 3811, 3812, 3814, 3815, 3817, 3818, 3820, 3821, 3823,
3824, 3825, 3827, 3828, 3829, 3831, 3832, 3834, 3835, 3836, 3838, 3839, 3840,
3841, 3843, 3844, 3845, 3847, 3848, 3849, 3850, 3851, 3853, 3854, 3855, 3856,
3858, 3859, 3860, 3861, 3862, 3863, 3865, 3866, 3867, 3868, 3869, 3870, 3871,
3873, 3874, 3875, 3876, 3877, 3878, 3879, 3880, 3881, 3882, 3884, 3885, 3886,
3887, 3888, 3889, 3890, 3891, 3892, 3893, 3894, 3895, 3896, 3897, 3898, 3900,
3901, 3902, 3903, 3904, 3905, 3906, 3907, 3908, 3909, 3910, 3911, 3912, 3913,
3914, 3915, 3916, 3917, 3918, 3919, 3920, 3922, 3923, 3924, 3925, 3926, 3927,
3928, 3929, 3930, 3931, 3932, 3933, 3934, 3935, 3936, 3937, 3938, 3939, 3940,
3941, 3942, 3943, 3944, 3945, 3946, 3947, 3948, 3949, 3950, 3951, 3952, 3953,
3954, 3955, 3956, 3957, 3958, 3959, 3960, 3961, 3962, 3963, 3964, 3965, 3966,
3967, 3968, 3969, 3970, 3971, 3972, 3973, 3974, 3975, 3976, 3977, 3978, 3979,
3979, 3980, 3981, 3982, 3983, 3984, 3985, 3986, 3987, 3988, 3989, 3990, 3991,
3992, 3992, 3993, 3994, 3995, 3996, 3997, 3998, 3999, 4000, 4001, 4002, 4002,
4003, 4004, 4005, 4006, 4007, 4008, 4009, 4009, 4010, 4011, 4012, 4013, 4014,
4015, 4015, 4016, 4017, 4018, 4019, 4020, 4020, 4021, 4022, 4023, 4024, 4024,
4025, 4026, 4027, 4028, 4028, 4029, 4030, 4031, 4032, 4032, 4033, 4034, 4035,
4035, 4036, 4037, 4038, 4039, 4039, 4040, 4041, 4041, 4042, 4043, 4044, 4044,
4045, 4046, 4047, 4047, 4048, 4049, 4049, 4050, 4051, 4051, 4052, 4053, 4053,
4054, 4055, 4055, 4056, 4057, 4057, 4058, 4059, 4059, 4060, 4061, 4061, 4062,
4062, 4063, 4064, 4064, 4065, 4065, 4066, 4067, 4067, 4068, 4068, 4069, 4069,
4070, 4071, 4071, 4072, 4072, 4073, 4073, 4074, 4074, 4075, 4075, 4076, 4076,
4077, 4077, 4078, 4078, 4079, 4079, 4080, 4080, 4081, 4081, 4082, 4082, 4082,
4083, 4083, 4084, 4084, 4085, 4085, 4085, 4086, 4086, 4087, 4087, 4087, 4088,
4088, 4088, 4089, 4089, 4089, 4090, 4090, 4090, 4091, 4091, 4091, 4092, 4092,
4092, 4093, 4093, 4093, 4093, 4094, 4094, 4094, 4095, 4095, 4095, 4095, 4095,
4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
4095, 4095,
};

uint16_t dci_lut0[] = {
1,   2,   2,   3,   4,   5,   6,   7,   8,   9,   11,  12,  13,  14,  15,  16,
17,  18,  19,  19,  20,  21,  22,  23,  24,  24,  25,  26,  27,  28,  28,  29,
30,  30,  31,  32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,  37,  37,
38,  38,  38,  39,  39,  39,  40,  40,  40,  41,  41,  41,  43,  44,  46,  48,
49,  51,  52,  54,  55,  57,  58,  60,  62,  63,  65,  66,  68,  70,  71,  73,
75,  77,  78,  80,  82,  84,  85,  87,  89,  91,  93,  95,  96,  98,  100, 102,
104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134,
136, 138, 140, 142, 144, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165,
167, 169, 170, 172, 174, 176, 178, 179, 181, 183, 185, 187, 188, 190, 192, 193,
195, 196, 198, 200, 201, 203, 204, 206, 207, 209, 210, 211, 213, 214, 215, 217,
218, 219, 220, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
235, 236, 236, 237, 238, 239, 239, 240, 241, 241, 242, 243, 243, 244, 244, 245,
246, 246, 247, 247, 247, 248, 248, 249, 249, 249, 250, 250, 250, 251, 251, 251,
252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254,
254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

uint16_t dci_lut1[256] = {0,};

uint16_t lscr_lut[] = {
512,   521,  541,  570,  620,  688,  767,  866,
985,  1147, 1340, 1551, 1802, 2102, 2463, 2842,
3304, 3886, 4095, 4095, 4095, 4095, 4095, 4095,
4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
};

/* for imx327 tuning */
struct isp_ccm_cfg ccm_hw_cfg = {
	.coef = {
			{0x6EC, 0x3D43, 0x3FD1},
			{0x3E16, 0x61F, 0x3FCA},
			{0x1E, 0x3DBE, 0x624},
		},
};

struct isp_buffer {
	enum cvi_isp_raw  raw_num;
	uint64_t          addr;
	uint64_t          y_addr;
	uint64_t          u_addr;
	uint64_t          v_addr;
	uint64_t          timestamp;
	struct vip_rect   crop_le;
	struct vip_rect   crop_se;
	struct isp_rgbmap_info map_info;
	struct list_head  list;
};

struct isp_queue {
	struct list_head rdy_queue;
	uint32_t num_rdy;
	enum cvi_isp_raw raw_num;
} pre_out_queue[ISP_PRERAW_MAX], pre_out_se_queue[ISP_PRERAW_MAX],
	raw_dump_q[ISP_PRERAW_MAX], raw_dump_se_q[ISP_PRERAW_MAX],
	raw_dump_out_q[ISP_PRERAW_MAX], raw_dump_out_se_q[ISP_PRERAW_MAX],
	post_in_queue, post_in_se_queue;

struct _isp_snr_i2c_node {
	struct snsr_regs_s n;
	struct list_head list;
};

struct _isp_crop_node {
	struct snsr_isp_s n;
	struct list_head list;
};

struct _isp_snr_cfg_queue {
	struct list_head	list;
} isp_snr_i2c_queue[ISP_PRERAW_MAX], isp_crop_queue[ISP_PRERAW_MAX];

struct _isp_raw_num_n {
	enum cvi_isp_raw raw_num;
	struct list_head list;
};

struct _isp_sof_raw_num_q {
	struct list_head	list;
} pre_raw_num_q;

static spinlock_t raw_num_lock;

struct _cfgs {
	struct cvi_vip_isp_blc_config blc_cfg[ISP_BLC_ID_MAX];
	struct cvi_vip_isp_wbg_config wbg_cfg[ISP_WBG_ID_MAX];
	struct cvi_vip_isp_ccm_config ccm_cfg;
	struct cvi_vip_isp_dhz_config dhz_cfg;
} isp_cfgs;

#define RGBMAP_BUF		3
#define OFFLINE_RAW_BUF_NUM	2
#define OFFLINE_YUV_BUF_NUM	2

/* struct mempool
 * @base: the address of the mempool
 * @size: the size of the mempool
 * @byteused: the number of bytes used
 * @sts_busy_idx: idx of sts_mem which is written
 * @sts_lock: if locked then sts_busy_idx won't be updated until lock released.
 */
struct _mempool {
	uint64_t base;
	uint32_t size;
	uint32_t byteused;

	uint64_t bayer_le[OFFLINE_RAW_BUF_NUM];
	uint64_t bayer_se[OFFLINE_RAW_BUF_NUM];
	uint64_t yuv_y[OFFLINE_YUV_BUF_NUM];//yuv sensor
	uint64_t yuv_u[OFFLINE_YUV_BUF_NUM];
	uint64_t yuv_v[OFFLINE_YUV_BUF_NUM];
	uint64_t manr;
	uint64_t manr_rtile;
	uint64_t rgbmap_le[RGBMAP_BUF];
	uint64_t rgbmap_se[RGBMAP_BUF];
	uint64_t lmap_le;
	uint64_t lmap_se;
	uint64_t lsc_le[2];//1 for tile
	uint64_t lsc_se[2];//1 for tile
	uint64_t tdnr[2];
	uint64_t tdnr_rtile[2];//tile

	struct cvi_isp_sts_mem sts_mem[2];
	uint8_t pre_sts_busy_idx;
	uint8_t post_sts_busy_idx;
	spinlock_t pre_sts_lock;
	uint8_t pre_sts_in_use;
	spinlock_t post_sts_lock;
	uint8_t post_sts_in_use;
} mempool, mempool_raw1;

static struct task_struct *isp_pre_th;
static struct task_struct *isp_post_th;
static atomic_t            isp_pre_exit;

static int _isp_preraw_thread(void *arg);
static int _isp_postraw_thread(void *arg);

struct isp_tuning_cfg tuning_buf_addr;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
typedef struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
} _timer;
#else
typedef struct timer_list _timer;
#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))

_timer usr_pic_timer;
static bool snr_ut_cfg_csibdg_flag;

static struct isp_buffer *isp_byr[ISP_PRERAW_MAX], *isp_byr_se[ISP_PRERAW_MAX];

struct byr_dump_info {
	u32 byr_size;
	u16 byr_w;
	u16 byr_h;
	u16 byr_crop_x;
	u16 byr_crop_y;
} byr_info[2];

static spinlock_t byr_dump;
static spinlock_t buf_lock;
static spinlock_t snr_node_lock[ISP_PRERAW_MAX];

static const char *const CLK_ISP_NAME[] = {"clk_sys_0", "clk_sys_1", "clk_sys_2", "clk_axi"};
static const char *const CLK_MAC_NAME[] = {"clk_csi_mac0", "clk_csi_mac1"};

/* vi_proc control */
static int viproc_en[2] = {1, 0};
static int count;
module_param_array(viproc_en, int, &count, 0664);

static int csi_patgen_en[ISP_PRERAW_MAX] = {0,};
int cif_auto = 1;
int isp_v4l2_debug;
int burst_i2c_en;
/* To define the output of proc.
 *
 * 0: default info
 * 1: preraw0 reg-dump
 * 2: preraw1 reg-dump
 * 3: postraw reg-dump
 */
int proc_isp_mode;
/* control internal patgen
 *
 * 1: enable
 * 0: disable
 */
module_param_array(csi_patgen_en, int, &count, 0644);
module_param(cif_auto, int, 0644);
/* v4l2 debug level of isp video device
 * Bigger, more v4l2 debug message.
 */
module_param(isp_v4l2_debug, int, 0644);
module_param(burst_i2c_en, int, 0644);

/* runtime tuning control
 * ctrl:
 *	0: all ch stop update.
 *	1: stop after apply ch1 setting
 *	2: stop after apply ch2 setting
 */
static int tuning_dis[3] = {0, 0, 0}; //ctrl, preraw, post
module_param_array(tuning_dis, int, &count, 0664);

/* control vi trigger */
int re_trigger;
module_param(re_trigger, int, 0644);

/* runtime disable i2c control */
int i2c_dis;
module_param(i2c_dis, int, 0644);

/* runtime to set early line interrupt */
int early_line = 1104;
module_param(early_line, int, 0644);

static int cif_lvds_reset(struct cvi_isp_vdev *vdev, enum cvi_isp_raw dev_num);
static void _isp_yuv_bypass_buf_enq(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);

	lt->function(lt->data);
}
#endif //(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))

/**
 * _mempool_reset - reset the byteused and assigned buffer for each dma
 *
 */
static void _mempool_reset(void)
{
	memset(&mempool.byteused, 0, sizeof(mempool) -
				sizeof(mempool.base) - sizeof(mempool.size));

	mempool_raw1.byteused = mempool.byteused;

	mempool.pre_sts_busy_idx = 0;
	mempool_raw1.pre_sts_busy_idx = 0;
	mempool.post_sts_busy_idx = 0;
	mempool_raw1.post_sts_busy_idx = 0;
	mempool.pre_sts_in_use = 0;
	mempool.post_sts_in_use = 0;
	mempool_raw1.pre_sts_in_use = 0;
	mempool_raw1.post_sts_in_use = 0;
	spin_lock_init(&mempool.pre_sts_lock);
	spin_lock_init(&mempool_raw1.pre_sts_lock);
	spin_lock_init(&mempool.post_sts_lock);
	spin_lock_init(&mempool_raw1.post_sts_lock);
}

/**
 * isp_mempool_setup - init isp's mempool
 *
 * @param addr: base-address of the mempool
 * @param size: size of the mempool
 */
void isp_mempool_setup(void)
{
	_mempool_reset();
}

/**
 * _mempool_get_addr - get mempool's latest address.
 *
 * @return: the latest address of the mempool.
 */
static uint64_t _mempool_get_addr(void)
{
	return mempool.base + mempool.byteused;
}

/**
 * _mempool_pop - acquire a buffer-space from mempool.
 *
 * @param size: the space acquired.
 * @return: negative if no enough space; o/w, the address of the buffer needed.
 */
static int64_t _mempool_pop(uint64_t size)
{
	int64_t addr;

	size = VIP_ALIGN(size);

	if ((mempool.byteused + size) > mempool.size) {
		pr_err("reserved_memory(0x%x) is not enough. byteused(0x%x), alloc size(0x%x)\n",
				mempool.size, mempool.byteused, size);
		return -EINVAL;
	}
	addr = mempool.base + mempool.byteused;
	mempool.byteused += size;
	return addr;
}

static int64_t _mempool_pop_128(uint64_t size)
{
	int64_t addr;

	size = ((size + 0x7F) & ~0x7F);

	if ((mempool.byteused + size) > mempool.size) {
		pr_err("reserved_memory(0x%x) is not enough. byteused(0x%x), alloc size(0x%x)\n",
				mempool.size, mempool.byteused, size);
		return -EINVAL;
	}
	addr = mempool.base + mempool.byteused;
	mempool.byteused += size;
	return addr;
}

static void _mempool_dump(struct isp_ctx *ictx)
{
	u8 i = 0;

	pr_info("VIP reserved memory total size:0x%x\n", mempool.byteused);
	pr_info("*************************************************\n");
	pr_info("bayer_le(0x%llx, 0x%llx)\n",
			mempool.bayer_le[0], mempool.bayer_le[1]);
	pr_info("bayer_se(0x%llx, 0x%llx)\n",
			mempool.bayer_se[0], mempool.bayer_se[1]);
	for (i = 0; i < RGBMAP_BUF; i++) {
		pr_info("rgbmap_le(0x%llx)\n",
				mempool.rgbmap_le[i]);
		pr_info("rgbmap_se(0x%llx)\n",
				mempool.rgbmap_se[i]);
	}
	pr_info("lmap le(0x%llx) se(0x%llx)\n",
			mempool.lmap_le, mempool.lmap_se);
	pr_info("lsc le(0x%llx) se(0x%llx)\n",
			mempool.lsc_le[0], mempool.lsc_se[0]);
	pr_info("manr(0x%llx)\n",
			mempool.manr);
	pr_info("tdnr(0x%llx, 0x%llx)\n",
			mempool.tdnr[0], mempool.tdnr[1]);
	if (ictx->is_tile) {
		pr_info("manr_rtile(0x%llx)\n", mempool.manr_rtile);
		pr_info("tdnr_rtile(0x%llx, 0x%llx)\n",
				mempool.tdnr_rtile[0], mempool.tdnr_rtile[1]);
		pr_info("lsc tile le(0x%llx) se(0x%llx)\n",
				mempool.lsc_le[1], mempool.lsc_se[1]);
	}
	pr_info("*************************************************\n");
	pr_info("awb_le(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].awb_le.phy_addr,
			mempool.sts_mem[1].awb_le.phy_addr);
	pr_info("awb_se(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].awb_se.phy_addr,
			mempool.sts_mem[1].awb_se.phy_addr);
	pr_info("awb_post(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].awb_post.phy_addr,
			mempool.sts_mem[1].awb_post.phy_addr);
	pr_info("ae_le0(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].ae_le0.phy_addr,
			mempool.sts_mem[1].ae_le0.phy_addr);
	pr_info("ae_le1(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].ae_le1.phy_addr,
			mempool.sts_mem[1].ae_le1.phy_addr);
	pr_info("hist_le(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].hist_le.phy_addr,
			mempool.sts_mem[1].hist_le.phy_addr);
	pr_info("ae_se(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].ae_se.phy_addr,
			mempool.sts_mem[1].ae_se.phy_addr);
	pr_info("hist_se(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].hist_se.phy_addr,
			mempool.sts_mem[1].hist_se.phy_addr);
	pr_info("gms(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].gms.phy_addr,
			mempool.sts_mem[1].gms.phy_addr);
	pr_info("dci(0x%llx, 0x%llx)\n",
			mempool.sts_mem[0].dci.phy_addr,
			mempool.sts_mem[1].dci.phy_addr);
	if (ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		pr_info("yuv_y(0x%llx, 0x%llx)\n", mempool_raw1.yuv_y[0], mempool_raw1.yuv_y[1]);
		pr_info("yuv_u(0x%llx, 0x%llx)\n", mempool_raw1.yuv_u[0], mempool_raw1.yuv_u[1]);
		pr_info("yuv_v(0x%llx, 0x%llx)\n", mempool_raw1.yuv_v[0], mempool_raw1.yuv_v[1]);
	}
	pr_info("*************************************************\n");

	if (ictx->is_dual_sensor) {
		pr_info("***********************Dual sensor dump**********************\n");
		pr_info("bayer_le(0x%llx, 0x%llx)\n",
				mempool_raw1.bayer_le[0], mempool_raw1.bayer_le[1]);
		pr_info("bayer_se(0x%llx, 0x%llx)\n",
				mempool_raw1.bayer_se[0], mempool_raw1.bayer_se[1]);
		for (i = 0; i < RGBMAP_BUF; i++) {
			pr_info("rgbmap_le(0x%llx)\n",
					mempool_raw1.rgbmap_le[i]);
			pr_info("rgbmap_se(0x%llx)\n",
					mempool_raw1.rgbmap_se[i]);
		}
		pr_info("lmap le(0x%llx) se(0x%llx)\n",
				mempool_raw1.lmap_le, mempool_raw1.lmap_se);
		pr_info("awb_le(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].awb_le.phy_addr,
				mempool_raw1.sts_mem[1].awb_le.phy_addr);
		pr_info("awb_se(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].awb_se.phy_addr,
				mempool_raw1.sts_mem[1].awb_se.phy_addr);
		pr_info("awb_post(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].awb_post.phy_addr,
				mempool_raw1.sts_mem[1].awb_post.phy_addr);
		pr_info("ae_le0(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].ae_le0.phy_addr,
				mempool_raw1.sts_mem[1].ae_le0.phy_addr);
		pr_info("ae_le1(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].ae_le1.phy_addr,
				mempool_raw1.sts_mem[1].ae_le1.phy_addr);
		pr_info("hist_le(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].hist_le.phy_addr,
				mempool_raw1.sts_mem[1].hist_le.phy_addr);
		pr_info("ae_se(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].ae_se.phy_addr,
				mempool_raw1.sts_mem[1].ae_se.phy_addr);
		pr_info("hist_se(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].hist_se.phy_addr,
				mempool_raw1.sts_mem[1].hist_se.phy_addr);
		pr_info("gms(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].gms.phy_addr,
				mempool_raw1.sts_mem[1].gms.phy_addr);
		pr_info("dci(0x%llx, 0x%llx)\n",
				mempool_raw1.sts_mem[0].dci.phy_addr,
				mempool_raw1.sts_mem[1].dci.phy_addr);
		pr_info("lsc le(0x%llx) se(0x%llx)\n",
				mempool_raw1.lsc_le[0], mempool_raw1.lsc_se[0]);
		if (ictx->is_tile) {
			pr_info("lsc tile le(0x%llx) se(0x%llx)\n",
					mempool_raw1.lsc_le[1], mempool_raw1.lsc_se[1]);
		}

		pr_info("manr(0x%llx)\n",
				mempool_raw1.manr);
		pr_info("tdnr(0x%llx, 0x%llx)\n",
				mempool_raw1.tdnr[0], mempool_raw1.tdnr[1]);
		if (ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
			pr_info("yuv_y(0x%llx, 0x%llx)\n", mempool_raw1.yuv_y[0], mempool_raw1.yuv_y[1]);
			pr_info("yuv_u(0x%llx, 0x%llx)\n", mempool_raw1.yuv_u[0], mempool_raw1.yuv_u[1]);
			pr_info("yuv_v(0x%llx, 0x%llx)\n", mempool_raw1.yuv_v[0], mempool_raw1.yuv_v[1]);
		}
		pr_info("*************************************************\n");
	}
}

void _isp_tuning_setup(void)
{
	uint8_t i = 0;
	static void *pre_vir[ISP_PRERAW_MAX];
	static void *post_vir[ISP_PRERAW_MAX];

	_mempool_reset();

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		if (pre_vir[i] == NULL)
			pre_vir[i] = kmalloc(sizeof(struct cvi_vip_isp_pre_cfg), GFP_ATOMIC);

		tuning_buf_addr.pre_vir[i] = pre_vir[i];
		tuning_buf_addr.pre_addr[i] = virt_to_phys(tuning_buf_addr.pre_vir[i]);

		memset((void *)tuning_buf_addr.pre_vir[i], 0x0, sizeof(struct cvi_vip_isp_pre_cfg));

		if (post_vir[i] == NULL)
			post_vir[i] = kmalloc(sizeof(struct cvi_vip_isp_post_cfg), GFP_ATOMIC);

		tuning_buf_addr.post_vir[i] = post_vir[i];
		tuning_buf_addr.post_addr[i] = virt_to_phys(tuning_buf_addr.post_vir[i]);

		memset((void *)tuning_buf_addr.post_vir[i], 0x0, sizeof(struct cvi_vip_isp_post_cfg));

		pr_info("tuning pre_addr[%d]=0x%llx, post_addr[%d]=0x%llx\n",
					i, tuning_buf_addr.pre_addr[i], i, tuning_buf_addr.post_addr[i]);
	}
}

void _isp_snr_cfg_enq(struct cvi_isp_snr_update *snr_node, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_snr_i2c_node  *n;
	struct _isp_crop_node	  *c_n;

	if (snr_node == NULL)
		return;

	if (i2c_dis)
		return;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);

	if (snr_node->snr_cfg_node.snsr.need_update) {
		n = kmalloc(sizeof(*n), GFP_ATOMIC);
		memcpy(&n->n, &snr_node->snr_cfg_node.snsr, sizeof(struct snsr_regs_s));
		list_add_tail(&n->list, &isp_snr_i2c_queue[raw_num].list);
	}

	if (snr_node->snr_cfg_node.isp.need_update) {
		c_n = kmalloc(sizeof(*c_n), GFP_ATOMIC);
		memcpy(&c_n->n, &snr_node->snr_cfg_node.isp, sizeof(struct snsr_isp_s));
		list_add_tail(&c_n->list, &isp_crop_queue[raw_num].list);
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
}

void pre_raw_num_enq(struct _isp_sof_raw_num_q *q, struct _isp_raw_num_n *n)
{
	unsigned long flags;

	spin_lock_irqsave(&raw_num_lock, flags);
	list_add_tail(&n->list, &q->list);
	spin_unlock_irqrestore(&raw_num_lock, flags);
}

void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b)
{
	unsigned long flags;

	if (b == NULL)
		return;

	spin_lock_irqsave(&buf_lock, flags);
	list_add_tail(&b->list, &q->rdy_queue);
	++q->num_rdy;
	spin_unlock_irqrestore(&buf_lock, flags);
}

struct isp_buffer *isp_next_buf(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	spin_lock_irqsave(&buf_lock, flags);
	if (!list_empty(&q->rdy_queue))
		b = list_first_entry(&q->rdy_queue, struct isp_buffer, list);
	spin_unlock_irqrestore(&buf_lock, flags);

	return b;
}

struct isp_buffer *isp_buf_remove(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	spin_lock_irqsave(&buf_lock, flags);
	if (!list_empty(&q->rdy_queue)) {
		b = list_first_entry(&q->rdy_queue, struct isp_buffer, list);
		list_del_init(&b->list);
		--q->num_rdy;
	}
	spin_unlock_irqrestore(&buf_lock, flags);

	return b;
}

int isp_buf_empty(struct isp_queue *q)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&buf_lock, flags);
	empty = list_empty(&q->rdy_queue);
	spin_unlock_irqrestore(&buf_lock, flags);

	return empty;
}

static enum ISP_BAYER_TYPE _mbus_remap(__u32 code)
{
	switch (code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		return ISP_BAYER_TYPE_BG;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		return ISP_BAYER_TYPE_GB;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		return ISP_BAYER_TYPE_GR;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		return ISP_BAYER_TYPE_RG;
	}

	return ISP_BAYER_TYPE_GB;
}

static void _dhz_cfg_remap(struct cvi_vip_isp_dhz_config *in_cfg,
			   struct isp_dhz_cfg *out_cfg)
{
	out_cfg->strength = in_cfg->strength;
	out_cfg->th_smooth = in_cfg->th_smooth;
	out_cfg->cum_th   = in_cfg->cum_th;
	out_cfg->hist_th  = in_cfg->hist_th;
	out_cfg->tmap_min = in_cfg->tmap_min;
	out_cfg->tmap_max = in_cfg->tmap_max;
	out_cfg->sw_dc_th = in_cfg->sw_dc_th;
	out_cfg->sw_aglobal = in_cfg->sw_aglobal;
	out_cfg->sw_dc_trig = in_cfg->sw_dc_trig;
}

static void isp_init_param(struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t i = 0;

	memset(ctx, 0, sizeof(*ctx));

	ctx->phys_regs          = isp_get_phys_reg_bases();
	ctx->sensor_bitdepth    = 12;

	ctx->cam_id		= 0;
	ctx->is_yuv_sensor      = false;
	ctx->is_3dnr_on         = true;
	ctx->is_offline_postraw = true;
	ctx->is_offline_scaler  = true;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		ctx->rgb_color_mode[i] = ISP_BAYER_TYPE_GB;
		ctx->isp_pipe_cfg[i].is_patgen_en	= false;
		ctx->isp_pipe_cfg[i].is_offline_preraw	= false;
		ctx->isp_pipe_cfg[i].is_yuv_bypass_path	= false;
		ctx->isp_pipe_cfg[i].max_height		= 0;
		ctx->isp_pipe_cfg[i].max_width		= 0;
		ctx->isp_pipe_cfg[i].csibdg_width	= 0;
		ctx->isp_pipe_cfg[i].csibdg_height	= 0;

		INIT_LIST_HEAD(&pre_out_queue[i].rdy_queue);
		INIT_LIST_HEAD(&pre_out_se_queue[i].rdy_queue);
		pre_out_queue[i].num_rdy       = 0;
		pre_out_queue[i].raw_num       = i;
		pre_out_se_queue[i].num_rdy    = 0;
		pre_out_se_queue[i].raw_num    = i;

		INIT_LIST_HEAD(&raw_dump_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_se_q[i].rdy_queue);
		raw_dump_q[i].num_rdy          = 0;
		raw_dump_q[i].raw_num          = i;
		raw_dump_se_q[i].num_rdy       = 0;
		raw_dump_se_q[i].raw_num       = i;

		INIT_LIST_HEAD(&raw_dump_out_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_out_se_q[i].rdy_queue);
		raw_dump_out_q[i].num_rdy          = 0;
		raw_dump_out_q[i].raw_num          = i;
		raw_dump_out_se_q[i].num_rdy       = 0;
		raw_dump_out_se_q[i].raw_num       = i;

		INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		INIT_LIST_HEAD(&isp_crop_queue[i].list);
	}

	INIT_LIST_HEAD(&post_in_queue.rdy_queue);
	INIT_LIST_HEAD(&post_in_se_queue.rdy_queue);
	post_in_queue.num_rdy	 = 0;
	post_in_se_queue.num_rdy = 0;

	INIT_LIST_HEAD(&pre_raw_num_q.list);

	memset(&vdev->sns_crop[0], 0, sizeof(vdev->sns_crop) * ISP_PRERAW_MAX);
	memset(&vdev->sns_se_crop[0], 0, sizeof(vdev->sns_se_crop) * ISP_PRERAW_MAX);
	memset(&vdev->usr_crop, 0, sizeof(vdev->usr_crop));

	for (i = 0; i < ISP_BLC_ID_MAX; ++i) {
		isp_cfgs.blc_cfg[i].enable = true;
		isp_cfgs.blc_cfg[i].bypass = true;
		isp_cfgs.blc_cfg[i].rgain = 0;
		isp_cfgs.blc_cfg[i].grgain = 0;
		isp_cfgs.blc_cfg[i].gbgain = 0;
		isp_cfgs.blc_cfg[i].bgain = 0;
		isp_cfgs.blc_cfg[i].roffset = 0;
		isp_cfgs.blc_cfg[i].groffset = 0;
		isp_cfgs.blc_cfg[i].gboffset = 0;
		isp_cfgs.blc_cfg[i].boffset = 0;
	}

	for (i = 0; i < ISP_WBG_ID_MAX; ++i) {
		isp_cfgs.wbg_cfg[i].enable = true;
		isp_cfgs.wbg_cfg[i].bypass = false;
		isp_cfgs.wbg_cfg[i].rgain = 0x677;
		isp_cfgs.wbg_cfg[i].ggain = 0x400;
		isp_cfgs.wbg_cfg[i].bgain = 0x4B0;
	}
}

#define DMA_SETUP(id)						\
	do {							\
		bufaddr = _mempool_get_addr();			\
		bufsize = ispblk_dma_config(ictx, id, bufaddr); \
		_mempool_pop(bufsize);				\
	} while (0)

void _vi_yuv_dma_setup(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	struct isp_buffer *b;
	uint32_t bufsize_y = 0, bufsize_u = 0, bufsize_v = 0;
	uint8_t  i = 0;

	struct _mempool *pool = (raw_num == ISP_PRERAW_A) ?
				&mempool :
				&mempool_raw1;
	u32 ydma = (raw_num == ISP_PRERAW_A) ?
			ISP_BLK_ID_DMA0 :
			ISP_BLK_ID_DMA53;
	u32 udma = (raw_num == ISP_PRERAW_A) ?
			ISP_BLK_ID_DMA1 :
			ISP_BLK_ID_DMA54;
	u32 vdma = (raw_num == ISP_PRERAW_A) ?
			ISP_BLK_ID_DMA2 :
			ISP_BLK_ID_DMA55;

	for (i = 0; i < OFFLINE_YUV_BUF_NUM; i++) {
		b = kmalloc(sizeof(*b), GFP_ATOMIC);
		b->raw_num = raw_num;
		b->addr = 0;

		bufsize_y = ispblk_dma_config(ctx, ydma, 0);
		b->y_addr = _mempool_pop(bufsize_y);
		ispblk_dma_setaddr(ctx, ydma, b->y_addr);
		pool->yuv_y[i] = b->y_addr;

		bufsize_u = ispblk_dma_config(ctx, udma, 0);
		b->u_addr = _mempool_pop(bufsize_u);
		ispblk_dma_setaddr(ctx, udma, b->u_addr);
		pool->yuv_u[i] = b->u_addr;

		bufsize_v = ispblk_dma_config(ctx, vdma, 0);
		b->v_addr = _mempool_pop(bufsize_v);
		ispblk_dma_setaddr(ctx, vdma, b->v_addr);
		pool->yuv_v[i] = b->v_addr;

		isp_buf_queue(&pre_out_queue[b->raw_num], b);
	}
}

void _vi_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint64_t bufaddr = 0;
	uint32_t bufsize = 0;
	uint8_t  i = 0;
	struct cvi_isp_vdev *vdev =
		container_of(ictx, struct cvi_isp_vdev, ctx);
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	for (; raw_num < raw_max; raw_num++) {
		struct _mempool *pool = (raw_num == ISP_PRERAW_A) ?
					&mempool :
					&mempool_raw1;
		u32 rgbdma = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA15 :
				ISP_BLK_ID_DMA31;
		u32 rgbdma_se = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA16 :
				ISP_BLK_ID_DMA32;
		u32 lmapdma = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA9 :
				ISP_BLK_ID_DMA29;
		u32 lmapdma_se = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA14 :
				ISP_BLK_ID_DMA30;
		//sts
		u32 awb0dma = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA39 :
				ISP_BLK_ID_DMA48;
		u32 ae0dma_le = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA33 :
				ISP_BLK_ID_DMA42;
		u32 ae1dma_le = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA34 :
				ISP_BLK_ID_DMA43;
		u32 hist0dma_le = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA35 :
				ISP_BLK_ID_DMA44;
		u32 gmsdma = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA41 :
				ISP_BLK_ID_DMA50;
		u32 afdma = (raw_num == ISP_PRERAW_A) ?
				ISP_BLK_ID_DMA17 :
				ISP_BLK_ID_DMA18;
		//preraw offline readdma
		u32 pre_offline_dma = (raw_num == ISP_PRERAW_A) ?
						ISP_BLK_ID_DMA8 :
						ISP_BLK_ID_DMA56;

		ictx->img_width = ictx->isp_pipe_cfg[raw_num].crop.w;
		ictx->img_height = ictx->isp_pipe_cfg[raw_num].crop.h;

		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (!ictx->is_offline_scaler) { //Online mode to scaler
				_vi_yuv_dma_setup(ictx, raw_num);
			}
			continue;
		}

		// rgbmap
		bufaddr = _mempool_get_addr();
		ispblk_dma_config(ictx, rgbdma, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, rgbdma);
		_mempool_pop(bufsize);

		pool->rgbmap_le[0] = bufaddr;
		for (i = 1; i < RGBMAP_BUF; i++)
			pool->rgbmap_le[i] = _mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			bufaddr = _mempool_get_addr();
			ispblk_dma_config(ictx, rgbdma_se, bufaddr);
			bufsize = ispblk_dma_buf_get_size(ictx, rgbdma_se);
			_mempool_pop(bufsize);

			pool->rgbmap_se[0] = bufaddr;
			for (i = 1; i < RGBMAP_BUF; i++)
				pool->rgbmap_se[i] = _mempool_pop(bufsize);
		} else {
			pool->rgbmap_se[0] = pool->rgbmap_se[1] = _mempool_pop(bufsize);
		}

		// lmap
		// lmap0
		bufaddr = _mempool_get_addr();
		ispblk_dma_config(ictx, lmapdma, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, lmapdma);
		_mempool_pop(bufsize);
		pool->lmap_le = bufaddr;

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// lmap1
			bufaddr = _mempool_get_addr();
			ispblk_dma_config(ictx, lmapdma_se, bufaddr);
			bufsize = ispblk_dma_buf_get_size(ictx, lmapdma_se);
			_mempool_pop(bufsize);
			pool->lmap_se = bufaddr;
		}

		ispblk_dma_config(ictx, ISP_BLK_ID_DMA12, pool->lmap_le);
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA13, pool->lmap_se);

		// sts
		// awb0 le
		bufaddr = _mempool_get_addr();
		bufsize = ispblk_dma_config(ictx, awb0dma, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, awb0dma);
		_mempool_pop(bufsize);
		pool->sts_mem[0].awb_le.phy_addr = bufaddr;
		pool->sts_mem[0].awb_le.size = bufsize;

		// ae0 le
		bufaddr = _mempool_get_addr();
		bufsize = ispblk_dma_config(ictx, ae0dma_le, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, ae0dma_le);
		_mempool_pop(bufsize);
		pool->sts_mem[0].ae_le0.phy_addr = bufaddr;
		pool->sts_mem[0].ae_le0.size = bufsize;

		// ae1 le
		bufaddr = _mempool_get_addr();
		bufsize = ispblk_dma_config(ictx, ae1dma_le, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, ae1dma_le);
		_mempool_pop(bufsize);
		pool->sts_mem[0].ae_le1.phy_addr = bufaddr;
		pool->sts_mem[0].ae_le1.size = bufsize;

		// hist0 le
		DMA_SETUP(hist0dma_le);
		pool->sts_mem[0].hist_le.phy_addr = bufaddr;
		pool->sts_mem[0].hist_le.size = bufsize;

		// gms
		DMA_SETUP(gmsdma);
		pool->sts_mem[0].gms.phy_addr = bufaddr;
		pool->sts_mem[0].gms.size = bufsize;

		// af
		DMA_SETUP(afdma);
		pool->sts_mem[0].af.phy_addr = bufaddr;
		pool->sts_mem[0].af.size = bufsize;

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			u32 awb1dma = (raw_num == ISP_PRERAW_A) ?
					ISP_BLK_ID_DMA40 :
					ISP_BLK_ID_DMA49;
			u32 ae0dma_se = (raw_num == ISP_PRERAW_A) ?
					ISP_BLK_ID_DMA37 :
					ISP_BLK_ID_DMA46;
			u32 hist0dma_se = (raw_num == ISP_PRERAW_A) ?
					ISP_BLK_ID_DMA38 :
					ISP_BLK_ID_DMA47;

			// awb1 se
			bufaddr = _mempool_get_addr();
			bufsize = ispblk_dma_config(ictx, awb1dma, bufaddr);
			bufsize = ispblk_dma_buf_get_size(ictx, awb1dma);
			_mempool_pop(bufsize);
			pool->sts_mem[0].awb_se.phy_addr = bufaddr;
			pool->sts_mem[0].awb_se.size = bufsize;

			// ae0 se
			bufaddr = _mempool_get_addr();
			bufsize = ispblk_dma_config(ictx, ae0dma_se, bufaddr);
			bufsize = ispblk_dma_buf_get_size(ictx, ae0dma_se);
			_mempool_pop(bufsize);
			pool->sts_mem[0].ae_se.phy_addr = bufaddr;
			pool->sts_mem[0].ae_se.size = bufsize;

			// hist0 se
			DMA_SETUP(hist0dma_se);
			pool->sts_mem[0].hist_se.phy_addr = bufaddr;
			pool->sts_mem[0].hist_se.size = bufsize;
		}

		// awb4 post
		bufaddr = _mempool_get_addr();
		bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA45, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA45);
		_mempool_pop(bufsize);
		pool->sts_mem[0].awb_post.phy_addr = bufaddr;
		pool->sts_mem[0].awb_post.size = bufsize;

		// dci
		DMA_SETUP(ISP_BLK_ID_DMA36);
		pool->sts_mem[0].dci.phy_addr = bufaddr;
		pool->sts_mem[0].dci.size = bufsize;

		pool->sts_mem[1] = pool->sts_mem[0];
		pool->sts_mem[1].awb_le.phy_addr	= _mempool_pop(pool->sts_mem[1].awb_le.size);
		pool->sts_mem[1].ae_le0.phy_addr	= _mempool_pop(pool->sts_mem[1].ae_le0.size);
		pool->sts_mem[1].ae_le1.phy_addr	= _mempool_pop(pool->sts_mem[1].ae_le1.size);
		pool->sts_mem[1].hist_le.phy_addr	= _mempool_pop(pool->sts_mem[1].hist_le.size);
		pool->sts_mem[1].gms.phy_addr		= _mempool_pop(pool->sts_mem[1].gms.size);
		pool->sts_mem[1].awb_post.phy_addr	= _mempool_pop(pool->sts_mem[1].awb_post.size);
		pool->sts_mem[1].dci.phy_addr		= _mempool_pop(pool->sts_mem[1].dci.size);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			pool->sts_mem[1].awb_se.phy_addr  = _mempool_pop(pool->sts_mem[1].awb_se.size);
			pool->sts_mem[1].ae_se.phy_addr   = _mempool_pop(pool->sts_mem[1].ae_se.size);
			pool->sts_mem[1].hist_se.phy_addr = _mempool_pop(pool->sts_mem[1].hist_se.size);
		}

		// preraw bayer
		if (ictx->isp_pipe_cfg[raw_num].is_offline_preraw)
			ispblk_dma_config(ictx, pre_offline_dma, vdev->usr_pic_phy_addr);

		if (ictx->is_offline_postraw) {
			struct isp_buffer *b;
			//uint8_t i = 0;
			u32 rawdma_le = (raw_num == ISP_PRERAW_A) ?
					ISP_BLK_ID_DMA0 :
					ISP_BLK_ID_DMA53;
			u32 rawdma_se = (raw_num == ISP_PRERAW_A) ?
					ISP_BLK_ID_DMA1 :
					ISP_BLK_ID_DMA54;

			bufaddr = _mempool_get_addr();
			ispblk_dma_config(ictx, rawdma_le, bufaddr);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA6, bufaddr);
			bufsize = ispblk_dma_buf_get_size(ictx, rawdma_le);

			for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
				b = kmalloc(sizeof(*b), GFP_ATOMIC);
				if (ictx->is_tile)
					b->addr = _mempool_pop_128(bufsize);
				else
					b->addr = _mempool_pop(bufsize);
				b->raw_num = raw_num;
				pool->bayer_le[i] = b->addr;
				isp_buf_queue(&pre_out_queue[b->raw_num], b);
			}

			ispblk_dma_enable(ictx, rawdma_le, 1);
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA6, 1);

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				struct isp_buffer *b_se;

				bufaddr = _mempool_get_addr();
				ispblk_dma_config(ictx, rawdma_se, bufaddr);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA7, bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, rawdma_se);

				for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
					b_se = kmalloc(sizeof(*b_se), GFP_ATOMIC);
					if (ictx->is_tile)
						b_se->addr = _mempool_pop_128(bufsize);
					else
						b_se->addr = _mempool_pop(bufsize);
					b_se->raw_num = raw_num;
					pool->bayer_se[i] = b_se->addr;
					isp_buf_queue(&pre_out_se_queue[b_se->raw_num], b_se);
				}

				ispblk_dma_enable(ictx, rawdma_se, 1);
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA7, 1);
			}
		} else {
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA0, 0);
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA6, 0);
		}

		// lsc
		DMA_SETUP(ISP_BLK_ID_DMA10);
		pool->lsc_le[0] = bufaddr;
		if (ictx->is_tile)
			pool->lsc_le[1] = _mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			DMA_SETUP(ISP_BLK_ID_DMA11);
			pool->lsc_se[0] = bufaddr;

			if (ictx->is_tile)
				pool->lsc_se[1] = _mempool_pop(bufsize);
		}

		// manr
		if (ictx->is_3dnr_on) {
			DMA_SETUP(ISP_BLK_ID_DMA23);
			pool->manr = bufaddr;
			pool->sts_mem[0].mmap.phy_addr = pool->sts_mem[1].mmap.phy_addr = bufaddr;
			pool->sts_mem[0].mmap.size = pool->sts_mem[1].mmap.size = bufsize;
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA24, pool->manr);

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				pool->manr_rtile = _mempool_pop(bufsize);

			if (ictx->is_offline_postraw) {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA19,
							pool->rgbmap_le[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA21,
							pool->rgbmap_le[1]);
				/* single frm read dummy data for manr */
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA20,
							pool->rgbmap_se[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA22,
							pool->rgbmap_se[1]);
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA19,
							pool->rgbmap_le[0]);
			}

			DMA_SETUP(ISP_BLK_ID_DMA25);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA27, bufaddr);
			pool->tdnr[0] = bufaddr;

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				pool->tdnr_rtile[0] = _mempool_pop(bufsize);

			DMA_SETUP(ISP_BLK_ID_DMA26);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA28, bufaddr);
			pool->tdnr[1] = bufaddr;

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				pool->tdnr_rtile[1] = _mempool_pop(bufsize);
		}
	}

	// postraw yuv
	if (ictx->is_offline_scaler) {
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA3, 0);
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA4, 0);
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA5, 0);
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA3, 1);
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA4, 1);
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA5, 1);
	} else {
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA3, 0);
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA4, 0);
		ispblk_dma_enable(ictx, ISP_BLK_ID_DMA5, 0);
	}

	_mempool_dump(ictx);
}

void _vi_yuv_get_dma_size(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uint32_t bufsize_y = 0, bufsize_u = 0, bufsize_v = 0;
	uint8_t  i = 0;

	u32 ydma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
	u32 udma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA1 : ISP_BLK_ID_DMA54;
	u32 vdma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA2 : ISP_BLK_ID_DMA55;

	for (i = 0; i < OFFLINE_YUV_BUF_NUM; i++) {
		bufsize_y = ispblk_dma_config(ctx, ydma, 0);
		_mempool_pop(bufsize_y);

		bufsize_u = ispblk_dma_config(ctx, udma, 0);
		_mempool_pop(bufsize_u);

		bufsize_v = ispblk_dma_config(ctx, vdma, 0);
		_mempool_pop(bufsize_v);
	}
}

void _vi_get_dma_buf_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint32_t bufsize = 0;
	uint8_t  i = 0;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	for (raw_num; raw_num < raw_max; raw_num++) {
		u32 raw_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
		u32 raw_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA1 : ISP_BLK_ID_DMA54;
		u32 rgbmap_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA15 : ISP_BLK_ID_DMA31;
		u32 rgbmap_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA16 : ISP_BLK_ID_DMA32;
		u32 lmap_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA9 : ISP_BLK_ID_DMA29;
		u32 lmap_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA14 : ISP_BLK_ID_DMA30;
		u32 awb0 = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA39 : ISP_BLK_ID_DMA48;
		u32 awb1 = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA40 : ISP_BLK_ID_DMA49;
		u32 ae0_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA33 : ISP_BLK_ID_DMA42;
		u32 ae0_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA37 : ISP_BLK_ID_DMA46;
		u32 ae1_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA34 : ISP_BLK_ID_DMA43;
		u32 hist0_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA35 : ISP_BLK_ID_DMA44;
		u32 hist0_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA38 : ISP_BLK_ID_DMA47;
		u32 gms = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA41 : ISP_BLK_ID_DMA50;
		u32 af = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA17 : ISP_BLK_ID_DMA18;

		ictx->img_width = ictx->isp_pipe_cfg[raw_num].crop.w;
		ictx->img_height = ictx->isp_pipe_cfg[raw_num].crop.h;

		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (!ictx->is_offline_scaler) { //Online mode to scaler
				_vi_yuv_get_dma_size(ictx, raw_num);
			}
			continue;
		}

		// rgbmap
		bufsize = ispblk_dma_buf_get_size(ictx, rgbmap_le);
		for (i = 0; i < RGBMAP_BUF; i++)
			_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			bufsize = ispblk_dma_buf_get_size(ictx, rgbmap_se);
			for (i = 0; i < RGBMAP_BUF; i++)
				_mempool_pop(bufsize);
		} else {
			_mempool_pop(bufsize);
		}

		// lmap
		bufsize = ispblk_dma_buf_get_size(ictx, lmap_le);
		_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			bufsize = ispblk_dma_buf_get_size(ictx, lmap_se);
			_mempool_pop(bufsize);
		}

		// sts
		// awb0 le
		bufsize = ispblk_dma_buf_get_size(ictx, awb0);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// ae0 le
		bufsize = ispblk_dma_buf_get_size(ictx, ae0_le);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// ae1 le
		bufsize = ispblk_dma_buf_get_size(ictx, ae1_le);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// hist0 le
		bufsize = ispblk_dma_config(ictx, hist0_le, 0);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// gms
		bufsize = ispblk_dma_config(ictx, gms, 0);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// af
		bufsize = ispblk_dma_config(ictx, af, 0);
		_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// awb1 se
			bufsize = ispblk_dma_buf_get_size(ictx, awb1);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);

			// ae0 se
			bufsize = ispblk_dma_buf_get_size(ictx, ae0_se);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);

			// hist0 se
			bufsize = ispblk_dma_config(ictx, hist0_se, 0);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);
		}

		// awb4 post
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA45);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// dci
		bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA36, 0);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		if (ictx->is_offline_postraw) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_le);
			for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
				if (ictx->is_tile)
					_mempool_pop_128(bufsize);
				else
					_mempool_pop(bufsize);
			}

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_se);
				for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
					if (ictx->is_tile)
						_mempool_pop_128(bufsize);
					else
						_mempool_pop(bufsize);
				}
			}
		}

		// lsc
		bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA10, 0);
		_mempool_pop(bufsize);
		if (ictx->is_tile)
			_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA11, 0);
			_mempool_pop(bufsize);
			if (ictx->is_tile)
				_mempool_pop(bufsize);
		}

		// manr
		if (ictx->is_3dnr_on) {
			bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA23, 0);
			_mempool_pop(bufsize);

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				_mempool_pop(bufsize);

			bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA25, 0);
			_mempool_pop(bufsize);

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				_mempool_pop(bufsize);

			bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA26, 0);
			_mempool_pop(bufsize);

			if (ictx->is_tile && raw_num == ISP_PRERAW_A) //only raw_a can run tile
				_mempool_pop(bufsize);
		}
	}
}

static void _isp_rawtop_update(struct cvi_isp_vdev *vdev, u8 tile_update)
{
	struct isp_ctx *ictx = &vdev->ctx;
	union REG_ISP_DPC_2 dpc_reg2;
	uint8_t i = 0;

	if (tile_update) {
		uint32_t bufsize;

		// raw_top
		ispblk_rawtop_tile(ictx);

		if (ictx->is_yuv_sensor) {
			//ispblk_dpc_config(ictx, ISP_RAW_PATH_LE, dpc_reg2);

			ispblk_lsc_config(ictx, ISP_BLK_ID_LSCM0, false);
		} else {
			ispblk_dpc_tile(ictx, ISP_RAW_PATH_LE);
			ispblk_lsc_tile(ictx, ISP_BLK_ID_LSCM0);

			if (ictx->is_work_on_r_tile) {
				bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA10, mempool.lsc_le[1]);
				dprintk(VIP_DBG, "Right tile addr for dma10(lsc le): 0x%llx, size: 0x%x\n",
						mempool.lsc_le[1], bufsize);
			} else
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA10, mempool.lsc_le[0]);
		}

		if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
			ispblk_dpc_tile(ictx, ISP_RAW_PATH_SE);
			ispblk_lsc_tile(ictx, ISP_BLK_ID_LSCM1);

			if (ictx->is_work_on_r_tile) {
				bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA11, mempool.lsc_se[1]);
				dprintk(VIP_DBG, "Right tile addr for dma11(lsc le): 0x%llx, size: 0x%x\n",
						mempool.lsc_se[1], bufsize);
			} else
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA11, mempool.lsc_se[0]);

			ispblk_fusion_tile(ictx);
			//ispblk_ltm_config(ictx, true, true, true, true);
			ispblk_ltm_tile(ictx);
		} else {
			ispblk_fusion_tile(ictx);
			//ispblk_ltm_config(ictx, 0, 0, 0, 0);
			ispblk_ltm_tile(ictx);
		}

		ispblk_manr_tile(ictx);

		if (ictx->is_yuv_sensor)
			ispblk_bnr_config(ictx, ISP_BNR_OUT_BYPASS, false, 0, 0);
		else
			ispblk_bnr_tile(ictx);

		if (ictx->is_work_on_r_tile) {
			u64 dma_addr = 0;

			dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA6);
			if (ictx->is_dpcm_on) {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA6,
					dma_addr + ((ictx->tile_cfg.r_in.start / 4) * 3));
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA6,
					dma_addr + ((ictx->tile_cfg.r_in.start / 2) * 3));
			}

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
				dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA7);
				if (ictx->is_dpcm_on) {
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA7,
							dma_addr + ((ictx->tile_cfg.r_in.start / 4) * 3));
				} else {
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA7,
							dma_addr + ((ictx->tile_cfg.r_in.start / 2) * 3));
				}
			}
		} else {
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA6, 0);

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on)
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA7, 0);
		}

		//Update DMA setting
		if (ictx->is_3dnr_on) {
			u64 dma_addr = 0, base_offset = 0;
			u32 rgbmap_grid_w = 0;

			if (ictx->is_work_on_r_tile) {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA23, mempool.manr_rtile);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA24, mempool.manr_rtile);
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA23, mempool.manr);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA24, mempool.manr);
			}

			if (ictx->is_offline_postraw) {
				if (ictx->is_work_on_r_tile) {
					rgbmap_grid_w = ispblk_rgbmap_get_w_bit(ictx, ISP_BLK_ID_DMA19);

					base_offset = ((((ictx->tile_cfg.r_in.start - 128) / (1 << rgbmap_grid_w)) +
							6) / 7) << 5;

					dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA19);
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA19, dma_addr + base_offset);

					dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA21);
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA21, dma_addr + base_offset);
				} else {
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA19, 0);
					ispblk_dma_config(ictx, ISP_BLK_ID_DMA21, 0);
				}
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA19, 0);
			}

			if (ictx->is_work_on_r_tile) {
				dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA20);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA20, dma_addr + base_offset);

				dma_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA22);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA22, dma_addr + base_offset);
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA20, 0);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA22, 0);
			}
		}
	} else { //Init config HW once
		struct cvi_vip_isp_wbg_config wbg_cfg;

		// raw_top
		ispblk_rawtop_config(ictx);

		if (ictx->is_yuv_sensor) {
			//dpc_reg2.bits.DPC_ENABLE = 0;
			//ispblk_dpc_config(ictx, ISP_RAW_PATH_LE, dpc_reg2);

			//ispblk_blc_enable(ictx, ISP_BLC_ID_PRE0_LE, false, true);
			//ispblk_blc_enable(ictx, ISP_BLC_ID_POST_LE, false, true);
			ispblk_lsc_config(ictx, ISP_BLK_ID_LSCM0, false);
			isp_fpn_config(ictx, ISP_BLK_ID_FPN0, false);
		} else {
			dpc_reg2.bits.DPC_ENABLE = 1;
			dpc_reg2.bits.GE_ENABLE = 1;
			//dpc_reg2.bits.DPC_DYNAMICBPC_ENABLE = 1;
			//dpc_reg2.bits.DPC_STATICBPC_ENABLE = 1;
			//dpc_reg2.bits.DPC_CLUSTER_SIZE = 2;
			ispblk_dpc_config(ictx, ISP_RAW_PATH_LE, dpc_reg2);

			//ispblk_blc_enable(ictx, ISP_BLC_ID_PRE0_LE, true, true);
			//ispblk_blc_enable(ictx, ISP_BLC_ID_POST_LE, true, true);
			ispblk_lsc_config(ictx, ISP_BLK_ID_LSCM0, false);
			isp_fpn_config(ictx, ISP_BLK_ID_FPN0, false);
		}

		if (ictx->is_hdr_on) {
			ispblk_dpc_config(ictx, ISP_RAW_PATH_SE, dpc_reg2);
			ispblk_lsc_config(ictx, ISP_BLK_ID_LSCM1, false);
			//ispblk_blc_enable(ictx, ISP_BLC_ID_PRE0_SE, true, true);
			//ispblk_blc_enable(ictx, ISP_BLC_ID_POST_SE, true, true);
			isp_fpn_config(ictx, ISP_BLK_ID_FPN1, false);
			ispblk_fusion_config(ictx, true, false, false, ISP_FS_OUT_FS);

			ispblk_ltm_b_lut(ictx, 0, ltm_b_lut);
			ispblk_ltm_d_lut(ictx, 0, ltm_d_lut);
			ispblk_ltm_g_lut(ictx, 0, ltm_g_lut);
			ispblk_ltm_config(ictx, true, true, true, true);
			ispblk_ltm_enable(ictx, true, false);
		} else {
			ispblk_fusion_config(ictx, true, true, true, ISP_FS_OUT_LONG);
			ispblk_ltm_config(ictx, 0, 0, 0, 0);
			ispblk_ltm_enable(ictx, true, true);
		}

		ispblk_manr_config(ictx, ictx->is_3dnr_on);

		for (i = 0; i < ISP_BLC_ID_MAX; ++i) {
			ispblk_blc_set_gain(ictx, i,
				isp_cfgs.blc_cfg[i].rgain,
				isp_cfgs.blc_cfg[i].grgain,
				isp_cfgs.blc_cfg[i].gbgain,
				isp_cfgs.blc_cfg[i].bgain);
			ispblk_blc_set_offset(ictx, i,
				isp_cfgs.blc_cfg[i].roffset,
				isp_cfgs.blc_cfg[i].groffset,
				isp_cfgs.blc_cfg[i].gboffset,
				isp_cfgs.blc_cfg[i].boffset);
			ispblk_blc_enable(ictx, i,
				isp_cfgs.blc_cfg[i].enable,
				isp_cfgs.blc_cfg[i].bypass);
		}

		/* for imx327 tuning */
		wbg_cfg.bypass = false;
		wbg_cfg.enable = true;
		wbg_cfg.inst = ISP_WBG_ID_POST_LE;
		wbg_cfg.rgain = 0x70F;
		wbg_cfg.bgain = 0xA06;
		wbg_cfg.ggain = 0x400;

		ispblk_wbg_config(ictx, wbg_cfg.inst, wbg_cfg.rgain, wbg_cfg.ggain, wbg_cfg.bgain);
		ispblk_wbg_enable(ictx, wbg_cfg.inst, true, false);

		if (ictx->is_hdr_on)
			ispblk_wbg_enable(ictx, 3, true, true);

		if (ictx->is_yuv_sensor)
			ispblk_bnr_config(ictx, ISP_BNR_OUT_BYPASS, false, 0, 0);
		else
			ispblk_bnr_config(ictx, ISP_BNR_OUT_B_OUT, false, 0, 0);
	}
}

static void _isp_rgbtop_update(struct cvi_isp_vdev *vdev, u8 tile_update)
{
	struct isp_ctx *ictx = &vdev->ctx;

	if (tile_update) { // Tile update CFA/RGBEE/DHZ/RGBDITHER
		if (!ictx->is_yuv_sensor) {
			ispblk_cfa_tile(ictx);
			ispblk_rgbee_tile(ictx);
			ispblk_dhz_tile(ictx);
			ispblk_rgbdither_tile(ictx);
		}
	} else {
		// rgb_top
		if (!ictx->is_yuv_sensor) {
			struct isp_dhz_cfg dhz_hw_cfg;

			_dhz_cfg_remap(&isp_cfgs.dhz_cfg, &dhz_hw_cfg);

			ispblk_rgb_config(ictx);
			ispblk_cfa_config(ictx);
			ispblk_rgbee_config(ictx, true, 358, 310);
	//		ispblk_gamma_config(ictx, 0, gamma_data);
			ispblk_gamma_enable(ictx, false, 0);
			ispblk_ccm_config(ictx, true, &ccm_hw_cfg);
			ispblk_dhz_config(ictx, isp_cfgs.dhz_cfg.enable, &dhz_hw_cfg);
	//		ispblk_hsv_config(ictx, 0, 0, hsv_sgain_data);
	//		ispblk_hsv_config(ictx, 0, 1, hsv_vgain_data);
	//		ispblk_hsv_config(ictx, 0, 2, hsv_htune_data);
	//		ispblk_hsv_config(ictx, 0, 3, hsv_stune_data);
			ispblk_hsv_enable(ictx, false, 0, true, true, true, true);
			ispblk_rgbdither_config(ictx, true, true, true, true);
			ispblk_csc_config(ictx);
		}
	}
}

static void _isp_yuvtop_update(struct cvi_isp_vdev *vdev, u8 tile_update)
{
	struct isp_ctx *ictx = &vdev->ctx;

	if (tile_update) {
		ispblk_yuvdither_tile(ictx);
		ispblk_444_422_tile(ictx);
		ispblk_cnr_tile(ictx);

		//if (ictx->is_tile && ictx->is_work_on_r_tile)
		//	ispblk_dci_config(ictx, true, dci_lut1, ARRAY_SIZE(dci_lut1));
		//else
		//	ispblk_dci_config(ictx, true, dci_lut0, ARRAY_SIZE(dci_lut0));
		ispblk_dci_tile(ictx);

		ispblk_ee_tile(ictx);

		ispblk_3dlut_config(ictx);

		//Update DMA setting
		if (ictx->is_3dnr_on) {
			if (ictx->is_work_on_r_tile) {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA25, mempool.tdnr_rtile[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA27, mempool.tdnr_rtile[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA26, mempool.tdnr_rtile[1]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA28, mempool.tdnr_rtile[1]);
			} else {
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA25, mempool.tdnr[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA27, mempool.tdnr[0]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA26, mempool.tdnr[1]);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA28, mempool.tdnr[1]);
			}
		}

		if (ictx->is_work_on_r_tile) {
			u64 y_addr = 0, u_addr = 0, v_addr = 0;

			y_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA3);
			u_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA4);
			v_addr = ispblk_dma_getaddr(ictx, ISP_BLK_ID_DMA5);

			ispblk_dma_config(ictx, ISP_BLK_ID_DMA3, y_addr + ictx->tile_cfg.r_out.start);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA4, u_addr + (ictx->tile_cfg.r_out.start >> 1));
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA5, v_addr + (ictx->tile_cfg.r_out.start >> 1));
		} else {
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA3, 0);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA4, 0);
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA5, 0);
		}
	} else {
		// yuv_top
		ispblk_yuvtop_config(ictx);

		if (ictx->is_tile) {
			ispblk_yuvdither_config(ictx, 0, false, true, true, true);
			ispblk_yuvdither_config(ictx, 1, false, true, true, true);
		} else {
			ispblk_yuvdither_config(ictx, 0, true, true, true, true);
			ispblk_yuvdither_config(ictx, 1, true, true, true, true);
		}

		ispblk_444_422_config(ictx);
		ispblk_ynr_config(ictx, ISP_YNR_OUT_Y_OUT, 16, 0);
		ispblk_cnr_config(ictx, true, true, 32);
		ispblk_dci_config(ictx, true, dci_lut0, ARRAY_SIZE(dci_lut0));
		ispblk_ee_config(ictx, false);

		//ispblk_ycur_config(ictx, 0, ycur_data);
		ispblk_ycur_enable(ictx, false, 0);

		ispblk_3dlut_config(ictx);
	}
}

void _isp_tile_cal_size(struct isp_ctx *ictx)
{
	const int guard = 0x40;
	int left_width = ((ictx->img_width >> 1) + 0x7F) & ~0x7F;

	ictx->tile_cfg.l_in.start  = 0;
	ictx->tile_cfg.l_in.end    = left_width - 1 + (guard << 1);
	ictx->tile_cfg.l_out.start = 0;
	ictx->tile_cfg.l_out.end   = left_width - 1 + guard;
	ictx->tile_cfg.r_in.start  = left_width;
	ictx->tile_cfg.r_in.end    = ictx->img_width - 1;
	ictx->tile_cfg.r_out.start = left_width + guard;
	ictx->tile_cfg.r_out.end   = ictx->img_width - 1;

	dprintk(VIP_DBG, "tile cfg: Left in(%d %d) out(%d %d)\n",
		ictx->tile_cfg.l_in.start, ictx->tile_cfg.l_in.end,
		ictx->tile_cfg.l_out.start, ictx->tile_cfg.l_out.end);
	dprintk(VIP_DBG, "tile cfg: Right in(%d %d) out(%d %d)\n",
		ictx->tile_cfg.r_in.start, ictx->tile_cfg.r_in.end,
		ictx->tile_cfg.r_out.start, ictx->tile_cfg.r_out.end);
}

void _vi_ctrl_init(enum cvi_isp_raw raw_num, struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;

	if (vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w != 0) { //MW config snr_info flow
		ictx->isp_pipe_cfg[raw_num].csibdg_width = vdev->snr_info[raw_num].snr_fmt.img_size[0].width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height = vdev->snr_info[raw_num].snr_fmt.img_size[0].height;
		ictx->isp_pipe_cfg[raw_num].max_width =
						vdev->snr_info[raw_num].snr_fmt.img_size[0].max_width;
		ictx->isp_pipe_cfg[raw_num].max_height =
						vdev->snr_info[raw_num].snr_fmt.img_size[0].max_height;

		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->snr_info[raw_num].snr_fmt.img_size[0].start_x;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->snr_info[raw_num].snr_fmt.img_size[0].start_y;

		if (vdev->snr_info[raw_num].snr_fmt.frm_num > 1) { //HDR
			ictx->isp_pipe_cfg[raw_num].crop_se.w =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].active_w;
			ictx->isp_pipe_cfg[raw_num].crop_se.h =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].active_h;
			ictx->isp_pipe_cfg[raw_num].crop_se.x =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].start_x;
			ictx->isp_pipe_cfg[raw_num].crop_se.y =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].start_y;

			ictx->isp_pipe_cfg[raw_num].is_hdr_on = true;
		}

		ictx->rgb_color_mode[raw_num] = _mbus_remap(vdev->snr_info[raw_num].color_mode);

	} else { //snr sub-dev flow
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->sns_crop[raw_num].width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->sns_crop[raw_num].height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->sns_crop[raw_num].left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->sns_crop[raw_num].top;

		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->sns_se_crop[raw_num].width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->sns_se_crop[raw_num].height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->sns_se_crop[raw_num].left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->sns_se_crop[raw_num].top;

		ictx->rgb_color_mode[raw_num] = _mbus_remap(vdev->sns_fmt[raw_num].code);

		ictx->isp_pipe_cfg[raw_num].csibdg_width = vdev->sns_fmt[raw_num].width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height = vdev->sns_fmt[raw_num].height;
		ictx->isp_pipe_cfg[raw_num].max_width = vdev->sns_fmt[raw_num].width;
		ictx->isp_pipe_cfg[raw_num].max_height = vdev->sns_fmt[raw_num].height;
	}

	if (ictx->isp_pipe_cfg[raw_num].is_patgen_en) {
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->usr_crop.top;
		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].csibdg_width	= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height	= vdev->usr_fmt.height;
		ictx->isp_pipe_cfg[raw_num].max_width		= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].max_height		= vdev->usr_fmt.height;

		ictx->rgb_color_mode[raw_num] = vdev->usr_fmt.code;
	} else if (ictx->isp_pipe_cfg[raw_num].is_offline_preraw) {
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->usr_crop.top;
		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].csibdg_width	= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height	= vdev->usr_fmt.height;
		ictx->isp_pipe_cfg[raw_num].max_width		= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].max_height		= vdev->usr_fmt.height;

		ictx->rgb_color_mode[raw_num] = vdev->usr_fmt.code;

		dprintk(VIP_INFO, "csi_bdg=%d:%d, post_crop=%d:%d:%d:%d\n",
				vdev->usr_fmt.width, vdev->usr_fmt.height,
				vdev->usr_crop.width, vdev->usr_crop.height,
				vdev->usr_crop.left, vdev->usr_crop.top);
	}

	ictx->isp_pipe_cfg[raw_num].post_img_w = ictx->isp_pipe_cfg[raw_num].crop.w;
	ictx->isp_pipe_cfg[raw_num].post_img_h = ictx->isp_pipe_cfg[raw_num].crop.h;

	if (!ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
		//Postraw out size
		ictx->img_width = ictx->isp_pipe_cfg[raw_num].crop.w;
		ictx->img_height = ictx->isp_pipe_cfg[raw_num].crop.h;
		ictx->crop_x = ictx->isp_pipe_cfg[raw_num].crop.x;
		ictx->crop_y = ictx->isp_pipe_cfg[raw_num].crop.y;
		ictx->crop_se_x = ictx->isp_pipe_cfg[raw_num].crop_se.x;
		ictx->crop_se_y = ictx->isp_pipe_cfg[raw_num].crop_se.y;
	}

	if (vdev->snr_info[ISP_PRERAW_A].snr_fmt.frm_num > 1) {
		dprintk(VIP_INFO, "frm_num(%d) enable HDR\n", vdev->snr_info[ISP_PRERAW_A].snr_fmt.frm_num);
		vdev->ctx.is_hdr_on = true;
	}

	if (ictx->img_width > 2688) {
		//set vip_sys_0 to 375M
		//iowrite32((4 << 16 | 0x09), ioremap(0x030020d0, 0x4));
		clk_set_rate(clk_get_parent(vdev->isp_clk[0]), 375000000);
		ictx->is_tile = true;
		//ictx->is_dual_sensor = false;
	}

	if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on || vdev->ctx.is_tile)
		vdev->ctx.is_offline_postraw = true;

	if (raw_num == ISP_PRERAW_A) {
		if (ictx->is_tile)
			_isp_tile_cal_size(ictx);
		isp_init(ictx);
	}
}

void _vi_preraw_ctrl_setup(enum cvi_isp_raw raw_num, struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	struct vip_rect crop, crop_se;

	crop = ictx->isp_pipe_cfg[raw_num].crop;
	crop_se = ictx->isp_pipe_cfg[raw_num].crop_se;

	// preraw
	ispblk_preraw_config(ictx, raw_num);
	if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {//YUV sensor
		ispblk_csibdg_yuv_bypass_config(ictx, raw_num, (ictx->is_yuv_early_path) ? early_line : 0);
		if (ictx->is_offline_scaler) //offline mode scaler
			_isp_yuv_bypass_buf_enq(vdev, raw_num);
	} else { //RGB sensor
		ispblk_csibdg_config(ictx, raw_num);

		ispblk_crop_config(ictx, ISP_BLK_ID_CROP0, crop, raw_num);
		ispblk_crop_config(ictx, ISP_BLK_ID_CROP2, crop, raw_num);
		ispblk_lscr_set_lut(ictx, ISP_BLK_ID_LSCR0, lscr_lut, ARRAY_SIZE(lscr_lut), raw_num);
		ispblk_lscr_config(ictx, ISP_BLK_ID_LSCR0, false, raw_num);

		ispblk_rgbmap_config(ictx, ISP_BLK_ID_RGBMAP0, raw_num);
		ispblk_lmap_config(ictx, ISP_BLK_ID_LMP0, true, raw_num);

		ispblk_awb_config(ictx, ISP_BLK_ID_AWB0, true, raw_num);
		ispblk_aehist_config(ictx, ISP_BLK_ID_AEHIST0, true, raw_num);
		ispblk_gms_config(ictx, ISP_BLK_ID_GMS, true, raw_num);

		ispblk_af_config(ictx, ISP_BLK_ID_AF, true, raw_num);
		ispblk_af_gamma_config(ictx, ISP_BLK_ID_AF_GAMMA, 0, af_gamma_data, raw_num);
		ispblk_af_gamma_enable(ictx, ISP_BLK_ID_AF_GAMMA, true, 0, raw_num);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			ispblk_lmap_config(ictx, ISP_BLK_ID_LMP1, true, raw_num);

			ispblk_crop_config(ictx, ISP_BLK_ID_CROP1, crop_se, raw_num);
			ispblk_crop_config(ictx, ISP_BLK_ID_CROP3, crop_se, raw_num);
			ispblk_lscr_set_lut(ictx, ISP_BLK_ID_LSCR1, lscr_lut, ARRAY_SIZE(lscr_lut), raw_num);
			ispblk_lscr_config(ictx, ISP_BLK_ID_LSCR1, false, raw_num);

			ispblk_rgbmap_config(ictx, ISP_BLK_ID_RGBMAP1, raw_num);

			ispblk_awb_config(ictx, ISP_BLK_ID_AWB1, true, raw_num);
			ispblk_aehist_config(ictx, ISP_BLK_ID_AEHIST1, true, raw_num);
		}
	}
}

void _vi_postraw_ctrl_setup(struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u8 cfg_post = false;

	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_post = true;
	} else if (ictx->is_dual_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_post = true;
	}

	if (cfg_post) {
		ispblk_awb_config(ictx, ISP_BLK_ID_AWB4, true, ISP_PRERAW_A);

		_isp_rawtop_update(vdev, 0);
		_isp_rgbtop_update(vdev, 0);
		_isp_yuvtop_update(vdev, 0);

		ispblk_crop_enable(ictx, ISP_BLK_ID_CROP4, false);
		ispblk_crop_enable(ictx, ISP_BLK_ID_CROP5, false);
		ispblk_crop_enable(ictx, ISP_BLK_ID_CROP6, false);
	}

	ispblk_isptop_config(ictx);
}

static void _preraw_tuning_update(
	struct cvi_isp_vdev *vdev,
	struct cvi_vip_isp_pre_tun_cfg *pre_cfg,
	enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t i = 0;
	static int stop_update = -1;

	if (tuning_dis[1]) {
		if (stop_update > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update = 1; // stop on next
	} else
		stop_update = 0;

	for (i = 0; i < 2; i++) {
		struct cvi_vip_isp_blc_config *cfg;
		struct cvi_vip_isp_wbg_config *wbg;
		struct cvi_vip_isp_ae_config  *ae;
		struct cvi_vip_isp_awb_config *awb;

		cfg = &pre_cfg->blc_cfg[i];
		wbg = &pre_cfg->wbg_cfg[i];
		ae  = &pre_cfg->ae_cfg[i];
		awb = &pre_cfg->awb_cfg[i];

		if (cfg->update) {
			ispblk_blc_set_gain(ctx, cfg->inst,
				cfg->rgain, cfg->grgain,
				cfg->gbgain, cfg->bgain);
			ispblk_blc_set_offset(ctx, cfg->inst,
				cfg->roffset, cfg->groffset,
				cfg->gboffset, cfg->boffset);
			ispblk_blc_enable(ctx, cfg->inst,
				cfg->enable, cfg->bypass);
		}

		if (wbg->update) {
			ispblk_wbg_config(ctx, wbg->inst,
				wbg->rgain, wbg->ggain, wbg->bgain);
			ispblk_wbg_enable(ctx, wbg->inst,
				wbg->enable, wbg->bypass);
		}

		if (ae->update)
			ispblk_ae_tun_cfg(ctx, ae);

		if (awb->update)
			ispblk_awb_tun_cfg(ctx, awb);
	}

	{
		struct cvi_vip_isp_lscr_config *cfg;

		cfg = &pre_cfg->lscr_cfg;
		ispblk_lscr_tun_cfg(ctx, cfg);
	}

	{
		struct cvi_vip_isp_af_config *af;

		af = &pre_cfg->af_cfg;
		if (af->update)
			ispblk_af_tun_cfg(ctx, af);
	}

	{
		struct cvi_vip_isp_gms_config *cfg;

		cfg = &pre_cfg->gms_cfg;
		if (cfg->update)
			ispblk_gms_tun_cfg(ctx, cfg, raw_num);
	}
}

static void _postraw_tuning_update(
	struct cvi_isp_vdev *vdev,
	struct cvi_vip_isp_post_tun_cfg *post_tun,
	enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t i = 0;
	static int stop_update = -1;

	if (tuning_dis[2]) {
		if (stop_update > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update = 1;
			return;
		} else if ((tuning_dis[0] - 1) == raw_num)
			stop_update = 1; // stop on next
	} else
		stop_update = 0;

	for (i = 0; i < 3; i++) {
		struct cvi_vip_isp_wbg_config *wbg;

		wbg = &post_tun->wbg_cfg[i];

		if (wbg->update) {
			ispblk_wbg_config(ctx, wbg->inst,
				wbg->rgain, wbg->ggain, wbg->bgain);
			ispblk_wbg_enable(ctx, wbg->inst,
				wbg->enable, wbg->bypass);
		}
	}

	for (i = 0; i < 2; i++) {
		struct cvi_vip_isp_blc_config *cfg;
		struct cvi_vip_isp_ge_config *ge_cfg;
		struct cvi_vip_isp_dpc_config *dpc_cfg;

		cfg = &post_tun->blc_cfg[i];
		ge_cfg = &post_tun->ge_cfg[i];
		dpc_cfg = &post_tun->dpc_cfg[i];

		if (cfg->update) {
			ispblk_blc_set_gain(ctx, cfg->inst,
				cfg->rgain, cfg->grgain,
				cfg->gbgain, cfg->bgain);
			ispblk_blc_set_offset(ctx, cfg->inst,
				cfg->roffset, cfg->groffset,
				cfg->gboffset, cfg->boffset);
			ispblk_blc_enable(ctx, cfg->inst,
				cfg->enable, cfg->bypass);
		}

		ispblk_ge_tun_cfg(ctx, ge_cfg);
		ispblk_dpc_tun_cfg(ctx, dpc_cfg);
	}

	{
		struct cvi_vip_isp_dhz_config *cfg;
		struct isp_dhz_cfg hw_cfg;

		cfg = &post_tun->dhz_cfg;
		if (cfg->update) {
			_dhz_cfg_remap(cfg, &hw_cfg);
			ispblk_dhz_config(ctx, cfg->enable, &hw_cfg);
		}
	}

	{
		struct cvi_vip_isp_ccm_config *cfg;
		struct isp_ccm_cfg hw_cfg;

		cfg = &post_tun->ccm_cfg;
		if (cfg->update) {
			memcpy_fromio(&hw_cfg, cfg->coef, sizeof(hw_cfg));
			ispblk_ccm_config(ctx, cfg->enable, &hw_cfg);
		}
	}

	ISP_RUNTIME_TUN(fswdr);

	{
		struct cvi_vip_isp_lsc_config *cfg;

		cfg = &post_tun->lsc_cfg;
		ispblk_lsc_tun_cfg(ctx, cfg, raw_num);
	}

	{
		struct cvi_vip_isp_drc_config *cfg;

		cfg = &post_tun->drc_cfg;
		ispblk_drc_tun_cfg(ctx, cfg, raw_num);
	}

	{
		struct cvi_vip_isp_tnr_config *cfg;

		cfg = &post_tun->tnr_cfg;
		ispblk_tnr_tun_cfg(ctx, cfg, raw_num);
	}

	ISP_RUNTIME_TUN(bnr);
	ISP_RUNTIME_TUN(demosiac);
	ISP_RUNTIME_TUN(gamma);
	ISP_RUNTIME_TUN(hsv);
	ISP_RUNTIME_TUN(ynr);
	ISP_RUNTIME_TUN(pfc);
	ISP_RUNTIME_TUN(cnr);
	ISP_RUNTIME_TUN(dci);
	ISP_RUNTIME_TUN(ee);
	ISP_RUNTIME_TUN(ycur);

	{
		struct cvi_vip_isp_mono_config *cfg;
		static u8 mono_mode_sts[ISP_PRERAW_MAX];

		cfg = &post_tun->mono_cfg;
		ispblk_mono_tun_cfg(ctx, cfg);

		if (cfg->force_mono_enable != mono_mode_sts[raw_num]) {
			isp_first_frm_reset(ctx, 1);
		} else {
			isp_first_frm_reset(ctx, 0);
		}

		mono_mode_sts[raw_num] = cfg->force_mono_enable;
	}

	{
		struct cvi_vip_isp_3dlut_config *cfg;

		cfg = &post_tun->thrdlut_cfg;
		ispblk_3dlut_tun_cfg(ctx, cfg);
	}
}

static void _isp_crop_update_chk(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_crop_node *node)
{
	enum ISP_BLK_ID_T p_dma_le, p_dma_se;
	u16 i = 0, del_node = true, frm_n = node->n.wdr.frm_num;
	unsigned long flags;

	p_dma_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
	p_dma_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA1 : ISP_BLK_ID_DMA54;

	if (node->n.dly_frm_num == 0) {
		for (; i < frm_n; i++) {
			struct vip_rect crop;

			crop.x = node->n.wdr.img_size[i].start_x;
			crop.y = node->n.wdr.img_size[i].start_y;
			crop.w = node->n.wdr.img_size[i].active_w;
			crop.h = node->n.wdr.img_size[i].active_h;
			vdev->ctx.isp_pipe_cfg[raw_num].csibdg_width =
						node->n.wdr.img_size[0].width;
			vdev->ctx.isp_pipe_cfg[raw_num].csibdg_height =
						node->n.wdr.img_size[0].height;

			dprintk(VIP_DBG, "Preraw_%d, frm_no=%d, crop_x:y:w:h=%d:%d:%d:%d\n",
				raw_num, i, crop.x, crop.y, crop.w, crop.h);

			if (i == 0) { //long expo
				spin_lock_irqsave(&byr_dump, flags);
				ispblk_crop_config(&vdev->ctx, ISP_BLK_ID_CROP0,
								crop,
								raw_num);
				ispblk_csibdg_update_size(&vdev->ctx, raw_num);
				spin_unlock_irqrestore(&byr_dump, flags);
				ispblk_dma_config(&vdev->ctx, p_dma_le, 0);
			} else if (i == 1) { //short expo
				spin_lock_irqsave(&byr_dump, flags);
				ispblk_crop_config(&vdev->ctx, ISP_BLK_ID_CROP1,
								crop,
								raw_num);
				spin_unlock_irqrestore(&byr_dump, flags);
				ispblk_dma_config(&vdev->ctx, p_dma_se, 0);
			}
		}
	} else {
		node->n.dly_frm_num--;
		del_node = false;
	}

	if (del_node) {
		dprintk(VIP_DBG, "crop del node and free\n");
		spin_lock_irqsave(&snr_node_lock[raw_num], flags);
		list_del_init(&node->list);
		kfree(node);
		spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
	}
}

static void _isp_crop_update(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_crop_node **_crop_n,
	const u16 _crop_num)
{
	struct _isp_crop_node *node;
	u16 i = 0;

	if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw || vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en)
		return;

	for (i = 0; i < _crop_num; i++) {
		node = _crop_n[i];

		_isp_crop_update_chk(vdev, raw_num, node);
	}
}

static void _snr_i2c_update(
	const struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_snr_i2c_node **_i2c_n,
	const u16 _i2c_num)
{
	struct _isp_snr_i2c_node *node;
	struct isp_i2c_data *i2c_data;
	unsigned long flags;
	u16 i = 0, j = 0, del_node = true;
	uint32_t dev_mask = 0;
	uint32_t cmd = burst_i2c_en ? CVI_SNS_I2C_BURST_QUEUE : CVI_SNS_I2C_WRITE;

	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	if (ctx->isp_pipe_cfg[raw_num].is_offline_preraw || ctx->isp_pipe_cfg[raw_num].is_patgen_en)
		return;

	for (j = 0; j < _i2c_num; j++) {
		node = _i2c_n[j];

		for (i = 0; i < node->n.regs_num; i++) {
			i2c_data = &node->n.i2c_data[i];

			if (i2c_data->update) {
				if (i2c_data->dly_frm_num == 0) {
					dprintk(VIP_DBG, "i2c_addr=0x%x write:0x%x\n",
							i2c_data->reg_addr, i2c_data->data);
					vip_sys_cmm_cb_i2c(cmd, (void *)i2c_data);
					i2c_data->update = 0;
					if (burst_i2c_en)
						dev_mask |= BIT(i2c_data->i2c_dev);

					if (i2c_data->drop_frame) {
						ctx->isp_pipe_cfg[raw_num].drop_frm_cnt =
							i2c_data->drop_frame_cnt;
						ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num =
							vdev->preraw_frame_number[raw_num] + 1;
					}
				} else {
					dprintk(VIP_DBG, "addr=0x%x, dly_frm=%d\n",
							i2c_data->reg_addr, i2c_data->dly_frm_num);
					i2c_data->dly_frm_num--;
					del_node = false;
				}
			}
		}

		if (del_node) {
			dprintk(VIP_DBG, "i2c del node and free\n");
			spin_lock_irqsave(&snr_node_lock[raw_num], flags);
			list_del_init(&node->list);
			kfree(node);
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
		}
	}

	while (dev_mask) {
		uint32_t tmp = ffs(dev_mask) - 1;

		vip_sys_cmm_cb_i2c(CVI_SNS_I2C_BURST_FIRE, (void *)&tmp);
		dev_mask &= ~BIT(tmp);
	}
}

static void _isp_snr_cfg_deq_and_fire(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct list_head *pos, *temp;
	struct _isp_snr_i2c_node *i2c_n[20];
	struct _isp_crop_node    *crop_n[20];
	unsigned long flags;
	u16 i2c_num = 0, crop_num = 0;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);

	list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
		i2c_n[i2c_num] = list_entry(pos, struct _isp_snr_i2c_node, list);
		i2c_num++;
	}

	list_for_each_safe(pos, temp, &isp_crop_queue[raw_num].list) {
		if (crop_num < i2c_num) {
			crop_n[crop_num] = list_entry(pos, struct _isp_crop_node, list);
			crop_num++;
		}
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);

	_snr_i2c_update(vdev, raw_num, i2c_n, i2c_num);
	_isp_crop_update(vdev, raw_num, crop_n, crop_num);
}

static void _swap_pre_sts_buf(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	uint8_t idx;
	struct isp_ctx *ctx = &vdev->ctx;
	unsigned long flags;

	if (raw_num == ISP_PRERAW_A) {
		spin_lock_irqsave(&mempool.pre_sts_lock, flags);
		if (mempool.pre_sts_in_use == 1) {
			spin_unlock_irqrestore(&mempool.pre_sts_lock, flags);
			return;
		}

		mempool.pre_sts_busy_idx ^= 1;
		spin_unlock_irqrestore(&mempool.pre_sts_lock, flags);

		idx = mempool.pre_sts_busy_idx;

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA39,
				   mempool.sts_mem[idx].awb_le.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA33,
				   mempool.sts_mem[idx].ae_le0.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA34,
				   mempool.sts_mem[idx].ae_le1.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA35,
				   mempool.sts_mem[idx].hist_le.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA41,
				   mempool.sts_mem[idx].gms.phy_addr);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA40,
					   mempool.sts_mem[idx].awb_se.phy_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA37,
					   mempool.sts_mem[idx].ae_se.phy_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA38,
					   mempool.sts_mem[idx].hist_se.phy_addr);
		}
	} else if (raw_num == ISP_PRERAW_B) {
		spin_lock_irqsave(&mempool_raw1.pre_sts_lock, flags);
		if (mempool_raw1.pre_sts_in_use == 1) {
			spin_unlock_irqrestore(&mempool_raw1.pre_sts_lock, flags);
			return;
		}

		mempool_raw1.pre_sts_busy_idx ^= 1;
		spin_unlock_irqrestore(&mempool_raw1.pre_sts_lock, flags);

		idx = mempool_raw1.pre_sts_busy_idx;

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA48,
				   mempool_raw1.sts_mem[idx].awb_le.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA42,
				   mempool_raw1.sts_mem[idx].ae_le0.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA43,
				   mempool_raw1.sts_mem[idx].ae_le1.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA44,
				   mempool_raw1.sts_mem[idx].hist_le.phy_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA50,
				   mempool_raw1.sts_mem[idx].gms.phy_addr);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA49,
					   mempool_raw1.sts_mem[idx].awb_se.phy_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA46,
					   mempool_raw1.sts_mem[idx].ae_se.phy_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA47,
					   mempool_raw1.sts_mem[idx].hist_se.phy_addr);
		}
	}
}

static inline void _swap_post_sts_buf(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	struct _mempool *pool;
	uint8_t idx;
	unsigned long flags;

	pool = (raw_num == ISP_PRERAW_A) ? &mempool : &mempool_raw1;

	spin_lock_irqsave(&pool->post_sts_lock, flags);
	if (pool->post_sts_in_use == 1) {
		spin_unlock_irqrestore(&pool->post_sts_lock, flags);
		return;
	}
	pool->post_sts_busy_idx ^= 1;
	spin_unlock_irqrestore(&pool->post_sts_lock, flags);

	idx = pool->post_sts_busy_idx;

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA45, pool->sts_mem[idx].awb_post.phy_addr);
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA36, pool->sts_mem[idx].dci.phy_addr);
}

void _cvi_isp_buf_queue(struct cvi_isp_base_vdev *vdev, struct cvi_vip_buffer *b)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue[vdev->chn_id]);
	++vdev->num_rdy[vdev->chn_id];
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

struct cvi_vip_buffer *_cvi_isp_next_buf(struct cvi_isp_base_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;
	struct cvi_vip_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue[raw_num]))
		b = list_first_entry(&vdev->rdy_queue[raw_num], struct cvi_vip_buffer, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

int cvi_isp_rdy_buf_empty(struct cvi_isp_base_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	empty = (vdev->num_rdy[raw_num] == 0);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return empty;
}

void cvi_isp_rdy_buf_pop(struct cvi_isp_base_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	vdev->num_rdy[raw_num]--;
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

struct cvi_vip_buffer *cvi_isp_rdy_buf_remove(struct cvi_isp_base_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;
	struct cvi_vip_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue[raw_num])) {
		b = list_first_entry(&vdev->rdy_queue[raw_num], struct cvi_vip_buffer, list);
		list_del_init(&b->list);
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

static inline void _pre_rgbmap_update(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 rgbmap_idx = (vdev->preraw_frame_number[raw_num]) % RGBMAP_BUF;

	if (raw_num == ISP_PRERAW_A)
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA15, mempool.rgbmap_le[rgbmap_idx]);
	else
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA31, mempool_raw1.rgbmap_le[rgbmap_idx]);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		if (raw_num == ISP_PRERAW_A)
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA16, mempool.rgbmap_se[rgbmap_idx]);
		else
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA32, mempool_raw1.rgbmap_se[rgbmap_idx]);
	}
}

static int _preraw_outbuf_enque(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	enum ISP_BLK_ID_T pre_dma_le, pre_dma_se;

	b = isp_next_buf(&pre_out_queue[raw_num]);
	if (!b) {
		dprintk(VIP_DBG, "preraw_%d outbuf is empty\n", raw_num);
		return 0;
	}

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
		struct isp_buffer *b_se = NULL;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			b_se = isp_next_buf(&pre_out_se_queue[raw_num]);
			if (!b_se) {
				dprintk(VIP_DBG, "preraw_%d se outbuf is empty\n", raw_num);
				return 0;
			}
		}

		pre_dma_le = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
		pre_dma_se = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA1 : ISP_BLK_ID_DMA54;

		ispblk_dma_setaddr(ctx, pre_dma_le, b->addr);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			ispblk_dma_setaddr(ctx, pre_dma_se, b_se->addr);
	} else { //YUV sensor online mode
		u32 y_dma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
		u32 u_dma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA1 : ISP_BLK_ID_DMA54;
		u32 v_dma = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA2 : ISP_BLK_ID_DMA55;

		ispblk_dma_setaddr(ctx, y_dma, b->y_addr);
		ispblk_dma_setaddr(ctx, u_dma, b->u_addr);
		ispblk_dma_setaddr(ctx, v_dma, b->v_addr);
	}

	return 1;
}

/*
 * for postraw offline only.
 *  trig preraw if there is output buffer in preraw output.
 */
static void _pre_hw_enque(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (vdev == NULL) {
		dprintk(VIP_ERR, "NULL pointer err\n");
		return;
	}

	if (atomic_cmpxchg(&vdev->preraw_state[raw_num],
				ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
				ISP_PRERAW_RUNNING) {
		dprintk(VIP_DBG, "Preraw_%d is running\n", raw_num);
		return;
	}

	// only if postraw offline
	if (_preraw_outbuf_enque(vdev, raw_num) && (atomic_read(&vdev->isp_streamoff) == 0))
		isp_pre_trig(ctx, raw_num);
	else
		atomic_set(&vdev->preraw_state[raw_num], ISP_PRERAW_IDLE);
}

static int _postraw_inbuf_enq_check(struct cvi_isp_vdev *vdev, enum cvi_isp_raw *raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;

	b = isp_next_buf(&post_in_queue);
	if (b == NULL) {
		dprintk(VIP_DBG, "postraw input buf is empty\n");
		ret = 1;
		return ret;
	}

	*raw_num = b->raw_num;

	//YUV senosr offline mode should not be here
	if (ctx->isp_pipe_cfg[b->raw_num].is_yuv_bypass_path && ctx->is_offline_scaler) {
		ret = 1;
		return ret;
	}

	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.x = b->crop_le.x;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.y = b->crop_le.y;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.w = vdev->ctx.img_width = ctx->isp_pipe_cfg[b->raw_num].post_img_w;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.h = vdev->ctx.img_height = ctx->isp_pipe_cfg[b->raw_num].post_img_h;

	if (ctx->isp_pipe_cfg[b->raw_num].is_yuv_bypass_path) { //YUV sensor online mode
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA59, b->y_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA60, b->u_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA61, b->v_addr);
		return ret;
	}

	if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
		b_se = isp_next_buf(&post_in_se_queue);
		if (b_se == NULL) {
			dprintk(VIP_DBG, "postraw se input buf is empty\n");
			ret = 1;
			return ret;
		}
	}

	if ((ctx->is_dual_sensor) && (!ctx->isp_pipe_cfg[b->raw_num].is_yuv_bypass_path)) {
		if ((tuning_dis[0] > 0) && ((tuning_dis[0] - 1) != b->raw_num)) {
			dprintk(VIP_DBG, "input buf is not equal to current tuning number\n");

			b = isp_buf_remove(&post_in_queue);
			isp_buf_queue(&pre_out_queue[b->raw_num], b);

			if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
				b_se = isp_buf_remove(&post_in_se_queue);
				isp_buf_queue(&pre_out_se_queue[b->raw_num], b_se);
			}

			ret = 1;
			return ret;
		}
	}

	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.w_bit = b->map_info.w_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.h_bit = b->map_info.h_bit;

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA6, b->addr);
	if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
		vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.x = b_se->crop_se.x;
		vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.y = b_se->crop_se.y;
		vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.w = vdev->ctx.img_width;
		vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.h = vdev->ctx.img_height;

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA7, b_se->addr);
	}

	return ret;
}

static void _postraw_outbuf_enque(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct vb2_buffer *vb2_buf;
	struct cvi_vip_buffer *b = NULL;
	struct isp_ctx *ctx = &vdev->ctx;
	u64 tmp_addr = 0, i;

	//Get the buffer for postraw output buffer
	b = _cvi_isp_next_buf((struct cvi_isp_base_vdev *)vdev, raw_num);
	vb2_buf = &b->vb.vb2_buf;

	dprintk(VIP_DBG, "update isp-buf: 0x%lx-0x%lx-0x%lx\n",
		vb2_buf->planes[0].m.userptr, vb2_buf->planes[1].m.userptr,
		vb2_buf->planes[2].m.userptr);

	for (i = 0; i < 3; i++) {
		tmp_addr = (u64)vb2_buf->planes[i].m.userptr;
		if (!(vb2_buf->planes[i].m.userptr & 0x100000000))
			tmp_addr += 0x100000000;
		ispblk_dma_config(ctx, ISP_BLK_ID_DMA3 + i, tmp_addr);
	}
}

static u8 _postraw_outbuf_empty(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	u8 ret = 0;

	if (cvi_isp_rdy_buf_empty((struct cvi_isp_base_vdev *)vdev, raw_num)) {
		dprintk(VIP_DBG, "Postraw output buffer is empty\n");
		ret = 1;
	}

	return ret;
}

static inline void _postraw_outbuf_enq(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	cvi_isp_rdy_buf_pop((struct cvi_isp_base_vdev *)vdev, raw_num);
	_postraw_outbuf_enque(vdev, raw_num);
}

void _isp_v4l2_event_queue(
	struct cvi_isp_vdev *vdev, const u32 type, const u32 frame_num)
{
	struct v4l2_event event = {
		.type = type,
		.u.frame_sync.frame_sequence = frame_num,
	};

	v4l2_event_queue(&vdev->vdev, &event);
}

static void _isp_left_tile(struct cvi_isp_vdev *vdev, enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ictx = &vdev->ctx;
	struct vip_rect crop;

	crop.x = ictx->crop_x;
	crop.y = ictx->crop_y;
	crop.w = ictx->tile_cfg.l_in.end - ictx->tile_cfg.l_in.start + 1;
	crop.h = ictx->img_height;

	ictx->is_work_on_r_tile = false;
	ictx->img_width = crop.w;

	// input crop
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP2, crop, raw_num);
	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		crop.x = ictx->crop_se_x;
		crop.y = ictx->crop_se_y;
		ispblk_crop_config(ictx, ISP_BLK_ID_CROP3, crop, raw_num);
	}

	// output crop
	memset(&crop, 0, sizeof(crop));
	crop.h = ictx->img_height;
	crop.w = ictx->tile_cfg.l_out.end - ictx->tile_cfg.l_out.start + 1;
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP4, crop, raw_num);
	crop.w >>= 1;
	crop.h >>= 1;
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP5, crop, raw_num);
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP6, crop, raw_num);

	_isp_rawtop_update(vdev, 1);
	_isp_rgbtop_update(vdev, 1);
	_isp_yuvtop_update(vdev, 1);
}

static void _isp_right_tile(struct cvi_isp_vdev *vdev, enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ictx = &vdev->ctx;
	struct vip_rect crop;

	crop.x = ictx->crop_x;
	crop.y = ictx->crop_y;
	crop.w = ictx->tile_cfg.r_in.end - ictx->tile_cfg.r_in.start + 1;
	crop.h = ictx->img_height;

	ictx->is_work_on_r_tile = true;
	ictx->img_width = crop.w;

	// for input
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP2, crop, raw_num);
	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		crop.x = ictx->crop_se_x;
		crop.y = ictx->crop_se_y;
		ispblk_crop_config(ictx, ISP_BLK_ID_CROP3, crop, raw_num);
	}

	// for output
	memset(&crop, 0, sizeof(crop));
	crop.h = ictx->img_height;
	crop.x = ictx->tile_cfg.r_out.start - ictx->tile_cfg.r_in.start;
	crop.w = ictx->tile_cfg.r_out.end - ictx->tile_cfg.r_out.start + 1;
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP4, crop, raw_num);
	crop.x >>= 1;
	crop.y >>= 1;
	crop.w >>= 1;
	crop.h >>= 1;
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP5, crop, raw_num);
	ispblk_crop_config(ictx, ISP_BLK_ID_CROP6, crop, raw_num);

	_isp_rawtop_update(vdev, 1);
	_isp_rgbtop_update(vdev, 1);
	_isp_yuvtop_update(vdev, 1);
}

static inline void _post_in_crop_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	struct vip_rect crop, crop_se;

	memcpy(&crop, &ctx->isp_pipe_cfg[raw_num].crop, sizeof(struct vip_rect));
	ispblk_crop_config(ctx, ISP_BLK_ID_CROP2, crop, raw_num);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		memcpy(&crop_se, &ctx->isp_pipe_cfg[raw_num].crop_se, sizeof(struct vip_rect));
		ispblk_crop_config(ctx, ISP_BLK_ID_CROP3, crop_se, raw_num);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA7, true);
		ispblk_crop_enable(ctx, ISP_BLK_ID_CROP3, true);
	} else {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA7, false);
		ispblk_crop_enable(ctx, ISP_BLK_ID_CROP3, false);
	}
}

static inline void _post_lmap_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	u64 lmap_le = (raw_num == ISP_PRERAW_A) ? mempool.lmap_le : mempool_raw1.lmap_le;
	u64 lmap_se = (raw_num == ISP_PRERAW_A) ? mempool.lmap_se : mempool_raw1.lmap_se;

	if (!ctx->is_tile)
		ispblk_ltm_cfg_update(ctx, raw_num);

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA12, lmap_le);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->isp_pipe_cfg[raw_num].is_hdr_detail_en)
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA13, lmap_se);
	else
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA13, lmap_le);
}

static inline void _mlsc_dma_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uint64_t dma_10 = (raw_num == ISP_PRERAW_A) ? mempool.lsc_le[0] : mempool_raw1.lsc_le[0];

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA10, dma_10);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		uint64_t dma_11 = (raw_num == ISP_PRERAW_A) ? mempool.lsc_se[0] : mempool_raw1.lsc_se[0];

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA11, dma_11);
	}
}

static inline void _post_dma_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uint64_t y_addr = (raw_num == ISP_PRERAW_A) ? mempool.tdnr[0] : mempool_raw1.tdnr[0];
	uint64_t uv_addr = (raw_num == ISP_PRERAW_A) ? mempool.tdnr[1] : mempool_raw1.tdnr[1];
	uint64_t manr_addr = (raw_num == ISP_PRERAW_A) ? mempool.manr : mempool_raw1.manr;

	ispblk_dma_config(ctx, ISP_BLK_ID_DMA25, y_addr);
	ispblk_dma_config(ctx, ISP_BLK_ID_DMA27, y_addr);

	ispblk_dma_config(ctx, ISP_BLK_ID_DMA26, uv_addr);
	ispblk_dma_config(ctx, ISP_BLK_ID_DMA28, uv_addr);

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA23, manr_addr);
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA24, manr_addr);

	//_post_lmap_update(ctx, raw_num);

	_mlsc_dma_update(ctx, raw_num);
}

static inline void _post_rgbmap_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const u32 frm_num)
{
	u64 dma19, dma20, dma21, dma22;
	u8 cur_idx = (frm_num - 1) % RGBMAP_BUF;
	u8 pre_idx = (frm_num - 1 + RGBMAP_BUF - ctx->rgbmap_prebuf_idx) % RGBMAP_BUF;

	dma21 = (raw_num == ISP_PRERAW_A) ? mempool.rgbmap_le[cur_idx] : mempool_raw1.rgbmap_le[cur_idx];
	if (frm_num <= ctx->rgbmap_prebuf_idx)
		dma19 = (raw_num == ISP_PRERAW_A) ? mempool.rgbmap_le[0] : mempool_raw1.rgbmap_le[0];
	else
		dma19 = (raw_num == ISP_PRERAW_A) ? mempool.rgbmap_le[pre_idx] : mempool_raw1.rgbmap_le[pre_idx];

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA19, dma19);
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA21, dma21);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		dma22 = (raw_num == ISP_PRERAW_A) ? mempool.rgbmap_se[cur_idx] : mempool_raw1.rgbmap_se[cur_idx];
		if (frm_num <= ctx->rgbmap_prebuf_idx)
			dma20 = (raw_num == ISP_PRERAW_A) ? mempool.rgbmap_se[0] : mempool_raw1.rgbmap_se[0];
		else {
			dma20 = (raw_num == ISP_PRERAW_A) ?
				mempool.rgbmap_se[pre_idx] :
				mempool_raw1.rgbmap_se[pre_idx];
		}

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA20, dma20);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA22, dma22);
	}
}

/*
 * - postraw offline -
 *  trig postraw if there is in/out buffer for postraw
 * - postraw online -
 *  trig preraw if there is output buffer for postraw
 */
static void _post_hw_enque(struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct cvi_vip_isp_post_cfg *post_cfg;
	u8 tun_idx = 0;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
#ifdef ISP_PERF_MEASURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
#else
	struct timeval tv;
#endif
#endif

	if (vdev == NULL) {
		dprintk(VIP_ERR, "NULL pointer err\n");
		return;
	}

	if (atomic_cmpxchg(&vdev->postraw_state, ISP_POSTRAW_IDLE, ISP_POSTRAW_RUNNING) != ISP_POSTRAW_IDLE) {
		dprintk(VIP_DBG, "Postraw is running\n");
		return;
	}

	if (ctx->is_offline_postraw) {
		if (_postraw_inbuf_enq_check(vdev, &raw_num)) {
			atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
			return;
		}

		if (!ctx->is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para;

			post_para.dev = container_of(vdev, struct cvi_vip_dev, isp_vdev);
			post_para.snr_num = raw_num;
			if (cvi_sc_cfg_cb(&post_para) != 0) {
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, raw_num)) {
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			_postraw_outbuf_enq(vdev, raw_num);
		}

		if (!ctx->is_tile)
			ispblk_post_cfg_update(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor online mode
			goto YUV_POSTRAW;

		if (!ctx->is_tile) {
			_post_in_crop_update(ctx, raw_num);
			ispblk_post_in_dma_update(ctx, ISP_BLK_ID_DMA6, raw_num);
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				ispblk_post_in_dma_update(ctx, ISP_BLK_ID_DMA7, raw_num);
			}

			post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
			tun_idx  = post_cfg->tun_idx;

			dprintk(VIP_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
					raw_num, post_cfg->tun_update[tun_idx], tun_idx);
			if ((tun_idx <= 2) && (post_cfg->tun_update[tun_idx] == 1))
				_postraw_tuning_update(vdev, &post_cfg->tun_cfg[tun_idx], raw_num);
		}

		if (ctx->is_tile) {
			ctx->crop_x = ctx->isp_pipe_cfg[raw_num].crop.x;
			ctx->crop_y = ctx->isp_pipe_cfg[raw_num].crop.y;

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				ctx->crop_se_x = ctx->isp_pipe_cfg[raw_num].crop_se.x;
				ctx->crop_se_y = ctx->isp_pipe_cfg[raw_num].crop_se.y;
			}

			post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
			tun_idx  = post_cfg->tun_idx;

			dprintk(VIP_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
					raw_num, post_cfg->tun_update[tun_idx], tun_idx);
			if ((tun_idx <= 2) && (post_cfg->tun_update[tun_idx] == 1))
				_postraw_tuning_update(vdev, &post_cfg->tun_cfg[tun_idx], raw_num);

			_isp_left_tile(vdev, raw_num);
		} else if (ctx->is_dual_sensor) {
			//Update post in/out dma.
			_post_dma_update(ctx, raw_num);

			//To set apply the prev frm or not for manr/3dnr
			if (vdev->preraw_first_frm[raw_num]) {
				vdev->preraw_first_frm[raw_num] = false;
				isp_first_frm_reset(ctx, 1);
			} else {
				isp_first_frm_reset(ctx, 0);
			}
		}

		// check if the first frame after drop
		if ((ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num != 0) &&
			(vdev->preraw_frame_number[raw_num] ==
				ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num +
				ctx->isp_pipe_cfg[raw_num].drop_frm_cnt + 1)) {
			isp_first_frm_reset(ctx, 1);
		} else {
			isp_first_frm_reset(ctx, 0);
		}

		//Update for wdr detail enhancement mode
		_post_lmap_update(ctx, raw_num);
		//Update rgbmap dma addr
		_post_rgbmap_update(ctx, raw_num, vdev->preraw_frame_number[raw_num]);
		//Update post_awb/dci dma addr
		_swap_post_sts_buf(ctx, raw_num);

		ispblk_fusion_hdr_cfg(ctx, raw_num);
		ispblk_tnr_post_chg(ctx, raw_num);

YUV_POSTRAW:
		isp_post_trig(ctx);
#ifdef ISP_PERF_MEASURE
		if (++time_chk.postraw_trg_cnt < ISP_MEASURE_FRM + 1) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			ktime_get_real_ts64(&ts);

			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].time.tv_sec = ts.tv_sec;
			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].time.tv_nsec = ts.tv_nsec;
			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].cnt = time_chk.postraw_trg_cnt;
#else
			do_gettimeofday(&tv);

			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].time.tv_sec = tv.tv_sec;
			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].time.tv_usec = tv.tv_usec;
			time_chk.postraw_str[time_chk.postraw_trg_cnt - 1].cnt = time_chk.postraw_trg_cnt;
#endif
		}
#endif

		_isp_v4l2_event_queue(vdev, (raw_num == ISP_PRERAW_A) ?
					V4L2_EVENT_CVI_VIP_POST_TUN_IDX : V4L2_EVENT_CVI_VIP_POST1_TUN_IDX,
					tun_idx);

	} else { //online trigger pre when enque and postraw done
		if (_postraw_outbuf_empty(vdev, raw_num)) {
			atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
			return;
		}

		_postraw_outbuf_enq(vdev, raw_num);

		if (atomic_read(&vdev->isp_streamoff) == 0)
			isp_pre_trig(ctx, raw_num);
	}
}

static void _usr_pic_timer_handler(unsigned long data)
{
	struct cvi_isp_vdev *vdev = (struct cvi_isp_vdev *)usr_pic_timer.data;
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw || !vb2_is_streaming(&vdev->vb_q)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
		mod_timer(&usr_pic_timer.t, jiffies + vdev->usr_pic_delay);
#else
		mod_timer(&usr_pic_timer, jiffies + vdev->usr_pic_delay);
#endif
		return;
	}

	if (ctx->is_offline_postraw) {
		enum cvi_isp_raw raw_num = (ctx->cam_id == 0)
					 ? ISP_PRERAW_A : ISP_PRERAW_B;

		if (atomic_read(&vdev->preraw_state[raw_num]) == ISP_PRERAW_IDLE &&
			(atomic_read(&vdev->isp_streamoff) == 0)) {
			struct _isp_raw_num_n  *n;

			_pre_hw_enque(vdev, raw_num);

			n = kmalloc(sizeof(*n), GFP_ATOMIC);
			if (n == NULL) {
				dprintk(VIP_ERR, "pre_raw_num_q kmalloc size(%d) fail\n", sizeof(*n));
				return;
			}
			n->raw_num = raw_num;
			pre_raw_num_enq(&pre_raw_num_q, n);

			vdev->isp_pre_int_flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
			wake_up(&vdev->isp_pre_wait_q);

			_isp_v4l2_event_queue(vdev,
						V4L2_EVENT_CVI_VIP_PRE0_SOF,
						++vdev->preraw_sof_count[raw_num]);
		}
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	mod_timer(&usr_pic_timer.t, jiffies + vdev->usr_pic_delay);
#else
	mod_timer(&usr_pic_timer, jiffies + vdev->usr_pic_delay);
#endif
}

static void usr_pic_time_remove(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	if (timer_pending(&usr_pic_timer.t)) {
		del_timer_sync(&usr_pic_timer.t);
		timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	if (timer_pending(&usr_pic_timer)) {
		del_timer_sync(&usr_pic_timer);
		init_timer(&usr_pic_timer);
#endif
	}
}

static int __init usr_pic_timer_init(struct cvi_isp_vdev *vdev)
{
	usr_pic_time_remove();
	usr_pic_timer.function = _usr_pic_timer_handler;
	usr_pic_timer.data = (uintptr_t)vdev;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	usr_pic_timer.t.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer.t);
#else
	usr_pic_timer.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer);
#endif

	return 0;
}

static int _isp_raw_dump(struct cvi_isp_vdev *vdev, struct cvi_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	int ret = 0;

	if (ctx->is_offline_postraw) {
		u8 raw_num = dump[0].raw_dump.raw_num;

		atomic_set(&vdev->isp_raw_dump_en[raw_num], 1);
		ret = wait_event_interruptible_timeout(
			vdev->isp_int_wait_q[raw_num], vdev->isp_int_flag[raw_num] != 0,
			msecs_to_jiffies(dump->time_out));

		vdev->isp_int_flag[raw_num] = 0;
		if (!ret) {
			dprintk(VIP_INFO, "vi get raw timeout(%d)\n", dump[0].time_out);
			ret = -ETIME;
			return ret;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			return ret;
		}

		isp_byr[raw_num] = isp_buf_remove(&raw_dump_q[raw_num]);
		isp_buf_queue(&raw_dump_out_q[raw_num], isp_byr[raw_num]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			isp_byr_se[raw_num] = isp_buf_remove(&raw_dump_se_q[raw_num]);
			isp_buf_queue(&raw_dump_out_se_q[raw_num], isp_byr_se[raw_num]);
		}

		dump[0].raw_dump.phy_addr = isp_byr[raw_num]->addr;
		dump[0].raw_dump.size	= byr_info[0].byr_size;
		dump[0].src_w		= byr_info[0].byr_w;
		dump[0].src_h		= byr_info[0].byr_h;
		dump[0].crop_x		= byr_info[0].byr_crop_x;
		dump[0].crop_y		= byr_info[0].byr_crop_y;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			if (isp_byr_se[raw_num] == NULL) {
				dprintk(VIP_ERR, "Get raw_se dump buffer time_out(%d)\n", dump[1].time_out);
				ret = -1;
				return ret;
			}

			dump[1].raw_dump.phy_addr = isp_byr_se[raw_num]->addr;
			dump[1].raw_dump.size	= byr_info[1].byr_size;
			dump[1].src_w		= byr_info[1].byr_w;
			dump[1].src_h		= byr_info[1].byr_h;
			dump[1].crop_x		= byr_info[1].byr_crop_x;
			dump[1].crop_y		= byr_info[1].byr_crop_y;
		}
	} else {
		//ToDo change online to offline
	}

	return ret;
}

static void _isp_yuv_bypass_buf_enq(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ictx = &vdev->ctx;
	struct vb2_buffer *vb2_buf;
	struct cvi_vip_buffer *b = NULL;
	enum ISP_BLK_ID_T dmaid = (raw_num == ISP_PRERAW_A) ? ISP_BLK_ID_DMA0 : ISP_BLK_ID_DMA53;
	u64 tmp_addr = 0, i = 0;

	cvi_isp_rdy_buf_pop((struct cvi_isp_base_vdev *)vdev, raw_num);
	b = _cvi_isp_next_buf((struct cvi_isp_base_vdev *)vdev, raw_num);
	if (b == NULL)
		return;
	vb2_buf = &b->vb.vb2_buf;

	dprintk(VIP_DBG, "update yuv bypass outbuf: 0x%lx-0x%lx-0x%lx\n",
			vb2_buf->planes[0].m.userptr,
			vb2_buf->planes[1].m.userptr,
			vb2_buf->planes[2].m.userptr);

	for (i = 0; i < 3; i++) {
		tmp_addr = (u64)vb2_buf->planes[i].m.userptr;
		if (!(vb2_buf->planes[i].m.userptr & 0x100000000))
			tmp_addr += 0x100000000;
		if (vdev->preraw_frame_number[raw_num] == 0)
			ispblk_dma_yuv_bypass_config(ictx, dmaid + i, tmp_addr, raw_num);
		else
			ispblk_dma_setaddr(ictx, dmaid + i, tmp_addr);
	}
}

static void _isp_yuv_bypass_trigger(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		if (atomic_cmpxchg(&vdev->preraw_state[raw_num],
					ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
					ISP_PRERAW_RUNNING) {
			dprintk(VIP_DBG, "Preraw_%d is running\n", raw_num);
			return;
		}
		_isp_yuv_bypass_buf_enq(vdev, raw_num);
		isp_pre_trig(ctx, raw_num);
	}
}

static void cvi_isp_clk_enable(void)
{
	union vip_sys_reset reset;
	union vip_sys_isp_clk clk;

	reset.b.axi = 1;
	reset.b.isp_top = 1;
	reset.b.csi_mac0 = 1;
	reset.b.csi_mac1 = 1;
	vip_set_reset(reset);

	// enable all clk
	clk.raw = 0xffffffff;
	vip_set_isp_clk(clk);
	// disable all clk
	clk.raw = 0x0;
	vip_set_isp_clk(clk);

	// release reset clk-axi
	clk.b.clk_axi_isp_en = 1;
	vip_set_isp_clk(clk);
	reset.b.axi = 0;
	vip_set_reset(reset);
	clk.raw = 0x0;
	vip_set_isp_clk(clk);

	// release reset clk-isp
	clk.b.clk_isp_top_en = 1;
	vip_set_isp_clk(clk);
	reset.b.isp_top = 0;
	vip_set_reset(reset);
	clk.raw = 0x0;
	vip_set_isp_clk(clk);

	// release reset clk-mac
	clk.b.clk_csi_mac0_en = 1;
	clk.b.clk_csi_mac1_en = 1;
	vip_set_isp_clk(clk);
	reset.b.csi_mac0 = 0;
	reset.b.csi_mac1 = 0;
	vip_set_reset(reset);
	clk.raw = 0x0;
	vip_set_isp_clk(clk);

	// enable clk
	clk.b.clk_isp_top_en  = 1;
	clk.b.clk_axi_isp_en  = 1;
	clk.b.clk_csi_mac0_en = 1;
	if (viproc_en[1] == 1)
		clk.b.clk_csi_mac1_en = 1;
	vip_set_isp_clk(clk);
}

static void cvi_isp_clk_disable(void)
{
	union vip_sys_isp_clk clk;

	clk.raw = 0;
	vip_set_isp_clk(clk);
}

/*************************************************************************
 *	VB2_OPS definition
 *************************************************************************/
/**
 * call before VIDIOC_REQBUFS to setup buf-queue.
 * nbuffers: number of buffer requested
 * nplanes:  number of plane each buffer
 * sizes:    size of each plane(bytes)
 */
static int cvi_isp_queue_setup(struct vb2_queue *vq,
			      unsigned int *nbuffers, unsigned int *nplanes,
			      unsigned int sizes[], struct device *alloc_devs[])
{
	struct cvi_isp_vdev *vdev = vb2_get_drv_priv(vq);
	unsigned int planes = vdev->fmt->buffers;
	unsigned int p;

	dprintk(VIP_VB2, "+\n");

	for (p = 0; p < planes; ++p)
		sizes[p] = vdev->sizeimage[p];

	if (vq->num_buffers + *nbuffers < 2)
		*nbuffers = 2 - vq->num_buffers;

	*nplanes = planes;

	dprintk(VIP_INFO, "num_buffer=%d, num_plane=%d\n", *nbuffers, *nplanes);
	for (p = 0; p < *nplanes; p++)
		dprintk(VIP_INFO, "size[%u]=%u\n", p, sizes[p]);

	return 0;
}

/**
 * for VIDIOC_STREAMON, start fill data.
 */
static void cvi_isp_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct cvi_isp_vdev *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct cvi_vip_buffer *cvi_vb2 =
		container_of(vbuf, struct cvi_vip_buffer, vb);

	dprintk(VIP_VB2, "+\n");

	if (vbuf->flags == V4L2_BUF_FLAG_FRAME_ISP_0)
		vdev->chn_id = 0;
	else
		vdev->chn_id = 1;

	_cvi_isp_buf_queue((struct cvi_isp_base_vdev *)vdev, cvi_vb2);

	//Trigger postraw(offline)/preraw(online) immediately if queue had been empty before
#ifndef ISP_PERF_MEASURE
	if (vdev->ctx.isp_pipe_cfg[vdev->chn_id].is_yuv_bypass_path) {
		if (vdev->num_rdy[vdev->chn_id] == 1 && (vb2_is_streaming(&vdev->vb_q)))
			_isp_yuv_bypass_trigger(vdev, vdev->chn_id);
	} else {
		if (vdev->num_rdy[vdev->chn_id] == 1 && (vb2_is_streaming(&vdev->vb_q)) && !vdev->ctx.is_dual_sensor)
			_post_hw_enque(vdev);
	}
#endif
}

static void cvi_isp_sw_init(struct cvi_isp_vdev *vdev)
{
	u8 i = 0;

	struct sched_param param = {
		.sched_priority = MAX_USER_RT_PRIO - 1,
	};

	snr_ut_cfg_csibdg_flag = false;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	init_timer(&usr_pic_timer);
#endif

	vdev->seq_count			= 0;
	vdev->frame_number		= 0;
	vdev->postraw_proc_num		= 0;
	vdev->ctx.is_tile		= false;
	vdev->ctx.is_dpcm_on		= false;
	vdev->ctx.is_work_on_r_tile	= false;
	vdev->ctx.is_hdr_on		= false;
	vdev->ctx.is_sublvds_path	= false;
	vdev->ctx.is_yuv_early_path	= false;
	vdev->ctx.sensor_bitdepth	= 12;
	vdev->ctx.rgbmap_prebuf_idx	= 1;
	vdev->usr_pic_delay		= 0;
	vdev->usr_pic_phy_addr		= 0;
	vdev->isp_source		= CVI_ISP_SOURCE_DEV;

	memset(vdev->snr_info, 0, sizeof(struct cvi_isp_snr_info) * ISP_PRERAW_MAX);

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		vdev->preraw_frame_number[i]		= 0;
		vdev->preraw_first_frm[i]		= true;
		vdev->preraw_sof_count[i]		= 0;
		vdev->postraw_frame_number[i]		= 0;
		vdev->drop_frame_number[i]		= 0;
		vdev->isp_err_times[i]			= 0;

		memset(&vdev->ctx.isp_pipe_cfg[i], 0, sizeof(struct _isp_cfg));
		vdev->ctx.isp_pipe_cfg[i].dg_info.preraw_debug_sts	= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.isp_top_sts		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_debug_sts		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_sts		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_fifo_of_cnt	= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_w_gt_cnt		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_w_ls_cnt		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_h_gt_cnt		= 0;
		vdev->ctx.isp_pipe_cfg[i].dg_info.bdg_h_ls_cnt		= 0;

		vdev->ctx.isp_pipe_cfg[i].is_yuv_bypass_path		= false;
		vdev->ctx.isp_pipe_cfg[i].is_hdr_on			= false;
		vdev->ctx.isp_pipe_cfg[i].is_hdr_detail_en		= false;
		vdev->ctx.isp_pipe_cfg[i].rgbmap_i.w_bit		= 3;
		vdev->ctx.isp_pipe_cfg[i].rgbmap_i.h_bit		= 3;

		vdev->ctx.mmap_grid_size[i]				= 3;

		atomic_set(&vdev->preraw_state[i], ISP_PRERAW_IDLE);
		spin_lock_init(&snr_node_lock[i]);

		atomic_set(&vdev->isp_raw_dump_en[i], 0);
	}

	atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
	atomic_set(&vdev->isp_streamoff, 0);
	atomic_set(&isp_pre_exit, 0);

	isp_pre_th = kthread_create(_isp_preraw_thread, (void *)vdev, "cvitask_isp_pre");
	if (IS_ERR(isp_pre_th)) {
		pr_err("Unable to start isp_preraw_thread.\n");
	}

	sched_setscheduler(isp_pre_th, SCHED_FIFO, &param);

	isp_post_th = kthread_create(_isp_postraw_thread, (void *)vdev, "cvitask_isp_pos");
	if (IS_ERR(isp_post_th)) {
		pr_err("Unable to start isp_postraw_thread.\n");
	}

	sched_setscheduler(isp_post_th, SCHED_FIFO, &param);

	vdev->isp_pre_int_flag = 0;
	vdev->isp_post_int_flag = 0;
	init_waitqueue_head(&vdev->isp_pre_wait_q);
	init_waitqueue_head(&vdev->isp_post_wait_q);

	wake_up_process(isp_pre_th);
	wake_up_process(isp_post_th);

	spin_lock_init(&buf_lock);
	spin_lock_init(&byr_dump);
	spin_lock_init(&raw_num_lock);
}

static int cvi_isp_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cvi_isp_vdev *vdev = vb2_get_drv_priv(vq);
	struct isp_ctx *ictx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_raw raw_max = ISP_PRERAW_MAX - 1;
	int rc = 0;
	uint32_t v = 0;
	struct cvi_vi_ctx *pviProcCtx = NULL;

	pviProcCtx = (struct cvi_vi_ctx *)(vdev->shared_mem);

	tee_cv_efuse_read(0x28, 4, &v);
	if (v & 0x00100000) {
		if (!(vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path ||
			vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path)) {
			cvi_isp_clk_disable();
			return -1;
		}
	}

	dprintk(VIP_VB2, "+\n");

	if (viproc_en[1] == 1) {
		ictx->is_dual_sensor = true;
		//ictx->is_tile = false;
		ictx->is_offline_postraw = true;
		raw_max = ISP_PRERAW_MAX;
	}

	/* cif lvds reset */
	cif_lvds_reset(vdev, ISP_PRERAW_A);
	if (vdev->ctx.is_dual_sensor) //two sensor mode
		cif_lvds_reset(vdev, ISP_PRERAW_B);

	vdev->vdev.dev_debug = isp_v4l2_debug;


	for (raw_num = ISP_PRERAW_A; raw_num < raw_max; raw_num++) {
		if (pviProcCtx->devAttr[raw_num].enInputDataType == VI_DATA_TYPE_YUV_EARLY) {
			vdev->ctx.is_yuv_early_path = true;
		}

		vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en = csi_patgen_en[raw_num];

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
			vdev->usr_fmt.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
			vdev->usr_fmt.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
			vdev->usr_fmt.code = ISP_BAYER_TYPE_BG;
			vdev->usr_crop.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
			vdev->usr_crop.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
			vdev->usr_crop.left = 0;
			vdev->usr_crop.top = 0;
		}

		_vi_ctrl_init(raw_num, vdev);
		_vi_preraw_ctrl_setup(raw_num, vdev);

		if (!vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw)
			isp_pre_trig(&vdev->ctx, raw_num);
	}

	_vi_postraw_ctrl_setup(vdev);

	_vi_dma_setup(&vdev->ctx, raw_max);

	isp_streaming(&vdev->ctx, true, ISP_PRERAW_A);
	if (vdev->ctx.is_dual_sensor) //two sensor mode
		isp_streaming(&vdev->ctx, true, ISP_PRERAW_B);

	return rc;
}

/* abort streaming and wait for last buffer */
static void cvi_isp_stop_streaming(struct vb2_queue *vq)
{
	struct cvi_isp_vdev *vdev = vb2_get_drv_priv(vq);
	struct cvi_vip_buffer *cvi_vb2, *tmp;
	unsigned long flags;
	struct vb2_buffer *vb2_buf;
	struct isp_buffer *isp_b;
	struct _isp_snr_i2c_node *i2c_n;
	struct _isp_crop_node    *crop_n;
	struct _isp_raw_num_n    *raw_n;
	u8 i = 0, count = 10;

	dprintk(VIP_VB2, "+\n");

	atomic_set(&vdev->isp_streamoff, 1);

	// disable load-from-dram at streamoff
	vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw = false;
	vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_offline_preraw = false;
	usr_pic_time_remove();

	// wait to make sure hw stopped.
	while (--count > 0) {
		if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
			atomic_read(&vdev->preraw_state[ISP_PRERAW_A]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->preraw_state[ISP_PRERAW_B]) == ISP_PRERAW_IDLE)
			break;
		dprintk(VIP_DBG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 10 * 1000);
	}

	if (count == 0) {
		dprintk(VIP_ERR, "frame_done status postraw(%d) preraw_0(%d) preraw_1(%d)\n",
				atomic_read(&vdev->postraw_state),
				atomic_read(&vdev->preraw_state[ISP_PRERAW_A]),
				atomic_read(&vdev->preraw_state[ISP_PRERAW_B]));
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		/*
		 * Release all the buffers enqueued to driver
		 * when streamoff is issued
		 */
		spin_lock_irqsave(&vdev->rdy_lock, flags);
		list_for_each_entry_safe(cvi_vb2, tmp, &(vdev->rdy_queue[i]), list) {
			vb2_buf = &(cvi_vb2->vb.vb2_buf);
			if (vb2_buf->state == VB2_BUF_STATE_DONE)
				continue;
			vb2_buffer_done(vb2_buf, VB2_BUF_STATE_DONE);
		}

		vdev->num_rdy[i] = 0;
		INIT_LIST_HEAD(&vdev->rdy_queue[i]);
		spin_unlock_irqrestore(&vdev->rdy_lock, flags);

		while ((isp_b = isp_buf_remove(&pre_out_queue[i])) != NULL)
			kfree(isp_b);
		while ((isp_b = isp_buf_remove(&pre_out_se_queue[i])) != NULL)
			kfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_q[i])) != NULL)
			kfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_se_q[i])) != NULL)
			kfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_out_q[i])) != NULL)
			kfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_out_se_q[i])) != NULL)
			kfree(isp_b);

		spin_lock_irqsave(&snr_node_lock[i], flags);
		while (!list_empty(&isp_snr_i2c_queue[i].list)) {
			i2c_n = list_first_entry(&isp_snr_i2c_queue[i].list, struct _isp_snr_i2c_node, list);
			list_del_init(&i2c_n->list);
			kfree(i2c_n);
		}

		while (!list_empty(&isp_crop_queue[i].list)) {
			crop_n = list_first_entry(&isp_crop_queue[i].list, struct _isp_crop_node, list);
			list_del_init(&crop_n->list);
			kfree(crop_n);
		}
		spin_unlock_irqrestore(&snr_node_lock[i], flags);
	}

	while ((isp_b = isp_buf_remove(&post_in_queue)) != NULL)
		kfree(isp_b);
	while ((isp_b = isp_buf_remove(&post_in_se_queue)) != NULL)
		kfree(isp_b);

	spin_lock_irqsave(&raw_num_lock, flags);
	while (!list_empty(&pre_raw_num_q.list)) {
		raw_n = list_first_entry(&pre_raw_num_q.list, struct _isp_raw_num_n, list);
		list_del_init(&raw_n->list);
		kfree(raw_n);
	}
	spin_unlock_irqrestore(&raw_num_lock, flags);

	// reset at stop for next run.
	//isp_reset(&vdev->ctx);
}

const struct vb2_ops cvi_isp_qops = {
//    .buf_init           =
	.queue_setup        = cvi_isp_queue_setup,
//    .buf_finish         = cvi_isp_buf_finish,
	.buf_queue          = cvi_isp_buf_queue,
	.start_streaming    = cvi_isp_start_streaming,
	.stop_streaming     = cvi_isp_stop_streaming,
//    .wait_prepare       = vb2_ops_wait_prepare,
//    .wait_finish        = vb2_ops_wait_finish,
};

/*************************************************************************
 *	VB2-MEM-OPS definition
 *************************************************************************/
static void *isp_get_userptr(struct device *dev, unsigned long vaddr,
			     unsigned long size,
			     enum dma_data_direction dma_dir)
{
	return (void *)0xdeadbeef;
}

static void isp_put_userptr(void *buf_priv)
{
}

static const struct vb2_mem_ops cvi_isp_vb2_mem_ops = {
	.get_userptr = isp_get_userptr,
	.put_userptr = isp_put_userptr,
};

/*************************************************************************
 *	FOPS definition
 *************************************************************************/
static int cvi_isp_open(struct file *file)
{
	int rc = 0;
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	u8 i = 0;

	WARN_ON(!vdev);

	rc = v4l2_fh_open(file);
	if (rc) {
		dprintk(VIP_ERR, "v4l2_fh_open failed(%d)\n", rc);
		return rc;
	}

	if (v4l2_fh_is_singular_file(file)) {
		for (i = 0; i < ARRAY_SIZE(vdev->isp_clk); ++i) {
			if (vdev->isp_clk[i])
				clk_prepare_enable(vdev->isp_clk[i]);
			else {
				pr_err("[ERR] enable %s is fail\n", CLK_ISP_NAME[i]);
				return -EAGAIN;
			}
		}

		_isp_tuning_setup();
		cvi_isp_sw_init(vdev);
	}

	dprintk(VIP_INFO, "by %s\n", current->comm);
	return rc;
}

static int cvi_isp_release(struct file *file)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	u8 i = 0;

	WARN_ON(!vdev);

	if (v4l2_fh_is_singular_file(file)) {
		usr_pic_time_remove();

		isp_streaming(&vdev->ctx, false, ISP_PRERAW_A);
		if (vdev->ctx.is_dual_sensor)
			isp_streaming(&vdev->ctx, false, ISP_PRERAW_B);

		vdev->isp_pre_int_flag = 3;
		wake_up(&vdev->isp_pre_wait_q);
		while (atomic_read(&isp_pre_exit) == 0) {
			pr_info("wait for preraw_thread exit\n");
			usleep_range(5 * 1000, 10 * 1000);
		}
		vdev->isp_post_int_flag = 3;
		wake_up(&vdev->isp_post_wait_q);

		snr_ut_cfg_csibdg_flag = false;

		if (vb2_is_streaming(&vdev->vb_q))
			vb2_streamoff(&vdev->vb_q, vdev->vb_q.type);

		clk_set_rate(clk_get_parent(vdev->isp_clk[0]), 250000000);

		cvi_isp_clk_disable();

		for (i = 0; i < ARRAY_SIZE(vdev->isp_clk); ++i) {
			if (vdev->isp_clk[i])
				clk_disable_unprepare(vdev->isp_clk[i]);
		}

		for (i = 0; i < ISP_PRERAW_MAX; ++i) {
			viproc_en[i] = 0;
		}
	}

	vb2_fop_release(file);

	dprintk(VIP_INFO, "-\n");
	return 0;
}

static int cvi_isp_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = vdev->shared_mem;

	if (offset < 0 || (vm_size + offset) > VI_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("vi proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}


static struct v4l2_file_operations cvi_isp_fops = {
	.owner = THIS_MODULE,
	.open = cvi_isp_open,
	.release = cvi_isp_release,
	.poll = vb2_fop_poll, //.poll = cvi_isp_poll,
	.mmap = cvi_isp_mmap,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
};

/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
static int cvi_isp_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(vdev, struct cvi_vip_dev, isp_vdev);

	strlcpy(cap->driver, CVI_VIP_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CVI_VIP_DVC_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", bdev->v4l2_dev.name);

	cap->capabilities = vdev->vid_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int cvi_isp_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_isp_s_ctrl(struct file *file, void *priv,
			 struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_isp_g_ext_ctrls(struct file *file, void *priv,
			      struct v4l2_ext_controls *vc)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct isp_ctx *ctx = &vdev->ctx;
	struct v4l2_ext_control *ext_ctrls;
	int rc = -EINVAL, i = 0;

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_ISP_STS_GET:
		{
			u8 raw_num;
			unsigned long flags;

			raw_num = ext_ctrls[i].value;

			if (raw_num == 0) {
				spin_lock_irqsave(&mempool.pre_sts_lock, flags);
				mempool.pre_sts_in_use = 1;
				ext_ctrls[i].value = mempool.pre_sts_busy_idx ^ 1;
				spin_unlock_irqrestore(&mempool.pre_sts_lock, flags);
				rc = 0;
			} else if (raw_num == 1) {
				spin_lock_irqsave(&mempool_raw1.pre_sts_lock, flags);
				mempool_raw1.pre_sts_in_use = 1;
				ext_ctrls[i].value = mempool_raw1.pre_sts_busy_idx ^ 1;
				spin_unlock_irqrestore(&mempool_raw1.pre_sts_lock, flags);
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_POST_STS_GET:
		{
			u8 raw_num;
			unsigned long flags;

			raw_num = ext_ctrls[i].value;

			if (raw_num == 0) {
				spin_lock_irqsave(&mempool.post_sts_lock, flags);
				mempool.post_sts_in_use = 1;
				ext_ctrls[i].value = mempool.post_sts_busy_idx ^ 1;
				spin_unlock_irqrestore(&mempool.post_sts_lock, flags);
				rc = 0;
			} else if (raw_num == 1) {
				spin_lock_irqsave(&mempool_raw1.post_sts_lock, flags);
				mempool_raw1.post_sts_in_use = 1;
				ext_ctrls[i].value = mempool_raw1.post_sts_busy_idx ^ 1;
				spin_unlock_irqrestore(&mempool_raw1.post_sts_lock, flags);
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_STS_MEM:
		{
			struct cvi_isp_sts_mem sts_mem;
			int rval = 0;
			u8 raw_num = 0;

			rval = copy_from_user(&sts_mem, ext_ctrls[i].ptr, sizeof(struct cvi_isp_sts_mem));
			if (rval != 0)
				break;
			raw_num = sts_mem.raw_num;

			if (raw_num == 0) {
				rval = copy_to_user(ext_ctrls[i].ptr,
							mempool.sts_mem,
							sizeof(struct cvi_isp_sts_mem) * 2);
				if (rval != 0)
					break;
			} else if (raw_num == 1) {
				rval = copy_to_user(ext_ctrls[i].ptr,
							mempool_raw1.sts_mem,
							sizeof(struct cvi_isp_sts_mem) * 2);
				if (rval != 0)
					break;
			}

			if (rval)
				dprintk(VIP_ERR, "fail copying %d bytes of ISP_STS_MEM info\n", rval);
			else
				rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_SCENE_INFO:
		{
			enum ISP_SCENE_INFO info = PRE_OFF_POST_OFF_SC;

			if (ctx->is_offline_scaler) {
				info = PRE_OFF_POST_OFF_SC;
			} else {
				info = PRE_OFF_POST_ON_SC;
			}

			ext_ctrls[i].value = info;

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_BUF_SIZE:
		{
			u32 tmp_size = 0;
			enum cvi_isp_raw raw_max = ISP_PRERAW_MAX - 1;
			u8 raw_num = 0;

			tmp_size = mempool.size;
			mempool.size = 0x8000000;
			mempool.byteused = 0;

			if (viproc_en[1] == 1) {
				vdev->ctx.is_dual_sensor = true;
				//vdev->ctx.is_tile = false;
				vdev->ctx.is_offline_postraw = true;
				raw_max = ISP_PRERAW_MAX;
			}

			for (raw_num = ISP_PRERAW_A; raw_num < raw_max; raw_num++) {
				vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en = csi_patgen_en[raw_num];

				if (vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
					vdev->usr_fmt.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
					vdev->usr_fmt.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
					vdev->usr_fmt.code = ISP_BAYER_TYPE_BG;
					vdev->usr_crop.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
					vdev->usr_crop.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
					vdev->usr_crop.left = 0;
					vdev->usr_crop.top = 0;
				}

				_vi_ctrl_init(raw_num, vdev);
			}

			_vi_get_dma_buf_size(ctx, raw_max);

			ext_ctrls[i].value = mempool.byteused;

			mempool.size		= tmp_size;
			mempool.byteused	= 0;

			rc = 0;
			break;
		}

		default:
			break;
		}
	}
	return rc;
}

static int cvi_isp_s_ext_ctrls(struct file *file, void *priv,
			      struct v4l2_ext_controls *vc)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct isp_ctx *ctx = &vdev->ctx;
	struct v4l2_ext_control *ext_ctrls;
	int rc = -EINVAL, i = 0;

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_ISP_PATTERN:
			if (ext_ctrls[i].value >= CVI_VIP_PAT_MAX) {
				dprintk(VIP_ERR, "invalid isp-pattern(%d)\n",
					ext_ctrls[i].value);
				break;
			}
			rc = 0;
			// TODO: for isp patgen ctrl
			break;

		case V4L2_CID_DV_VIP_ISP_ONLINE:
			ctx->is_offline_postraw = !ext_ctrls[i].value;
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_HDR:
			ctx->is_hdr_on = ext_ctrls[i].value;
			dprintk(VIP_INFO, "HDR_ON(%d)\n", ctx->is_hdr_on);
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_HDR_DETAIL_EN:
		{
			u32 val = 0, snr_num = 0, enable = 0;

			val = ext_ctrls[i].value;
			snr_num = val & 0x3; //bit0~1: snr_num
			enable = val & 0x4; //bit2: enable/disable

			if (snr_num < ISP_PRERAW_MAX) {
				ctx->isp_pipe_cfg[snr_num].is_hdr_detail_en = enable;
				dprintk(VIP_INFO, "HDR_DETAIL_EN(%d)\n",
					ctx->isp_pipe_cfg[snr_num].is_hdr_detail_en);
				rc = 0;
			}

			break;
		}

		case V4L2_CID_DV_VIP_ISP_3DNR:
			ctx->is_3dnr_on = ext_ctrls[i].value;
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_TILE:
			ctx->is_tile = ext_ctrls[i].value;
			dprintk(VIP_INFO, "TILE_ON(%d)\n", ctx->is_tile);
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_COMPRESS_EN:
			ctx->is_dpcm_on = ext_ctrls[i].value;
			pr_info("ISP_COMPRESS_ON(%d)\n", ctx->is_dpcm_on);
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_YUV_BYPASS_PATH:
		{
			struct cvi_vip_isp_yuv_param param;
			int rval = 0;

			rval = copy_from_user(&param, ext_ctrls[i].ptr, sizeof(struct cvi_vip_isp_yuv_param));
			if ((param.raw_num < ISP_PRERAW_A) || (param.raw_num >= ISP_PRERAW_MAX)) {
				dprintk(VIP_ERR, "invalid raw_num(%d)\n", param.raw_num);
				break;
			}
			ctx->isp_pipe_cfg[param.raw_num].is_yuv_bypass_path = param.yuv_bypass_path;
			pr_info("%s YUV_BYPASS_PATH_ON(%d)\n",
				(param.raw_num == ISP_PRERAW_A) ? "ISP_PRERAW_A" : "ISP_PRERAW_B",
				ctx->isp_pipe_cfg[param.raw_num].is_yuv_bypass_path);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SUBLVDS_PATH:
			ctx->is_sublvds_path = ext_ctrls[i].value;
			pr_info("SUBLVDS_PATH_ON(%d)\n", ctx->is_sublvds_path);
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_ISP_BLC_CFG:
		{
#if 0
			struct cvi_vip_isp_blc_config *cfg;

			cfg = (struct cvi_vip_isp_blc_config *)ext_ctrls[i].ptr;
			if (cfg->inst > ISP_BLC_ID_MAX) {
				dprintk(VIP_ERR, "invalid blc-id(%d)\n",
						cfg->inst);
				break;
			}
			ispblk_blc_set_gain(ctx, cfg->inst,
				cfg->rgain, cfg->grgain,
				cfg->gbgain, cfg->bgain);
			ispblk_blc_set_offset(ctx, cfg->inst,
				cfg->roffset, cfg->groffset,
				cfg->gboffset, cfg->boffset);
			ispblk_blc_enable(ctx, cfg->inst,
				cfg->enable, cfg->bypass);
			isp_cfgs.blc_cfg[cfg->inst] = *cfg;
			rc = 0;
#endif
			break;
		}

		case V4L2_CID_DV_VIP_ISP_WBG_CFG:
		{
#if 0
			struct cvi_vip_isp_wbg_config *cfg;

			cfg = (struct cvi_vip_isp_wbg_config *)ext_ctrls[i].ptr;
			if (cfg->inst > ISP_WBG_ID_MAX) {
				dprintk(VIP_ERR, "invalid wbg-id(%d)\n",
						cfg->inst);
				break;
			}
			ispblk_wbg_config(ctx, cfg->inst,
				cfg->rgain, cfg->ggain, cfg->bgain);
			ispblk_wbg_enable(ctx, cfg->inst,
				cfg->enable, cfg->bypass);
			isp_cfgs.wbg_cfg[cfg->inst] = *cfg;
			rc = 0;
#endif
			break;
		}

		case V4L2_CID_DV_VIP_ISP_CCM_CFG:
		{
			struct cvi_vip_isp_ccm_config *cfg;
			struct isp_ccm_cfg hw_cfg;

			cfg = (struct cvi_vip_isp_ccm_config *)ext_ctrls[i].ptr;
			memcpy(&hw_cfg, cfg->coef, sizeof(hw_cfg));
			ispblk_ccm_config(ctx, cfg->enable, &hw_cfg);
			isp_cfgs.ccm_cfg = *cfg;
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_DHZ_CFG:
		{
#if 0
			struct cvi_vip_isp_dhz_config *cfg;
			struct isp_dhz_cfg hw_cfg;

			cfg = (struct cvi_vip_isp_dhz_config *)ext_ctrls[i].ptr;
			_dhz_cfg_remap(cfg, &hw_cfg);
			ispblk_dhz_config(ctx, cfg->enable, &hw_cfg);
			isp_cfgs.dhz_cfg = *cfg;
			rc = 0;
#endif
			break;
		}

		case V4L2_CID_DV_VIP_ISP_STS_PUT:
		{
			u8 raw_num = 0;
			unsigned long flags;

			raw_num = ext_ctrls[i].value;
			if (raw_num == 0) {
				spin_lock_irqsave(&mempool.pre_sts_lock, flags);
				mempool.pre_sts_in_use = 0;
				spin_unlock_irqrestore(&mempool.pre_sts_lock, flags);
				rc = 0;
			} else if (raw_num == 1) {
				spin_lock_irqsave(&mempool_raw1.pre_sts_lock, flags);
				mempool_raw1.pre_sts_in_use = 0;
				spin_unlock_irqrestore(&mempool_raw1.pre_sts_lock, flags);
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_POST_STS_PUT:
		{
			u8 raw_num = 0;
			unsigned long flags;

			raw_num = ext_ctrls[i].value;
			if (raw_num == 0) {
				spin_lock_irqsave(&mempool.post_sts_lock, flags);
				mempool.post_sts_in_use = 0;
				spin_unlock_irqrestore(&mempool.post_sts_lock, flags);
				rc = 0;
			} else if (raw_num == 1) {
				spin_lock_irqsave(&mempool_raw1.post_sts_lock, flags);
				mempool_raw1.post_sts_in_use = 0;
				spin_unlock_irqrestore(&mempool_raw1.post_sts_lock, flags);
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GE_CFG:
		{
			struct cvi_vip_isp_ge_config *cfg;

			cfg = (struct cvi_vip_isp_ge_config *)ext_ctrls[i].ptr;

			if (cfg->inst > 1) {
				dprintk(VIP_ERR, "GE inst should be 0/1\n");
				break;
			}

			if (cfg->enable > 1) {
				dprintk(VIP_ERR, "GE enable should be 0/1\n");
				break;
			}

			ispblk_ge_tun_cfg(ctx, cfg);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GAMMA_CFG:
		{
			struct cvi_vip_isp_gamma_config *cfg;

			cfg = (struct cvi_vip_isp_gamma_config *)ext_ctrls[i].ptr;

			if (cfg->enable > 1) {
				dprintk(VIP_ERR, "Gamma enable should be 0/1\n");
				break;
			}

			ispblk_gamma_tun_cfg(ctx, cfg);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_EE_CFG:
		{
			struct cvi_vip_isp_ee_config *cfg;

			cfg = (struct cvi_vip_isp_ee_config *)ext_ctrls[i].ptr;

			if (cfg->enable > 1) {
				dprintk(VIP_ERR, "ee enable should be 0/1\n");
				break;
			}

			if (cfg->dbg_mode > 11) {
				dprintk(VIP_ERR, "ee dbg_mode should be 0~11\n");
				break;
			}

			if (cfg->total_coring > 1023) {
				dprintk(VIP_ERR, "ee total_coring should be 0~1023\n");
				break;
			}

			if (cfg->total_gain > 255) {
				dprintk(VIP_ERR, "ee total_gain should be 0~255\n");
				break;
			}

			ispblk_ee_tun_cfg(ctx, cfg);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_USR_PIC_CFG:
		{
			struct cvi_isp_usr_pic_cfg *cfg;

			cfg = (struct cvi_isp_usr_pic_cfg *)ext_ctrls[i].ptr;
			if ((cfg->crop.width < 32) || (cfg->crop.width > 4096)
			    || (cfg->crop.left > cfg->crop.width) || (cfg->crop.top > cfg->crop.height)) {
				dprintk(VIP_ERR, "USR_PIC_CFG:(Invalid Param) w(%d) h(%d) x(%d) y(%d)",
					cfg->crop.width, cfg->crop.height, cfg->crop.left, cfg->crop.top);
			} else {
				vdev->usr_fmt = cfg->fmt;
				vdev->usr_crop = cfg->crop;

				vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_width	= vdev->usr_fmt.width;
				vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_height	= vdev->usr_fmt.height;
				vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].max_width		= vdev->usr_fmt.width;
				vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].max_height		= vdev->usr_fmt.height;

				ispblk_csibdg_update_size(&vdev->ctx, ISP_PRERAW_A);

				rc = 0;
			}

			break;
		}

		case V4L2_CID_DV_VIP_ISP_USR_PIC_ONOFF:
		{
			vdev->isp_source = ext_ctrls[i].value;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw =
				(vdev->isp_source == CVI_ISP_SOURCE_FE);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_PIPE_DUMP:
		{
			struct cvi_vip_isp_raw_blk *dump;

			dump = (struct cvi_vip_isp_raw_blk *)ext_ctrls[i].ptr;

			if (ctx->is_yuv_sensor) {
				//ToDo
			} else {
				rc = _isp_raw_dump(vdev, dump);
			}

			break;
		}

		case V4L2_CID_DV_VIP_ISP_PUT_PIPE_DUMP:
		{
			u32 raw_num = 0;

			raw_num = ext_ctrls[i].value;

			isp_byr[raw_num] = isp_buf_remove(&raw_dump_out_q[raw_num]);
			isp_buf_queue(&pre_out_queue[raw_num], isp_byr[raw_num]);
			isp_byr[raw_num] = NULL;

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				isp_byr_se[raw_num] = isp_buf_remove(&raw_dump_out_se_q[raw_num]);
				isp_buf_queue(&pre_out_se_queue[raw_num], isp_byr_se[raw_num]);
				isp_byr_se[raw_num] = NULL;
			}

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_USR_PIC_PUT:
		{
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA8, ext_ctrls[i].value64);
				vdev->usr_pic_phy_addr = ext_ctrls[i].value64;
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_USR_PIC_TIMING:
		{
			if (ext_ctrls[i].value > 30)
				vdev->usr_pic_delay = msecs_to_jiffies(33);
			else if (ext_ctrls[i].value > 0)
				vdev->usr_pic_delay =
				    msecs_to_jiffies(1000 / ext_ctrls[i].value);
			else
				vdev->usr_pic_delay = 0;

			if (vdev->usr_pic_delay)
				usr_pic_timer_init(vdev);
			else
				usr_pic_time_remove();

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_LSC_PHY_BUF:
		{
			struct cvi_vip_memblock *isp_mem;
			u8 raw_num = 0;
			int rval = 0;

			isp_mem = vmalloc(sizeof(struct cvi_vip_memblock) * 4);
			rval = copy_from_user(isp_mem, ext_ctrls[i].ptr, sizeof(struct cvi_vip_memblock) * 4);
			raw_num = isp_mem[0].raw_num;

			isp_mem[0].phy_addr = mempool.lsc_le[0];
			isp_mem[0].phy_addr = (raw_num == 0) ? mempool.lsc_le[0] : mempool_raw1.lsc_le[0];
			isp_mem[0].size = ispblk_dma_config(ctx, ISP_BLK_ID_DMA10, 0);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				isp_mem[1].phy_addr = mempool.lsc_se[0];
				isp_mem[1].phy_addr = (raw_num == 0) ? mempool.lsc_se[0] : mempool_raw1.lsc_se[0];
				isp_mem[1].size = ispblk_dma_config(ctx, ISP_BLK_ID_DMA11, 0);
			}

			// 2 for lsc le r_tile
			isp_mem[2].phy_addr = (raw_num == 0) ? mempool.lsc_le[1] : mempool_raw1.lsc_le[1];
			isp_mem[2].size = ispblk_dma_config(ctx, ISP_BLK_ID_DMA10, 0);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				// 3 for lsc se r_tile
				isp_mem[3].phy_addr = (raw_num == 0) ? mempool.lsc_se[1] : mempool_raw1.lsc_se[1];
				isp_mem[3].size = ispblk_dma_config(ctx, ISP_BLK_ID_DMA11, 0);
			}

			rval = copy_to_user(ext_ctrls[i].ptr, isp_mem, sizeof(struct cvi_vip_memblock) * 4);
			vfree(isp_mem);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_TUN_ADDR:
		{
			int rval = 0;

			rval = copy_to_user(ext_ctrls[i].ptr, &tuning_buf_addr, sizeof(struct isp_tuning_cfg));

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SET_PROC_CONTENT:
		{
			struct isp_proc_cfg proc_cfg;
			int rval = 0;

			rval = copy_from_user(&proc_cfg, ext_ctrls[i].ptr, sizeof(struct isp_proc_cfg));
			if ((rval != 0) || (proc_cfg.buffer_size == 0))
				break;
			isp_proc_setProcContent(proc_cfg.buffer, proc_cfg.buffer_size);

			break;
		}

		case V4L2_CID_DV_VIP_ISP_BNR_CFG:
			ISP_TUN_CFG(bnr);
			break;

		case V4L2_CID_DV_VIP_ISP_CNR_CFG:
			ISP_TUN_CFG(cnr);
			break;

		case V4L2_CID_DV_VIP_ISP_YNR_CFG:
			ISP_TUN_CFG(ynr);
			break;

		case V4L2_CID_DV_VIP_ISP_PFC_CFG:
			ISP_TUN_CFG(pfc);
			break;

		case V4L2_CID_DV_VIP_ISP_TNR_CFG:
		{
			struct cvi_vip_isp_tnr_config *cfg;

			cfg = (struct cvi_vip_isp_tnr_config *)ext_ctrls[i].ptr;

			if (!ctx->is_offline_postraw) {
				if (cfg->rgbmap_w_bit > 3 || cfg->rgbmap_h_bit > 3) {
					dprintk(VIP_ERR, "W(%d)/H(%d)_bit should be <= 3 with online mode\n",
							cfg->rgbmap_w_bit, cfg->rgbmap_h_bit);
					break;
				}
			}

			ispblk_tnr_tun_cfg(ctx, cfg, ISP_PRERAW_A);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_DCI_CFG:
			ISP_TUN_CFG(dci);
			break;

		case V4L2_CID_DV_VIP_ISP_DEMOSIAC_CFG:
			ISP_TUN_CFG(demosiac);
			break;

		case V4L2_CID_DV_VIP_ISP_3DLUT_CFG:
			ISP_TUN_CFG(3dlut);
			break;

		case V4L2_CID_DV_VIP_ISP_DPC_CFG:
#if 0
			ISP_TUN_CFG(dpc);
#endif
			break;

		case V4L2_CID_DV_VIP_ISP_LSC_CFG:
			break;

		case V4L2_CID_DV_VIP_ISP_LSCR_CFG:
			ISP_TUN_CFG(lscr);
			break;

		case V4L2_CID_DV_VIP_ISP_AE_CFG:
		{
			struct cvi_vip_isp_ae_config ae_cfg;
			int rval = 0;

			rval = copy_from_user(&ae_cfg, ext_ctrls[i].ptr, sizeof(struct cvi_vip_isp_ae_config));
			if (rval != 0)
				break;
			ispblk_ae_tun_cfg(ctx, &ae_cfg);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_AWB_CFG:
		{
			struct cvi_vip_isp_awb_config awb_cfg;
			int rval = 0;

			rval = copy_from_user(&awb_cfg, ext_ctrls[i].ptr, sizeof(struct cvi_vip_isp_awb_config));
			if (rval != 0)
				break;
			ispblk_awb_tun_cfg(ctx, &awb_cfg);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_AF_CFG:
		{
			struct cvi_vip_isp_af_config af_cfg;
			int rval = 0;

			rval = copy_from_user(&af_cfg, ext_ctrls[i].ptr, sizeof(struct cvi_vip_isp_af_config));
			ispblk_af_tun_cfg(ctx, &af_cfg);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_FSWDR_CFG:
			ISP_TUN_CFG(fswdr);
			break;

		case V4L2_CID_DV_VIP_ISP_DRC_CFG:
			break;

		case V4L2_CID_DV_VIP_ISP_MONO_CFG:
			ISP_TUN_CFG(mono);
			break;

		case V4L2_CID_DV_VIP_ISP_SET_SNR_INFO:
		{
			struct cvi_isp_snr_info snr_info;

			if (copy_from_user(&snr_info, ext_ctrls[i].ptr, sizeof(struct cvi_isp_snr_info)) != 0)
				break;
			memcpy(&vdev->snr_info[snr_info.raw_num], &snr_info, sizeof(struct cvi_isp_snr_info));

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SET_SNR_CFG_NODE:
		{
			struct cvi_isp_snr_update *snr_update;
			u8                        raw_num;
			int rval = 0;

			if (vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
				break;

			snr_update = vmalloc(sizeof(struct cvi_isp_snr_update));
			rval = copy_from_user(snr_update, ext_ctrls[i].ptr, sizeof(struct cvi_isp_snr_update));
			if (rval != 0) {
				pr_info("SNR_CFG_NODE copy from user fail.\n");
				vfree(snr_update);
				break;
			}
			raw_num = snr_update->raw_num;

			if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw ||
				vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
				rc = 0;
				break;
			}

			dprintk(VIP_DBG, "raw=%d, SNR_CFG_NODE regs_num=%d, i2c_update=%d, isp_update=%d\n",
				raw_num,
				snr_update->snr_cfg_node.snsr.regs_num,
				snr_update->snr_cfg_node.snsr.need_update,
				snr_update->snr_cfg_node.isp.need_update);

			if (atomic_read(&vdev->isp_streamoff) == 0)
				_isp_snr_cfg_enq(snr_update, raw_num);

			vfree(snr_update);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SET_RGBMAP_IDX:
		{
			if (ext_ctrls[i].value > RGBMAP_BUF - 1 || ext_ctrls[i].value < 1)
				dprintk(VIP_ERR, "RGBmap prev buf idx should be 1 to %d\n", RGBMAP_BUF - 1);
			else {
				ctx->rgbmap_prebuf_idx = ext_ctrls[i].value;
				dprintk(VIP_DBG, "RGBmap prev buf idx is %d\n", ctx->rgbmap_prebuf_idx);
			}

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_CSIBDG_CFG:
		{
			struct cvi_isp_snr_ut_cfg *indata;

			indata = (struct cvi_isp_snr_ut_cfg *)ext_ctrls[i].ptr;

			ctx->is_hdr_on = indata->hdr_on;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_width = indata->w;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_height = indata->h;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].max_width = indata->w;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].max_height = indata->h;
			ctx->img_width = indata->w;
			ctx->img_height = indata->h;
			ctx->is_offline_postraw = true;
			ctx->is_yuv_sensor	= false;

			if (indata->dual_sensor_on == true) {
				ctx->is_dual_sensor = true;
				ctx->isp_pipe_cfg[ISP_PRERAW_B].csibdg_width = indata->w;
				ctx->isp_pipe_cfg[ISP_PRERAW_B].csibdg_height = indata->h;
				ctx->isp_pipe_cfg[ISP_PRERAW_B].max_width = indata->w;
				ctx->isp_pipe_cfg[ISP_PRERAW_B].max_height = indata->h;
			}

			// preraw
			ispblk_preraw_config(ctx, ISP_PRERAW_A);
			ispblk_csibdg_config(ctx, ISP_PRERAW_A);
			if (ctx->is_dual_sensor) {
				ispblk_preraw_config(ctx, ISP_PRERAW_B);
				ispblk_csibdg_config(ctx, ISP_PRERAW_B);
			}

			ispblk_isptop_config(ctx);
			//isp_dma_setup(ctx);

			isp_pre_trig(ctx, ISP_PRERAW_A);
			isp_streaming(ctx, true, ISP_PRERAW_A);
			if (ctx->is_dual_sensor) {
				isp_pre_trig(ctx, ISP_PRERAW_B);
				isp_streaming(ctx, true, ISP_PRERAW_B);
			}

			snr_ut_cfg_csibdg_flag = true;

			indata->phy_addr[0] = mempool.bayer_le[0];
			if (ctx->is_hdr_on)
				indata->phy_addr[1] = mempool.bayer_se[0];

			if (ctx->is_dual_sensor) {
				indata->phy_addr[2] = mempool_raw1.bayer_le[0];
				if (ctx->is_hdr_on)
					indata->phy_addr[3] = mempool_raw1.bayer_se[0];
			}

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SET_ALIGN:
		{
			if (ext_ctrls[i].value >= VIP_ALIGNMENT) {
				vdev->align = ext_ctrls[i].value;
				rc = 0;
			}
			break;
		}

		case V4L2_CID_DV_VIP_ISP_GET_IP_INFO:
		{
			if (copy_to_user(ext_ctrls[i].ptr, &ip_info_list,
						sizeof(struct ip_info) * IP_INFO_ID_MAX) != 0)
				break;

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_TRIG_PRERAW:
		{
			enum cvi_isp_raw raw_num;

			raw_num = (ext_ctrls[i].value == 0) ? ISP_PRERAW_A : ISP_PRERAW_B;
			if (atomic_read(&vdev->isp_streamoff) == 0)
				isp_pre_trig(ctx, raw_num);

			dprintk(VIP_DBG, "TRIG_PRERAW(%d)\n", raw_num);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SC_ONLINE:
		{
			struct cvi_isp_sc_online sc_online;

			if (copy_from_user(&sc_online, ext_ctrls[i].ptr, sizeof(struct cvi_isp_sc_online)) != 0)
				break;

			ctx->is_offline_scaler = !sc_online.is_sc_online;
			pr_info("set is_offline_scaler:%d\n", ctx->is_offline_scaler);
			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_MMAP_GRID_SIZE:
		{
			struct cvi_isp_mmap_grid_size m_gd_sz;

			if (copy_from_user(&m_gd_sz, ext_ctrls[i].ptr, sizeof(struct cvi_isp_mmap_grid_size)) != 0)
				break;

			m_gd_sz.grid_size = ctx->mmap_grid_size[m_gd_sz.raw_num];

			if (copy_to_user(ext_ctrls[i].ptr, &m_gd_sz, sizeof(struct cvi_isp_mmap_grid_size)) != 0)
				break;

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_SET_DMA_BUF_INFO:
		{
			struct cvi_vi_dma_buf_info info;
			int rval = 0;

			rval = copy_from_user(&info, ext_ctrls[i].ptr, sizeof(struct cvi_vi_dma_buf_info));
			if ((rval != 0) || (info.size == 0) || (info.paddr == 0))
				break;

			mempool.base = info.paddr;
			mempool.size = info.size;

			pr_info("ISP dma buf paddr(0x%llx) size=0x%x\n",
					mempool.base, mempool.size);

			rc = 0;
			break;
		}

		case V4L2_CID_DV_VIP_ISP_ENABLE_DEV:
		{
			int raw_num = 0;

			raw_num = ext_ctrls[i].value;

			if (raw_num >= ISP_PRERAW_MAX)
				break;

			if (raw_num >= ARRAY_SIZE(vdev->mac_clk))
				break;

			viproc_en[raw_num] = 1;
			cvi_isp_clk_enable();

			pr_info("ENABLE_DEV(%d)\n", raw_num);

			rc = 0;
			break;
		}

		default:
			break;
		}
	}
	return rc;
}

int cvi_isp_s_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct vip_rect crop;
	int rc = -EINVAL;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
	{
		if (!vdev->ctx.is_tile) {
			// output crop
			crop.x = sel->r.left;
			crop.y = sel->r.top;
			crop.h = sel->r.height;
			crop.w = sel->r.width;
			ispblk_crop_config(&vdev->ctx, ISP_BLK_ID_CROP4, crop, ISP_PRERAW_A);
			ispblk_dma_crop_update(&vdev->ctx, ISP_BLK_ID_DMA3, crop);

			crop.x >>= 1;
			crop.y >>= 1;
			crop.w >>= 1;
			crop.h >>= 1;
			ispblk_crop_config(&vdev->ctx, ISP_BLK_ID_CROP5, crop, ISP_PRERAW_A);
			ispblk_crop_config(&vdev->ctx, ISP_BLK_ID_CROP6, crop, ISP_PRERAW_A);
			ispblk_dma_crop_update(&vdev->ctx, ISP_BLK_ID_DMA4, crop);
			ispblk_dma_crop_update(&vdev->ctx, ISP_BLK_ID_DMA5, crop);
		}
	}
	rc = 0;
	break;
	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

int cvi_isp_enum_fmt_vid_mplane(struct file *file, void  *priv,
			       struct v4l2_fmtdesc *f)
{
	dprintk(VIP_DBG, "+\n");
	return cvi_vip_enum_fmt_vid(file, priv, f);
}

int cvi_isp_g_fmt_vid_cap_mplane(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	unsigned int p;

	dprintk(VIP_DBG, "+\n");
	WARN_ON(!vdev);

	mp->width        = vdev->ctx.img_width;
	mp->height       = vdev->ctx.img_height;
	mp->field        = V4L2_FIELD_NONE;
	mp->pixelformat  = vdev->fmt->fourcc;
	mp->colorspace   = vdev->colorspace;
	mp->xfer_func    = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc    = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	mp->num_planes   = vdev->fmt->buffers;
	for (p = 0; p < mp->num_planes; p++) {
		mp->plane_fmt[p].bytesperline = vdev->bytesperline[p];
		mp->plane_fmt[p].sizeimage = vdev->sizeimage[p];
	}

	return 0;
}

int cvi_isp_try_fmt_vid_cap_mplane(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	int rc = cvi_vip_try_fmt_vid_mplane(f, vdev->align);

	if (rc < 0)
		return rc;

	if ((mp->width < vdev->ctx.img_width) ||
	    (mp->height < vdev->ctx.img_height)) {
		dprintk(VIP_WARN, "size(%d*%d) should be (%d*%d).\n",
			mp->width, mp->height,
			vdev->ctx.img_width, vdev->ctx.img_height);
		mp->width = vdev->ctx.img_width;
		mp->height = vdev->ctx.img_height;
	}
	if (mp->pixelformat != V4L2_PIX_FMT_YUV420M) {
		dprintk(VIP_WARN, "fourcc(%x) should be yuv420.\n",
			mp->pixelformat);
		mp->pixelformat = V4L2_PIX_FMT_YUV420M;
	}

	return 0;
}

int cvi_isp_s_fmt_vid_cap_mplane(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int p;
	int rc = cvi_isp_try_fmt_vid_cap_mplane(file, priv, f);

	dprintk(VIP_DBG, "+\n");
	if (rc < 0)
		return rc;

	fmt = cvi_vip_get_format(mp->pixelformat);
	vdev->fmt = fmt;
	vdev->colorspace = mp->colorspace;
	for (p = 0; p < mp->num_planes; p++) {
		vdev->bytesperline[p] = pfmt[p].bytesperline;
		vdev->sizeimage[p] = pfmt[p].sizeimage = 1024;
	}

	return rc;
}

static int cif_lvds_reset(struct cvi_isp_vdev *vdev, enum cvi_isp_raw dev_num)
{
	unsigned int devno = dev_num;

	vip_sys_cif_cb(CVI_MIPI_RESET_LVDS, (void *)&devno);

	return 0;
}

#if 0
int cvi_isp_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct isp_ctx *ctx = &vdev->ctx;
	int rc = 0;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	for (; raw_num < vdev->num_snsr; raw_num++) {
		if (!csi_patgen_en[raw_num] && !ctx->isp_pipe_cfg[raw_num].is_offline_preraw && cif_auto) {
			/* CIF stream on*/
			cif_streamon(vdev, raw_num);
			/* sensor stream on*/
			sensor_streamon(vdev, raw_num);
		}
	}

	/* cif lvds reset */
	cif_lvds_reset(vdev, ISP_PRERAW_A);
	if (vdev->ctx.is_dual_sensor) //two sensor mode
		cif_lvds_reset(vdev, ISP_PRERAW_B);

	rc = vb2_streamon(&vdev->vb_q, i);
	if (!rc) {
		isp_streaming(&vdev->ctx, true, ISP_PRERAW_A);

		if (vdev->ctx.is_dual_sensor) //two sensor mode
			isp_streaming(&vdev->ctx, true, ISP_PRERAW_B);
	}

	return rc;
}

int cvi_isp_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_isp_vdev *vdev = video_drvdata(file);
	struct isp_ctx *ctx = &vdev->ctx;
	int rc = 0, j = 0, count = 10;

	atomic_set(&vdev->isp_streamoff, 1);

	// disable load-from-dram at streamoff
	vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw = false;
	vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_offline_preraw = false;
	usr_pic_time_remove();

	// wait to make sure hw stopped.
	while (--count > 0) {
		if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
			atomic_read(&vdev->preraw_state[ISP_PRERAW_A]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->preraw_state[ISP_PRERAW_B]) == ISP_PRERAW_IDLE)
			break;
		dprintk(VIP_DBG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 10 * 1000);
	}

	if (count == 0) {
		dprintk(VIP_ERR, "frame_done status postraw(%d) preraw_0(%d) preraw_1(%d)\n",
				atomic_read(&vdev->postraw_state),
				atomic_read(&vdev->preraw_state[ISP_PRERAW_A]),
				atomic_read(&vdev->preraw_state[ISP_PRERAW_B]));
	}

	isp_streaming(&vdev->ctx, false, ISP_PRERAW_A);
	if (ctx->is_dual_sensor)
		isp_streaming(&vdev->ctx, false, ISP_PRERAW_B);

	for (j = 0; j < ISP_PRERAW_MAX; j++)
		cancel_work_sync(&isp_preraw_data_wq[j].work);
	cancel_work_sync(&isp_postraw_data_wq.work);

	rc = vb2_streamoff(&vdev->vb_q, i);
	if (!rc) {
		// reset at stop for next run.
		isp_reset(&vdev->ctx);

		/* sensor stream off */
		for (j = 0; j < vdev->num_snsr; j++) {
			if (!ctx->isp_pipe_cfg[j].is_patgen_en && cif_auto)
				sensor_streamoff(vdev, j);
		}
	}

	return rc;
}
#endif

static int isp_subscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	if ((sub->type & V4L2_EVENT_CVI_VIP_CLASS) != V4L2_EVENT_CVI_VIP_CLASS)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, CVI_ISP_NEVENTS, NULL);
}

static const struct v4l2_ioctl_ops cvi_isp_ioctl_ops = {
	.vidioc_querycap = cvi_isp_querycap,
	.vidioc_g_ctrl = cvi_isp_g_ctrl,
	.vidioc_s_ctrl = cvi_isp_s_ctrl,
	.vidioc_g_ext_ctrls = cvi_isp_g_ext_ctrls,
	.vidioc_s_ext_ctrls = cvi_isp_s_ext_ctrls,

	.vidioc_s_selection     = cvi_isp_s_selection,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.vidioc_enum_fmt_vid_cap = cvi_isp_enum_fmt_vid_mplane,
#else
	.vidioc_enum_fmt_vid_cap_mplane = cvi_isp_enum_fmt_vid_mplane,
#endif
	.vidioc_g_fmt_vid_cap_mplane    = cvi_isp_g_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane  = cvi_isp_try_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane    = cvi_isp_s_fmt_vid_cap_mplane,

//	.vidioc_s_dv_timings        = cvi_isp_s_dv_timings,
//	.vidioc_g_dv_timings        = cvi_isp_g_dv_timings,
//	.vidioc_query_dv_timings    = cvi_isp_query_dv_timings,
//	.vidioc_enum_dv_timings     = cvi_isp_enum_dv_timings,
//	.vidioc_dv_timings_cap      = cvi_isp_dv_timings_cap,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	//.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf     = vb2_ioctl_prepare_buf,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	//.vidioc_expbuf          = vb2_ioctl_expbuf,
	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,

	.vidioc_subscribe_event     = isp_subscribe_event,
	.vidioc_unsubscribe_event   = v4l2_event_unsubscribe,
};

/*************************************************************************
 *	VI DBG Proc functions
 *************************************************************************/
static inline void vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct cvi_isp_vdev *vdev = m->private;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts1, ts2;
#else
	struct timeval tv1, tv2;
#endif
	u32 frmCnt1, frmCnt2, sofCnt1, sofCnt2;
	u32 frmCnt1_b, frmCnt2_b, sofCnt1_b, sofCnt2_b;
	u64 t2 = 0, t1 = 0;

	seq_puts(m, "[VI Info]\n");
	seq_printf(m, "VIOutImgWidth\t\t:%4d\n", vdev->ctx.img_width);
	seq_printf(m, "VIOutImgHeight\t\t:%4d\n", vdev->ctx.img_height);
	seq_printf(m, "VIIspTopStatus\t\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.isp_top_sts);
	seq_puts(m, "[VI ISP_PIPE_A]\n");
	seq_printf(m, "VIInImgWidth\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_width);
	seq_printf(m, "VIInImgHeight\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_height);
	seq_printf(m, "VISofCnt\t\t:%4d\n", vdev->preraw_sof_count[ISP_PRERAW_A]);
	seq_printf(m, "VIPreCnt\t\t:%4d\n", vdev->preraw_frame_number[ISP_PRERAW_A]);
	seq_printf(m, "VIPostCnt\t\t:%4d\n", vdev->postraw_frame_number[ISP_PRERAW_A]);
	seq_printf(m, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[ISP_PRERAW_A]);

	sofCnt1 = vdev->preraw_sof_count[ISP_PRERAW_A];
	frmCnt1 = vdev->postraw_frame_number[ISP_PRERAW_A];
	sofCnt1_b = vdev->preraw_sof_count[ISP_PRERAW_B];
	frmCnt1_b = vdev->postraw_frame_number[ISP_PRERAW_B];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_real_ts64(&ts1);
	t1 = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;
#else
	do_gettimeofday(&tv1);
	t1 = tv1.tv_sec * 1000000 + tv1.tv_usec;
#endif
	msleep(980);
	do {
		sofCnt2 = vdev->preraw_sof_count[ISP_PRERAW_A];
		frmCnt2 = vdev->postraw_frame_number[ISP_PRERAW_A];
		sofCnt2_b = vdev->preraw_sof_count[ISP_PRERAW_B];
		frmCnt2_b = vdev->postraw_frame_number[ISP_PRERAW_B];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ktime_get_real_ts64(&ts2);
		t2 = ts2.tv_sec * 1000000 + ts2.tv_nsec / 1000;
#else
		do_gettimeofday(&tv2);
		t2 = tv2.tv_sec * 1000000 + tv2.tv_usec;
#endif
	} while ((t2 - t1) < 1000000);

	seq_printf(m, "VIDevFPS\t\t:%4d\n", sofCnt2 - sofCnt1);
	seq_printf(m, "VIFPS\t\t\t:%4d\n", frmCnt2 - frmCnt1);
	seq_puts(m, "[VI ISP_PIPE_A CsiBdg_Debug_Info]\n");
	seq_printf(m, "VICsiStatus\t\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_sts);
	seq_printf(m, "VICsiDebugStatus\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_debug_sts);
	seq_printf(m, "VICsiOverFlowCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_fifo_of_cnt);
	seq_printf(m, "VICsiWidthGTCnt\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_w_gt_cnt);
	seq_printf(m, "VICsiWidthLSCnt\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_w_ls_cnt);
	seq_printf(m, "VICsiHeightGTCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_h_gt_cnt);
	seq_printf(m, "VICsiHeightLSCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_h_ls_cnt);
	seq_printf(m, "VIPrerawStatus\t\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].dg_info.preraw_debug_sts);

	seq_puts(m, "[VI ISP_PIPE_A SW_State_Debug_Info]\n");
	seq_printf(m, "VIPreOutBufEmpty\t:%4d\n", isp_buf_empty(&pre_out_queue[ISP_PRERAW_A]));
	seq_printf(m, "VIPostInBufEmpty\t:%4d\n", isp_buf_empty(&post_in_queue));
	seq_printf(m, "VIPostOutBufEmpty\t:%4d\n",
				cvi_isp_rdy_buf_empty((struct cvi_isp_base_vdev *)vdev, ISP_PRERAW_A));
	seq_printf(m, "VIPreSWstatus\t\t:%4d\n", atomic_read(&vdev->preraw_state[ISP_PRERAW_A]));
	seq_printf(m, "VIPostSWstatus\t\t:%4d\n", atomic_read(&vdev->postraw_state));
	seq_printf(m, "VIPostIsRightTile\t:%4d\n", vdev->ctx.is_work_on_r_tile);

	seq_puts(m, "[VI ISP_PIPE_B]\n");
	seq_printf(m, "VIInImgWidth\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].csibdg_width);
	seq_printf(m, "VIInImgHeight\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].csibdg_height);
	seq_printf(m, "VISofCnt\t\t:%4d\n", vdev->preraw_sof_count[ISP_PRERAW_B]);
	seq_printf(m, "VIPreCnt\t\t:%4d\n", vdev->preraw_frame_number[ISP_PRERAW_B]);
	seq_printf(m, "VIPostCnt\t\t:%4d\n", vdev->postraw_frame_number[ISP_PRERAW_B]);
	seq_printf(m, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[ISP_PRERAW_B]);
	seq_printf(m, "VIDevFPS\t\t:%4d\n", sofCnt2_b - sofCnt1_b);
	seq_printf(m, "VIFPS\t\t\t:%4d\n", frmCnt2_b - frmCnt1_b);

	seq_puts(m, "[VI ISP_PIPE_B CsiBdg_Debug_Info]\n");
	seq_printf(m, "VICsiStatus\t\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_sts);
	seq_printf(m, "VICsiDebugStatus\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_debug_sts);
	seq_printf(m, "VICsiOverFlowCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_fifo_of_cnt);
	seq_printf(m, "VICsiWidthGTCnt\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_w_gt_cnt);
	seq_printf(m, "VICsiWidthLSCnt\t\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_w_ls_cnt);
	seq_printf(m, "VICsiHeightGTCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_h_gt_cnt);
	seq_printf(m, "VICsiHeightLSCnt\t:%4d\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_h_ls_cnt);
	seq_printf(m, "VIPrerawStatus\t\t:0x%x\n", vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].dg_info.preraw_debug_sts);

	seq_puts(m, "[VI ISP_PIPE_B SW_State_Debug_Info]\n");
	seq_printf(m, "VIPreOutBufEmpty\t:%4d\n", isp_buf_empty(&pre_out_queue[ISP_PRERAW_B]));
	seq_printf(m, "VIPostInBufEmpty\t:%4d\n", isp_buf_empty(&post_in_queue));
	seq_printf(m, "VIPostOutBufEmpty\t:%4d\n",
				cvi_isp_rdy_buf_empty((struct cvi_isp_base_vdev *)vdev, ISP_PRERAW_B));
	seq_printf(m, "VIPreSWstatus\t\t:%4d\n", atomic_read(&vdev->preraw_state[ISP_PRERAW_B]));
	seq_printf(m, "VIPostSWstatus\t\t:%4d\n", atomic_read(&vdev->postraw_state));
}

static int isp_dbg_proc_show(struct seq_file *m, void *v)
{
	struct cvi_isp_vdev *isp_vdev = m->private;

	if (proc_isp_mode == 0)
		vi_dbg_proc_show(m, v);
	else
		isp_register_dump(&isp_vdev->ctx, m, proc_isp_mode);

	return 0;
}

static ssize_t isp_dbg_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	if (kstrtoint(user_buf, 10, &proc_isp_mode))
		proc_isp_mode = 0;
	return count;
}

static int isp_dbg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_dbg_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vi_dbg_proc_fops = {
	.proc_open = isp_dbg_proc_open,
	.proc_read = seq_read,
	.proc_write = isp_dbg_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vi_dbg_proc_fops = {
	.owner = THIS_MODULE,
	.open = isp_dbg_proc_open,
	.read = seq_read,
	.write = isp_dbg_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/*************************************************************************
 *	General functions
 *************************************************************************/
int isp_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct video_device *vfd;
	struct cvi_isp_vdev *vdev;
	struct vb2_queue *q;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}
	vdev = &bdev->isp_vdev;
	vdev->align = VIP_ALIGNMENT;
	vdev->fmt = cvi_vip_get_format(V4L2_PIX_FMT_YUV420M);
	vdev->vid_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING;
	//vdev->dv_timings = def_dv_timings[0];
	spin_lock_init(&vdev->rdy_lock);

	vfd = &(vdev->vdev);
	snprintf(vfd->name, sizeof(vfd->name), "vip-isp");
	vfd->fops = &cvi_isp_fops;
	vfd->ioctl_ops = &cvi_isp_ioctl_ops;
	vfd->vfl_dir = VFL_DIR_RX;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	vfd->vfl_type = VFL_TYPE_VIDEO;
#else
	vfd->vfl_type = VFL_TYPE_GRABBER;
#endif
	vfd->minor = -1;
	vfd->device_caps = vdev->vid_caps;
	vfd->release = video_device_release_empty;
	vfd->v4l2_dev = &bdev->v4l2_dev;
	vfd->queue = &vdev->vb_q;

	// vb2_queue init
	q = &vdev->vb_q;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_USERPTR;
	q->buf_struct_size = sizeof(struct cvi_vip_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 0;
	q->drv_priv = vdev;
	q->dev = bdev->v4l2_dev.dev;
	q->ops = &cvi_isp_qops;
	q->mem_ops = &cvi_isp_vb2_mem_ops;
	//q->lock = &vdev->mutex;
	rc = vb2_queue_init(q);
	if (rc) {
		dprintk(VIP_ERR, "vb2_queue_init failed, ret=%d\n", rc);
		return rc;
	}
	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		INIT_LIST_HEAD(&vdev->rdy_queue[i]);
		vdev->num_rdy[i] = 0;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rc = video_register_device(vfd, VFL_TYPE_VIDEO, ISP_DEVICE_IDX);
#else
	rc = video_register_device(vfd, VFL_TYPE_GRABBER, ISP_DEVICE_IDX);
#endif
	if (rc) {
		dprintk(VIP_ERR, "Failed to register isp-device\n");
		goto err_register;
	}

	vfd->lock = &bdev->mutex;
	video_set_drvdata(vfd, vdev);

	isp_init_param(vdev);

	vdev->shared_mem = kzalloc(VI_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!vdev->shared_mem)
		return -ENOMEM;

	if (vi_proc_init(&bdev->isp_vdev, vdev->shared_mem) < 0)
		pr_err("vi proc init failed\n");

	if (proc_create_data(VI_DBG_PROC_NAME, 0644, NULL, &vi_dbg_proc_fops, &bdev->isp_vdev) == NULL)
		dprintk(VIP_ERR, "vi dbg proc creation failed\n");

	if (isp_proc_init(&bdev->isp_vdev) < 0)
		pr_err("isp proc init failed\n");

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		vdev->isp_int_flag[i]		= false;
		init_waitqueue_head(&vdev->isp_int_wait_q[i]);
	}

	for (i = 0; i < ARRAY_SIZE(CLK_ISP_NAME); ++i) {
		vdev->isp_clk[i] = devm_clk_get(&pdev->dev, CLK_ISP_NAME[i]);
		if (IS_ERR(vdev->isp_clk[i])) {
			pr_err("Cannot get clk for %s\n", CLK_ISP_NAME[i]);
			vdev->isp_clk[i] = NULL;
		}
	}

	for (i = 0; i < ARRAY_SIZE(CLK_MAC_NAME); ++i) {
		vdev->mac_clk[i] = devm_clk_get(&pdev->dev, CLK_MAC_NAME[i]);
		if (IS_ERR(vdev->mac_clk[i])) {
			pr_err("Cannot get clk for %s\n", CLK_MAC_NAME[i]);
			vdev->mac_clk[i] = NULL;
		}
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		sync_task_init(i);
	}

	dprintk(VIP_INFO, "isp registered as %s\n",
		video_device_node_name(vfd));
	return rc;

err_register:
	return rc;
}

int isp_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_isp_vdev *vdev;
	int i;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}
	vdev = &bdev->isp_vdev;

	vi_proc_remove();
	kfree(vdev->shared_mem);
	remove_proc_entry(VI_DBG_PROC_NAME, NULL);
	isp_proc_remove();

	isp_pre_th = NULL;
	isp_post_th = NULL;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		sync_task_exit(i);
	}

	return 0;
}

#ifdef ISP_PERF_MEASURE
void _isp_perf_time_dump(void)
{
	u64 time_0 = 0, time_1 = 0;
	u32 i = 0, post_str_cnt = 0, post_eof_cnt = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.sof_time[i].tv_sec * STOUS) +
				time_chk.sof_time[i].tv_nsec / 1000; //us
		time_1 = (time_chk.sof_time[i + 1].tv_sec * STOUS) +
				time_chk.sof_time[i + 1].tv_nsec / 1000; //us
		//dprintk(VIP_INFO, "SOF[%d]=%lluus, SOF[%d]=%lluus, diff=%lluus\n",
		//		i, time_0, i + 1, time_1, (time_1 - time_0));
		dprintk(VIP_INFO, "SOF_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.preraw_eof[i].tv_sec * STOUS) +
				time_chk.preraw_eof[i].tv_nsec / 1000; //us
		time_1 = (time_chk.preraw_eof[i + 1].tv_sec * STOUS) +
				time_chk.preraw_eof[i + 1].tv_nsec / 1000; //us
		//dprintk(VIP_INFO, "Preraw_eof[%d]=%lluus, Preraw_eof[%d]=%lluus, diff=%lluus\n",
		//		i, time_0, i + 1, time_1, (time_1 - time_0));
		dprintk(VIP_INFO, "Pre_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) +
				time_chk.postraw_eof[i].time.tv_nsec / 1000; //us
		time_1 = (time_chk.postraw_eof[i + 1].time.tv_sec * STOUS) +
				time_chk.postraw_eof[i + 1].time.tv_nsec / 1000; //us
		dprintk(VIP_INFO, "Post_diff=%llu\n", (time_1 - time_0));
	}
#else
	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.sof_time[i].tv_sec * STOUS) + time_chk.sof_time[i].tv_usec; //us
		time_1 = (time_chk.sof_time[i + 1].tv_sec * STOUS) + time_chk.sof_time[i + 1].tv_usec; //us
		//dprintk(VIP_INFO, "SOF[%d]=%lluus, SOF[%d]=%lluus, diff=%lluus\n",
		//		i, time_0, i + 1, time_1, (time_1 - time_0));
		dprintk(VIP_INFO, "SOF_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.preraw_eof[i].tv_sec * STOUS) + time_chk.preraw_eof[i].tv_usec; //us
		time_1 = (time_chk.preraw_eof[i + 1].tv_sec * STOUS) + time_chk.preraw_eof[i + 1].tv_usec; //us
		//dprintk(VIP_INFO, "Preraw_eof[%d]=%lluus, Preraw_eof[%d]=%lluus, diff=%lluus\n",
		//		i, time_0, i + 1, time_1, (time_1 - time_0));
		dprintk(VIP_INFO, "Pre_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us
		time_1 = (time_chk.postraw_eof[i + 1].time.tv_sec * STOUS) +
				time_chk.postraw_eof[i + 1].time.tv_usec; //us
		dprintk(VIP_INFO, "Post_diff=%llu\n", (time_1 - time_0));
	}
#endif
#if 0
	for (i = 0; i < ISP_MEASURE_FRM; i++) {
		time_0 = (time_chk.postraw_str[i].time.tv_sec * STOUS) + time_chk.postraw_str[i].time.tv_usec; //us
		time_1 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us
		post_str_cnt = time_chk.postraw_str[i].cnt;
		post_eof_cnt = time_chk.postraw_eof[i].cnt;

		dprintk(VIP_INFO, "Post_diff=%llu\n", (time_1 - time_0));
	}

	u64 time_2 = 0, time_3 = 0;

	for (i = 0; i < ISP_MEASURE_FRM; i += 2) {
		time_0 = (time_chk.postraw_str[i].time.tv_sec * STOUS) + time_chk.postraw_str[i].time.tv_usec; //us
		time_1 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us

		dprintk(VIP_INFO, "left_tile_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 1; i < ISP_MEASURE_FRM; i += 2) {
		time_0 = (time_chk.postraw_str[i].time.tv_sec * STOUS) + time_chk.postraw_str[i].time.tv_usec; //us
		time_1 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us

		dprintk(VIP_INFO, "right_tile_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.sof_time[i].tv_sec * STOUS) + time_chk.sof_time[i].tv_usec; //us
		time_1 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us

		dprintk(VIP_INFO, "SOF_Post_diff=%llu\n", time_1 - time_0);
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		time_0 = (time_chk.preraw_eof[i].tv_sec * STOUS) + time_chk.preraw_eof[i].tv_usec; //us
		time_1 = (time_chk.postraw_eof[i].time.tv_sec * STOUS) + time_chk.postraw_eof[i].time.tv_usec; //us

		dprintk(VIP_INFO, "Pre_Post_diff=%llu\n", time_1 - time_0);
	}

#endif
	time_chk.postraw_end = false;
	time_chk.preraw_end = false;
	time_chk.sof_end = false;
}
#endif

void isp_ol_sc_trig_post(struct cvi_isp_vdev *vdev)
{
	dprintk(VIP_DBG, "isp_ol_sc_trig_post\n");

	vdev->isp_post_int_flag = 4;
	wake_up(&vdev->isp_post_wait_q);
}

static int _isp_postraw_thread(void *arg)
{
	struct cvi_isp_vdev *vdev = (struct cvi_isp_vdev *)arg;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	u8 sc_ol_trig_post = false;

	while (1) {
		wait_event(vdev->isp_post_wait_q, vdev->isp_post_int_flag != 0);
		if (vdev->isp_post_int_flag == 3) {
			sc_ol_trig_post = false;
			do_exit(1);
		} else if (vdev->isp_post_int_flag == 2) {
			raw_num = ISP_PRERAW_B;
			sc_ol_trig_post = false;
		} else if (vdev->isp_post_int_flag == 1) {
			raw_num = ISP_PRERAW_A;
			sc_ol_trig_post = false;
		} else if (vdev->isp_post_int_flag == 4) {
			sc_ol_trig_post = true;
		}

		vdev->isp_post_int_flag = 0;

		//pr_info("isp_postraw_thread raw_num=%d\n", raw_num);
		ANNOTATE_CHANNEL_COLOR(1, ANNOTATE_GREEN, "VI");

		if (ctx->is_offline_postraw) {
			_post_hw_enque(vdev);
			//Sensor hold clear and enable Frame_VLD when post out buffer is not empty
			if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw && sc_ol_trig_post == false)
				_pre_hw_enque(vdev, raw_num);
		} else { //online mode
			//Enable Frame_VLD
			_post_hw_enque(vdev);
		}

		ANNOTATE_CHANNEL_END(1);
		ANNOTATE_NAME_CHANNEL(1, 1, "VI");
	}

	return 0;
}

static int _isp_preraw_thread(void *arg)
{
	struct cvi_isp_vdev *vdev = (struct cvi_isp_vdev *)arg;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	struct isp_ctx *ctx = &vdev->ctx;
	struct cvi_vip_isp_pre_cfg *pre_cfg;
	struct cvi_vip_isp_post_cfg *post_cfg;
	u8 tun_idx = 0;

	struct list_head *pos, *temp;
	struct _isp_raw_num_n  *n[10];
	unsigned long flags = 0;
	u32 enq_num = 0, i = 0;

	while (1) {
		wait_event(vdev->isp_pre_wait_q, vdev->isp_pre_int_flag != 0);
		if (vdev->isp_pre_int_flag == 3) {
			pr_info("preraw_thread exit\n");
			atomic_set(&isp_pre_exit, 1);
			do_exit(1);
		} else if (vdev->isp_pre_int_flag == 2)
			raw_num = ISP_PRERAW_B;
		else if (vdev->isp_pre_int_flag == 1)
			raw_num = ISP_PRERAW_A;

		vdev->isp_pre_int_flag = 0;

		spin_lock_irqsave(&raw_num_lock, flags);
		list_for_each_safe(pos, temp, &pre_raw_num_q.list) {
			n[enq_num] = list_entry(pos, struct _isp_raw_num_n, list);
			enq_num++;
		}
		spin_unlock_irqrestore(&raw_num_lock, flags);

		for (i = 0; i < enq_num; i++) {
			raw_num = n[i]->raw_num;

			spin_lock_irqsave(&raw_num_lock, flags);
			list_del_init(&n[i]->list);
			kfree(n[i]);
			spin_unlock_irqrestore(&raw_num_lock, flags);

			if (atomic_read(&vdev->isp_streamoff) == 0)
				_isp_snr_cfg_deq_and_fire(vdev, raw_num);

			pre_cfg = (struct cvi_vip_isp_pre_cfg *)tuning_buf_addr.pre_vir[raw_num];
			tun_idx = pre_cfg->tun_idx;
			dprintk(VIP_DBG, "Preraw_%d tuning update(%d):idx(%d)\n",
					raw_num, pre_cfg->tun_update[tun_idx], tun_idx);
			if ((tun_idx <= 2) && (pre_cfg->tun_update[tun_idx] == 1)) {
				_preraw_tuning_update(vdev, &pre_cfg->tun_cfg[tun_idx], raw_num);
				pre_cfg->tun_update[tun_idx] = 0;
			}

			if (ctx->is_offline_postraw) {
				_post_hw_enque(vdev);
			} else { //online mode
				// Postraw update
				post_cfg = (struct cvi_vip_isp_post_cfg *)tuning_buf_addr.post_vir[raw_num];
				tun_idx = post_cfg->tun_idx;
				dprintk(VIP_DBG, "Postraw tuning update(%d):idx(%d)\n",
						post_cfg->tun_update[tun_idx], tun_idx);
				if ((tun_idx <= 2) && (post_cfg->tun_update[tun_idx] == 1)) {
					_postraw_tuning_update(vdev, &post_cfg->tun_cfg[tun_idx], raw_num);
					post_cfg->tun_update[tun_idx] = 0;
				}
			}

			//_isp_v4l2_event_queue(vdev, (raw_num == ISP_PRERAW_A) ?
			//			V4L2_EVENT_CVI_VIP_PRE0_TUN_IDX : V4L2_EVENT_CVI_VIP_PRE1_TUN_IDX,
			//			tun_idx);
		}

		enq_num = 0;
	}

	return 0;
}

static void _isp_yuv_online_handler(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;

	atomic_set(&vdev->preraw_state[raw_num], ISP_PRERAW_IDLE);

	b = isp_buf_remove(&pre_out_queue[raw_num]);
	if (b == NULL) {
		dprintk(VIP_ERR, "Preraw_%d done outbuf is empty\n", raw_num);
		return;
	}

	b->crop_le.x = 0;
	b->crop_le.y = 0;
	b->map_info.w_bit = 0;
	b->map_info.h_bit = 0;
	b->timestamp = ktime_get_ns();

	isp_buf_queue(&post_in_queue, b);

	// if preraw offline, let usr_pic_timer_handler do it.
	if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
		_pre_hw_enque(vdev, raw_num);
}

static void _isp_yuv_bypass_handler(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	u32 type = (raw_num == ISP_PRERAW_A) ? V4L2_EVENT_CVI_VIP_PRE0_EOF : V4L2_EVENT_CVI_VIP_PRE1_EOF;

	atomic_set(&vdev->preraw_state[raw_num], ISP_PRERAW_IDLE);

	if (vb2_is_streaming(&vdev->vb_q)) {
		struct cvi_vip_buffer *b = NULL;

		b = cvi_isp_rdy_buf_remove((struct cvi_isp_base_vdev *)vdev, raw_num);
		if (b == NULL) {
			dprintk(VIP_ERR, "[ERR] Preraw_%d yuv bypass done remove rdy_buf\n", raw_num);
			return;
		}

		_isp_v4l2_event_queue(vdev, type, vdev->preraw_frame_number[raw_num]);

		b->vb.vb2_buf.timestamp = ktime_get_ns();
		b->vb.sequence = ++vdev->seq_count;
		b->vb.flags = (raw_num == ISP_PRERAW_A) ?
				V4L2_BUF_FLAG_FRAME_ISP_0 :
				V4L2_BUF_FLAG_FRAME_ISP_1;
		vb2_buffer_done(&b->vb.vb2_buf, VB2_BUF_STATE_DONE);

		if (cvi_isp_rdy_buf_empty((struct cvi_isp_base_vdev *)vdev, raw_num))
			dprintk(VIP_DBG, "Preraw_%d yuv bypass outbuf is empty\n", raw_num);
		else
			_isp_yuv_bypass_trigger(vdev, raw_num);
	}
}

static void _isp_sof_handler(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	dprintk(VIP_DBG, "sof_%d_cnt_%d\n", raw_num, vdev->preraw_sof_count[raw_num]);

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		struct _isp_raw_num_n  *n;

		n = kmalloc(sizeof(*n), GFP_ATOMIC);
		if (n == NULL) {
			dprintk(VIP_ERR, "pre_raw_num_q kmalloc size(%d) fail\n", sizeof(*n));
			return;
		}
		n->raw_num = raw_num;
		pre_raw_num_enq(&pre_raw_num_q, n);

		vdev->isp_pre_int_flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
		wake_up(&vdev->isp_pre_wait_q);
	}

	isp_sync_task_process(raw_num);
}

static u32 _is_drop_frame(
	struct cvi_isp_vdev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint32_t start_drop_num = ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num;
	uint32_t end_drop_num = start_drop_num + ctx->isp_pipe_cfg[raw_num].drop_frm_cnt;

	if ((start_drop_num != 0) &&
		(vdev->preraw_frame_number[raw_num] > start_drop_num) &&
		(vdev->preraw_frame_number[raw_num] <= end_drop_num)) {
		return 1;
	} else {
		return 0;
	}
}

static inline void _isp_preraw_done_handler(struct cvi_isp_vdev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 type = (raw_num == ISP_PRERAW_A) ? V4L2_EVENT_CVI_VIP_PRE0_EOF : V4L2_EVENT_CVI_VIP_PRE1_EOF;

	if (vdev->isp_err_times[raw_num])
		vdev->isp_err_times[raw_num] = 0;

	//AE HW bug. SW workaround
	ispblk_aehist_reset(ctx, ISP_BLK_ID_AEHIST0, raw_num);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ispblk_aehist_reset(ctx, ISP_BLK_ID_AEHIST1, raw_num);
		ispblk_csibdg_sw_reset(ctx, raw_num);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
		if (ctx->is_offline_scaler) { //offline mode
			dprintk(VIP_DBG, "preraw_%d_cnt_%d yuv bypass done\n",
					raw_num, vdev->preraw_frame_number[raw_num]);
			_isp_yuv_bypass_handler(vdev, raw_num);
		} else { //YUV sensor online mode
			_isp_yuv_online_handler(vdev, raw_num);
		}
		return;
	}

	if (snr_ut_cfg_csibdg_flag) {
		dprintk(VIP_DBG, "preraw_%d done Enable Frame_VLD\n", raw_num);
		isp_pre_trig(ctx, raw_num);
		return;
	}

#ifdef ISP_PERF_MEASURE
	if (time_chk.sof_end && time_chk.preraw_end) //&& time_chk.postraw_end)
		_isp_perf_time_dump();
#endif

	if (_is_drop_frame(vdev, raw_num)) {
		++vdev->drop_frame_number[raw_num];
		atomic_set(&vdev->preraw_state[raw_num], ISP_PRERAW_IDLE);

		// if preraw offline, let usr_pic_timer_handler do it.
		if ((ctx->is_offline_postraw) &&
			(!ctx->isp_pipe_cfg[raw_num].is_offline_preraw))
			_pre_hw_enque(vdev, raw_num);

		return;
	}

	_swap_pre_sts_buf(vdev, raw_num);
	//Preraw done for tuning to get stt.
	_isp_v4l2_event_queue(vdev, type, vdev->preraw_frame_number[raw_num]);

	atomic_set(&vdev->preraw_state[raw_num], ISP_PRERAW_IDLE);

	if (ctx->is_offline_postraw) {
		struct isp_buffer *b = NULL, *b_se = NULL;

		b = isp_buf_remove(&pre_out_queue[raw_num]);
		if (b == NULL) {
			dprintk(VIP_ERR, "Preraw_%d done outbuf is empty\n", raw_num);
			return;
		}

		if (atomic_cmpxchg(&vdev->isp_raw_dump_en[raw_num], 1, 0) == 0) {
			struct vip_rect _crop_le;
			struct isp_rgbmap_info m_info;

			_crop_le = ispblk_crop_get_offset(ctx, ISP_BLK_ID_CROP0, raw_num);
			b->crop_le.x = _crop_le.x;
			b->crop_le.y = _crop_le.y;

			m_info = ispblk_rgbmap_info(ctx, raw_num);
			b->map_info.w_bit = m_info.w_bit;
			b->map_info.h_bit = m_info.h_bit;
			b->timestamp = ktime_get_ns();

			isp_buf_queue(&post_in_queue, b);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				struct vip_rect _crop_se;

				b_se = isp_buf_remove(&pre_out_se_queue[raw_num]);

				_crop_se = ispblk_crop_get_offset(ctx, ISP_BLK_ID_CROP1, raw_num);
				b_se->crop_se.x = _crop_se.x;
				b_se->crop_se.y = _crop_se.y;

				isp_buf_queue(&post_in_se_queue, b_se);
			}
		} else {
			struct vip_rect crop, csi_size;
			unsigned long flags;

			spin_lock_irqsave(&byr_dump, flags);
			crop = ispblk_crop_get_offset(ctx, ISP_BLK_ID_CROP0, raw_num);
			csi_size = ispblk_csibdg_get_size(ctx, raw_num);
			spin_unlock_irqrestore(&byr_dump, flags);

			byr_info[0].byr_size	= ispblk_dma_get_size(&vdev->ctx, ISP_BLK_ID_DMA0,
									csi_size.w, csi_size.h);
			byr_info[0].byr_w	= csi_size.w;
			byr_info[0].byr_h	= csi_size.h;
			byr_info[0].byr_crop_x	= crop.x;
			byr_info[0].byr_crop_y	= crop.y;

			if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on) {
				struct vip_rect crop_se;

				spin_lock_irqsave(&byr_dump, flags);
				crop_se = ispblk_crop_get_offset(&vdev->ctx, ISP_BLK_ID_CROP1, raw_num);
				spin_unlock_irqrestore(&byr_dump, flags);

				byr_info[1].byr_size	= byr_info[0].byr_size;
				byr_info[1].byr_w	= csi_size.w;
				byr_info[1].byr_h	= csi_size.h;
				byr_info[1].byr_crop_x	= crop_se.x;
				byr_info[1].byr_crop_y	= crop_se.y;
			}

			isp_buf_queue(&raw_dump_q[raw_num], b);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				b_se = isp_buf_remove(&pre_out_se_queue[raw_num]);
				isp_buf_queue(&raw_dump_se_q[raw_num], b_se);
			}

			vdev->isp_int_flag[raw_num] = 1;
			wake_up_interruptible(&vdev->isp_int_wait_q[raw_num]);
		}

		if (ctx->is_3dnr_on && !ctx->is_dual_sensor)
			ispblk_tnr_grid_chg(ctx);
		else if (ctx->is_3dnr_on && ctx->is_dual_sensor)
			ispblk_tnr_rgbmap_chg(ctx, raw_num);

		//Change lmap w_h_bit at runtime
		ispblk_lmap_chg_size(ctx, raw_num);
		//Change rgbmap buf address
		_pre_rgbmap_update(vdev, raw_num);

		vdev->isp_post_int_flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
		wake_up(&vdev->isp_post_wait_q);

		// if preraw offline, let usr_pic_timer_handler do it.
		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
			_pre_hw_enque(vdev, raw_num);
	}
}

static void _isp_postraw_done_handler(struct cvi_isp_vdev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	u32 type = V4L2_EVENT_CVI_VIP_POST_EOF;
	u64 timestamp = 0;

	if (ctx->is_tile && (++vdev->postraw_proc_num == 2))
		vdev->postraw_proc_num = 0;

	if (vdev->postraw_proc_num == 0) { //Tile:The second postraw done, then notify MW to DQ.
		++vdev->frame_number;

		if (vb2_is_streaming(&vdev->vb_q)) {
			struct cvi_vip_buffer *b = NULL;

			if (ctx->is_offline_postraw) {
				struct isp_buffer *ispb, *ispb_se;

				ispb = isp_buf_remove(&post_in_queue);
				raw_num = ispb->raw_num;
				timestamp = ispb->timestamp;
				isp_buf_queue(&pre_out_queue[raw_num], ispb);

				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					ispb_se = isp_buf_remove(&post_in_se_queue);
					isp_buf_queue(&pre_out_se_queue[raw_num], ispb_se);
				}
			}

			if (ctx->is_offline_scaler) {
				b = cvi_isp_rdy_buf_remove((struct cvi_isp_base_vdev *)vdev, raw_num);
				if (b == NULL) {
					dprintk(VIP_ERR, "[ERR] Postraw done remove rdy_buf\n");
					return;
				}
			}

			atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);

			if (raw_num == ISP_PRERAW_B)
				type = V4L2_EVENT_CVI_VIP_POST1_EOF;

			ctx->mmap_grid_size[raw_num] = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;

			++vdev->postraw_frame_number[raw_num];

			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //ISP team no need yuv post done
				_isp_v4l2_event_queue(vdev, type, vdev->postraw_frame_number[raw_num]);

			if (ctx->is_offline_scaler) {
				if (ctx->is_offline_postraw)
					b->vb.vb2_buf.timestamp = timestamp;
				else
					b->vb.vb2_buf.timestamp = ktime_get_ns();
				b->vb.sequence = vdev->postraw_frame_number[raw_num];
				b->vb.flags = (raw_num == ISP_PRERAW_A) ?
						V4L2_BUF_FLAG_FRAME_ISP_0 :
						V4L2_BUF_FLAG_FRAME_ISP_1;
				vb2_buffer_done(&b->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		}

		if (ctx->is_offline_scaler) { //Online mode, let vpss trigger post
			vdev->isp_post_int_flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
			wake_up(&vdev->isp_post_wait_q);
		}

		//Sensor hold clear and enable Frame_VLD when post out buffer is not empty
		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
			_pre_hw_enque(vdev, raw_num);
	} else { //Trigger Right tile
		_isp_right_tile(vdev, raw_num);
		isp_post_trig(ctx);
	}
}

void isp_irq_handler(union isp_intr top_sts, union isp_csi_intr cbdg_sts,
			struct cvi_vip_dev *bdev, union isp_csi_intr cbdg_sts_b)
{
	u8 size_err = 0;
	struct cvi_isp_vdev *vdev;
	struct isp_ctx *ctx = NULL;
#ifdef ISP_PERF_MEASURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
#else
	struct timeval tv;
#endif
#endif

	if (!bdev) {
		pr_err("%s dev is null\n", __func__);
		return;
	}

	vdev = &bdev->isp_vdev;
	ctx = &vdev->ctx;

	ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.isp_top_sts = ispblk_isptop_dg_info(ctx);
	ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.preraw_debug_sts = ispblk_preraw_dg_info(ctx, ISP_PRERAW_A);
	ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_debug_sts = ispblk_csibdg_dg_info(ctx, ISP_PRERAW_A);
	ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_sts = cbdg_sts.raw;
	ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.preraw_debug_sts = ispblk_preraw_dg_info(ctx, ISP_PRERAW_B);
	ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_debug_sts = ispblk_csibdg_dg_info(ctx, ISP_PRERAW_B);
	ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_sts = cbdg_sts_b.raw;

	dprintk(VIP_ISP_IRQ, "top_sts(%#x), csi_sts(%#x), csi_sts_b(%#x)\n",
				top_sts.raw, cbdg_sts.raw, cbdg_sts_b.raw);

	if (cbdg_sts.bits.FIFO_OVERFLOW_INT) {
		//SW workaround if fifo overflow, then enable frm_vld for ae issue.
		ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_fifo_of_cnt++;

		//dprintk(VIP_ERR, "CSIBDG_A fifo overflow, enable frm_vld for ae_issue\n");
		if (atomic_read(&vdev->isp_streamoff) == 0)
			isp_pre_trig(ctx, ISP_PRERAW_A);
	}

	if (cbdg_sts.bits.FRAME_WIDTH_GT_INT) {
		struct vip_rect csi_size;

		csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_A);

		ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_w_gt_cnt++;

		pr_err("Preraw_0 Input frame width greater than setting(%d).\n", csi_size.w);
		size_err = 1;
		//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
	}

	if (cbdg_sts.bits.FRAME_WIDTH_LS_INT) {
		struct vip_rect csi_size;

		csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_A);

		ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_w_ls_cnt++;

		pr_err("Preraw_0 Input frame width less than setting(%d).\n", csi_size.w);
		size_err = 1;
		//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
	}

	if (cbdg_sts.bits.FRAME_HEIGHT_GT_INT) {
		struct vip_rect csi_size;

		csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_A);

		ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_h_gt_cnt++;

		pr_err("Preraw_0 Input frame height greater than setting(%d).\n", csi_size.h);
		size_err = 1;
		//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
	}

	if (cbdg_sts.bits.FRAME_HEIGHT_LS_INT) {
		struct vip_rect csi_size;

		csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_A);

		ctx->isp_pipe_cfg[ISP_PRERAW_A].dg_info.bdg_h_ls_cnt++;

		pr_err("Preraw_0 Input frame height less than setting(%d).\n", csi_size.h);
		size_err = 1;
		//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
	}

	if ((re_trigger) && (size_err == 1)) {
		if (vdev->isp_err_times[ISP_PRERAW_A]++ < 500) {
			isp_pre_trig(ctx, ISP_PRERAW_A);
			pr_err("isp_pre_trig retry times(%d), sof(%d), raw_num(%d).\n",
					vdev->isp_err_times[ISP_PRERAW_A],
					vdev->preraw_sof_count[ISP_PRERAW_A],
					ISP_PRERAW_A);
		} else {
			pr_err("too much errors happened.\n");
		}
		size_err = 0;
	}

	if (ctx->is_dual_sensor) {
		if (cbdg_sts_b.bits.FIFO_OVERFLOW_INT) {
			//SW workaround if fifo overflow, then enable frm_vld for ae issue.
			ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_fifo_of_cnt++;

			//dprintk(VIP_ERR, "CSIBDG_B fifo overflow, enable frm_vld for ae_issue\n");
			if (atomic_read(&vdev->isp_streamoff) == 0)
				isp_pre_trig(ctx, ISP_PRERAW_B);
		}

		if (cbdg_sts_b.bits.FRAME_WIDTH_GT_INT) {
			struct vip_rect csi_size;

			csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_B);

			ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_w_gt_cnt++;

			pr_err("Preraw_1 Input frame width greater than setting(%d).\n", csi_size.w);
			size_err = 1;
		}

		if (cbdg_sts_b.bits.FRAME_WIDTH_LS_INT) {
			struct vip_rect csi_size;

			csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_B);

			ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_w_ls_cnt++;

			pr_err("Preraw_1 Input frame width less than setting(%d).\n", csi_size.w);
			size_err = 1;
		}

		if (cbdg_sts_b.bits.FRAME_HEIGHT_GT_INT) {
			struct vip_rect csi_size;

			csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_B);

			ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_h_gt_cnt++;

			pr_err("Preraw_1 Input frame height greater than setting(%d).\n", csi_size.h);
			size_err = 1;
			//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
		}

		if (cbdg_sts_b.bits.FRAME_HEIGHT_LS_INT) {
			struct vip_rect csi_size;

			csi_size = ispblk_csibdg_get_size(ctx, ISP_PRERAW_B);

			ctx->isp_pipe_cfg[ISP_PRERAW_B].dg_info.bdg_h_ls_cnt++;

			pr_err("Preraw_1 Input frame height less than setting(%d).\n", csi_size.h);
			size_err = 1;
			//isp_register_dump(ctx, NULL, ISP_DUMP_PRERAW);
		}

		if ((re_trigger) && (size_err == 1)) {
			if (vdev->isp_err_times[ISP_PRERAW_B]++ < 500) {
				isp_pre_trig(ctx, ISP_PRERAW_B);
				pr_err("isp_pre_trig retry times(%d), sof(%d), raw_num(%d).\n",
						vdev->isp_err_times[ISP_PRERAW_B],
						vdev->preraw_sof_count[ISP_PRERAW_B],
						ISP_PRERAW_B);
			} else {
				pr_err("too much errors happened.\n");
			}
			size_err = 0;
		}
	}

	/* preraw0 sof */
	if (top_sts.bits.FRAME_START_PRE) {
		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
			++vdev->preraw_sof_count[ISP_PRERAW_A];
#ifdef ISP_PERF_MEASURE
		if (vdev->preraw_sof_count[ISP_PRERAW_A] < ISP_MEASURE_FRM + 1) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			ktime_get_real_ts64(&ts);

			time_chk.sof_time[vdev->preraw_sof_count[ISP_PRERAW_A] - 1].tv_sec = ts.tv_sec;
			time_chk.sof_time[vdev->preraw_sof_count[ISP_PRERAW_A] - 1].tv_nsec = ts.tv_nsec;
#else
			do_gettimeofday(&tv);

			time_chk.sof_time[vdev->preraw_sof_count[ISP_PRERAW_A] - 1].tv_sec = tv.tv_sec;
			time_chk.sof_time[vdev->preraw_sof_count[ISP_PRERAW_A] - 1].tv_usec = tv.tv_usec;
#endif

			if (vdev->preraw_sof_count[ISP_PRERAW_A] == ISP_MEASURE_FRM)
				time_chk.sof_end = true;
		}
#endif
		if (vdev->ctx.is_yuv_early_path) {
			dprintk(VIP_DBG, "sof_%d_cnt_%d\n", ISP_PRERAW_A, vdev->preraw_sof_count[ISP_PRERAW_A]);

			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_A, true);
		}

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) { //RGB sensor
			if (vdev->ctx.is_sublvds_path) {
				ispblk_csibdg_line_reach_config(ctx, ISP_PRERAW_A, true);
				_post_hw_enque(vdev);
			} else {
				if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
					_isp_sof_handler(vdev, ISP_PRERAW_A);
			}

			if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				_isp_v4l2_event_queue(vdev, V4L2_EVENT_CVI_VIP_PRE0_SOF,
							vdev->preraw_sof_count[ISP_PRERAW_A]);
			}
		} else if (!vdev->ctx.is_offline_scaler) { //YUV sensor online mode
			//ISP team no need sof event by yuv sensor
			_post_hw_enque(vdev);
		}
	}

	if (top_sts.bits.LINE_REACH_INT_PRE) {
		if (vdev->ctx.is_sublvds_path) {
			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_A, false);
			_isp_sof_handler(vdev, ISP_PRERAW_A);
		}

		if (vdev->ctx.is_yuv_early_path) {
			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_A, false);
			++vdev->preraw_frame_number[ISP_PRERAW_A];
			_isp_preraw_done_handler(vdev, ISP_PRERAW_A);
		}
	}

	/* preraw1 sof */
	if (top_sts.bits.FRAME_START_PRE1) { //RGB sensor
		++vdev->preraw_sof_count[ISP_PRERAW_B];

		if (vdev->ctx.is_yuv_early_path) {
			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_B, true);
		}

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
			if (vdev->ctx.is_sublvds_path) {
				ispblk_csibdg_line_reach_config(ctx, ISP_PRERAW_B, true);
				_post_hw_enque(vdev);
			} else {
				_isp_sof_handler(vdev, ISP_PRERAW_B);
			}

			_isp_v4l2_event_queue(vdev, V4L2_EVENT_CVI_VIP_PRE1_SOF, vdev->preraw_sof_count[ISP_PRERAW_B]);
		} else if (!vdev->ctx.is_offline_scaler) { //YUV sensor online mode
			//ISP team no need sof event by yuv sensor
			_post_hw_enque(vdev);
		}
	}

	if (top_sts.bits.LINE_REACH_INT_PRE1) {
		if (vdev->ctx.is_sublvds_path) {
			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_B, false);
			_isp_sof_handler(vdev, ISP_PRERAW_B);
		}

		if (vdev->ctx.is_yuv_early_path) {
			ispblk_csibdg_line_reach_config(&vdev->ctx, ISP_PRERAW_B, false);
			++vdev->preraw_frame_number[ISP_PRERAW_B];
			_isp_preraw_done_handler(vdev, ISP_PRERAW_B);
		}
	}

	/* preraw frm_done */
	if (top_sts.bits.FRAME_DONE_PRE) {
		if (!vdev->ctx.is_yuv_early_path) {
			++vdev->preraw_frame_number[ISP_PRERAW_A];
#ifdef ISP_PERF_MEASURE
			if (vdev->preraw_frame_number[ISP_PRERAW_A] < ISP_MEASURE_FRM + 1) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
				ktime_get_real_ts64(&ts);

				time_chk.preraw_eof[vdev->preraw_frame_number[ISP_PRERAW_A] - 1].tv_sec = ts.tv_sec;
				time_chk.preraw_eof[vdev->preraw_frame_number[ISP_PRERAW_A] - 1].tv_nsec = ts.tv_nsec;
#else
				do_gettimeofday(&tv);

				time_chk.preraw_eof[vdev->preraw_frame_number[ISP_PRERAW_A] - 1].tv_sec = tv.tv_sec;
				time_chk.preraw_eof[vdev->preraw_frame_number[ISP_PRERAW_A] - 1].tv_usec = tv.tv_usec;
#endif

				if (vdev->preraw_frame_number[ISP_PRERAW_A] == ISP_MEASURE_FRM)
					time_chk.preraw_end = true;
			}
#endif
			_isp_preraw_done_handler(vdev, ISP_PRERAW_A);
		}
	}

	/* preraw1 frm_done */
	if (top_sts.bits.FRAME_DONE_PRE1) {
		if (!vdev->ctx.is_yuv_early_path) {
			++vdev->preraw_frame_number[ISP_PRERAW_B];
			_isp_preraw_done_handler(vdev, ISP_PRERAW_B);
		}
	}

	/* postraw frm_done */
	if (top_sts.bits.FRAME_DONE_POST) {
#ifdef ISP_PERF_MEASURE
	if (vdev->postraw_frame_number < ISP_MEASURE_FRM + 1) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ktime_get_real_ts64(&ts);

		time_chk.postraw_eof[vdev->postraw_frame_number - 1].time.tv_sec = ts.tv_sec;
		time_chk.postraw_eof[vdev->postraw_frame_number - 1].time.tv_nsec = ts.tv_nsec;
		time_chk.postraw_eof[vdev->postraw_frame_number - 1].cnt = vdev->postraw_frame_number;
#else
		do_gettimeofday(&tv);

		time_chk.postraw_eof[vdev->postraw_frame_number - 1].time.tv_sec = tv.tv_sec;
		time_chk.postraw_eof[vdev->postraw_frame_number - 1].time.tv_usec = tv.tv_usec;
		time_chk.postraw_eof[vdev->postraw_frame_number - 1].cnt = vdev->postraw_frame_number;
#endif

		if (vdev->postraw_frame_number == ISP_MEASURE_FRM)
			time_chk.postraw_end = true;
	}
#endif
		_isp_postraw_done_handler(vdev);
	}

	/* prerarw0 shadow */
	//if (top_sts.bits.SHAW_DONE_PRE)
}
