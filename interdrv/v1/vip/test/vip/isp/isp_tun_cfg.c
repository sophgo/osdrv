#include "isp_ut.h"
#include <ut_common.h>

#include <linux/cvi_vip.h>
#include <linux/cvi_vip_isp.h>
//#include <linux/cvi_vip_tun_cfg.h>

uint8_t cnr_weight_lut[] = {
	16, 16, 8,  4,	2,  1,	1,  16,	16,  16, 8,  8, 8,  4,  4,  0,
};

uint8_t cnr_intensity_sel[] = {
	16, 11, 8, 9, 14, 9, 3, 1,
};

uint8_t bnr_intensity_sel[8] = {7, 6, 5, 4, 3, 2, 1, 0};

uint8_t bnr_weight_lut[256] = {
	31, 16, 8,  4,	2,  1,	1,  1,	1,  2,	2,  4,	4,  8,	8,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  16,	0,  16,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  32,	32,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
};

uint8_t bnr_lsc_lut[32] = {
	32, 32, 32, 32, 32, 32, 32, 33, 33, 34, 34, 35, 36, 37, 38, 39,
	40, 41, 43, 44, 45, 47, 49, 51, 53, 55, 57, 59, 61, 64, 66, 69,
};

// depth = 8
uint8_t ynr_intensity_sel[] = {
	22, 11, 8, 9, 14, 9, 31, 31
};
// depth =64
uint8_t ynr_weight_lut[] = {
	31, 16, 8,  4,	2,  1,	1,  16,	16,  16, 8,  8, 8,  4,  4,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
};
// depth = 6
uint8_t ynr_ns0_luma_th[] = {
	22, 33, 55, 99, 139, 181,
};
// depth = 5
int16_t ynr_ns0_slope[] = {
	279, 209, 81, -12, -121,
};
// depth = 6
uint8_t ynr_ns0_offset[] = {
	25, 31, 40, 47, 46, 36
};
// depth = 6
uint8_t ynr_ns1_luma_th[] = {
	22, 33, 55, 99, 139, 181,
};
// depth = 5
int16_t ynr_ns1_slope[] = {
	93, 46, 23, 0, -36,
};
// depth = 6
uint8_t ynr_ns1_offset[] = {
	7, 9, 11, 13, 13, 10
};

uint16_t manr_data[] = {
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	264, 436, 264, 59, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	255, 434, 272, 63, 262, 436, 266, 60, 260, 435, 268, 61, 258, 435, 270, 62,
	138, 374, 374, 138,
};

uint8_t cfa_ghp_lut[] = {
	0x8, 0x8, 0x8, 0x8,
	0xa, 0xa, 0xa, 0xa,
	0xc, 0xc, 0xc, 0xc,
	0xe, 0xe, 0xe, 0xe,
	0x10, 0x10, 0x10, 0x10,
	0x12, 0x12, 0x12, 0x12,
	0x14, 0x14, 0x14, 0x14,
	0x16, 0x16, 0x16, 0x16,
};

uint8_t rgbee_ac_lut[] = {
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128,
};

uint8_t rgbee_edge_lut[] = {
	16, 48, 48, 64, 64, 64, 64, 56, 48, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32,
};

uint8_t rgbee_np_lut[] = {
	16, 16, 24, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32,
};

uint8_t dci_lut[] = {
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

uint16_t C_lut[] = {
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
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
	1,   2,   2,   3,   4,	 5,   6,   7,	8,   9,   11,  12,  13,  14,  15,  16,
	17,  18,  19,  19,  20,  21,  22,  23,	24,  24,  25,  26,  27,  28,  28,  29,
	30,  30,  31,  32,  32,  33,  33,  34,	34,  35,  35,  36,  36,  37,  37,  37,
	38,  38,  38,  39,  39,  39,  40,  40,	40,  41,  41,  41,  43,  44,  46,  48,
	49,  51,  52,  54,  55,  57,  58,  60,	62,  63,  65,  66,  68,  70,  71,  73,
	75,  77,  78,  80,  82,  84,  85,  87,	89,  91,  93,  95,  96,  98,  100, 102,
	104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134,
	136, 138, 140, 142, 144, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165,
	167, 169, 170, 172, 174, 176, 178, 179, 181, 183, 185, 187, 188, 190, 192, 193,
	195, 196, 198, 200, 201, 203, 204, 206, 207, 209, 210, 211, 213, 214, 215, 217,
	218, 219, 220, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
	235, 236, 236, 237, 238, 239, 239, 240, 241, 241, 242, 243, 243, 244, 244, 245,
};

uint16_t lscr_gain_lut[] = {
	512,   521,  541,  570,  620,  688,  767,  866,
	985,  1147, 1340, 1551, 1802, 2102, 2463, 2842,
	3304, 3886, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
};

uint16_t gamma_lut[] = {
	0, 120, 220, 310, 390, 470, 540, 610, 670, 730, 786, 842, 894, 944, 994, 1050, 1096, 1138, 1178,
	1218, 1254, 1280, 1314, 1346, 1378, 1408, 1438, 1467, 1493, 1519, 1543, 1568, 1592, 1615, 1638,
	1661, 1683, 1705, 1726, 1748, 1769, 1789, 1810, 1830, 1849, 1869, 1888, 1907, 1926, 1945, 1963,
	1981, 1999, 2017, 2034, 2052, 2069, 2086, 2102, 2119, 2136, 2152, 2168, 2184, 2200, 2216, 2231,
	2247, 2262, 2277, 2292, 2307, 2322, 2337, 2351, 2366, 2380, 2394, 2408, 2422, 2436, 2450, 2464,
	2477, 2491, 2504, 2518, 2531, 2544, 2557, 2570, 2583, 2596, 2609, 2621, 2634, 2646, 2659, 2671,
	2683, 2696, 2708, 2720, 2732, 2744, 2756, 2767, 2779, 2791, 2802, 2814, 2825, 2837, 2848, 2859,
	2871, 2882, 2893, 2904, 2915, 2926, 2937, 2948, 2959, 2969, 2980, 2991, 3001, 3012, 3023, 3033,
	3043, 3054, 3064, 3074, 3085, 3095, 3105, 3115, 3125, 3135, 3145, 3155, 3165, 3175, 3185, 3194,
	3204, 3214, 3224, 3233, 3243, 3252, 3262, 3271, 3281, 3290, 3300, 3309, 3318, 3327, 3337, 3346,
	3355, 3364, 3373, 3382, 3391, 3400, 3409, 3418, 3427, 3436, 3445, 3454, 3463, 3471, 3480, 3489,
	3498, 3506, 3515, 3523, 3532, 3540, 3549, 3557, 3566, 3574, 3583, 3591, 3600, 3608, 3616, 3624,
	3633, 3641, 3649, 3657, 3665, 3674, 3682, 3690, 3698, 3706, 3714, 3722, 3730, 3738, 3746, 3754,
	3762, 3769, 3777, 3785, 3793, 3801, 3808, 3816, 3824, 3832, 3839, 3847, 3855, 3862, 3870, 3877,
	3885, 3892, 3900, 3907, 3915, 3922, 3930, 3937, 3945, 3952, 3959, 3967, 3974, 3981, 3989, 3996,
	4003, 4010, 4018, 4025, 4032, 4039, 4046, 4054, 4061, 4068, 4075, 4082, 4089, 4095,
};

uint8_t luma_coring_lut[] = {
	0,    0,    0,    0,    4,    8,   12,   16,   20,   24,   28,   32,   32,   32,   32,   32,
	32,   32,   32,   32,   32,   32,  32,   32,   32,   32,   32,   32,   32,   32,   32,   32,
	32,
};

uint8_t luma_adptctrl_lut[] = {
	0,    0,    0,    0,    4,    8,   12,   16,   20,   24,   28,   32,   32,   32,   32,   32,
	32,   32,   32,   32,   32,   32,  32,   32,   32,   32,   32,   32,   32,   32,   32,   32,
	32,
};

uint8_t delta_adptctrl_lut[] = {
	0,    0,    0,    0,    4,    8,   12,   16,   20,   24,   28,   32,   32,   32,   32,   32,
	32,   32,   32,   32,   32,   32,  32,   32,   32,   32,   32,   32,   32,   32,   32,   32,
	32,
};

uint8_t luma_shtctrl_lut[] = {
	0,    0,    0,    0,    4,    8,   12,   16,   20,   24,   28,   32,   32,   32,   32,   32,
	32,   32,   32,   32,   32,   32,  32,   32,   32,   32,   32,   32,   32,   32,   32,   32,
	32,
};

uint8_t delta_shtctrl_lut[] = {
	0,    0,    0,    0,    4,    8,   12,   16,   20,   24,   28,   32,   32,   32,   32,   32,
	32,   32,   32,   32,   32,   32,  32,   32,   32,   32,   32,   32,   32,   32,   32,   32,
	32,
};


void isp_get_stt(int fd, enum cvi_isp_raw raw_num)
{
	struct v4l2_ext_controls ecs1;
	struct v4l2_ext_control ec1;

	memset(&ecs1, 0, sizeof(ecs1));
	memset(&ec1, 0, sizeof(ec1));
	ec1.id = V4L2_CID_DV_VIP_ISP_STS_GET;
	ec1.value = (uint32_t)raw_num;
	ecs1.controls = &ec1;
	ecs1.count = 1;
	ecs1.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);
	ut_v4l2_cmd(fd, VIDIOC_G_EXT_CTRLS, (void *)&ecs1);

	ut_pr(UT_INFO, "Get pre_%d stt buffer idx(%d)\n", raw_num, ec1.value);

	memset(&ecs1, 0, sizeof(ecs1));
	memset(&ec1, 0, sizeof(ec1));
	ec1.id = V4L2_CID_DV_VIP_ISP_STS_PUT;
	ec1.value = (uint32_t)raw_num;
	ecs1.controls = &ec1;
	ecs1.count = 1;
	ecs1.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);
	ut_v4l2_cmd(fd, VIDIOC_S_EXT_CTRLS, (void *)&ecs1);

	memset(&ecs1, 0, sizeof(ecs1));
	memset(&ec1, 0, sizeof(ec1));
	ec1.id = V4L2_CID_DV_VIP_ISP_POST_STS_GET;
	ec1.value = (uint32_t)raw_num;
	ecs1.controls = &ec1;
	ecs1.count = 1;
	ecs1.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);
	ut_v4l2_cmd(fd, VIDIOC_G_EXT_CTRLS, (void *)&ecs1);

	ut_pr(UT_INFO, "Get post_%d stt buffer idx(%d)\n", raw_num, ec1.value);

	memset(&ecs1, 0, sizeof(ecs1));
	memset(&ec1, 0, sizeof(ec1));
	ec1.id = V4L2_CID_DV_VIP_ISP_POST_STS_PUT;
	ec1.value = (uint32_t)raw_num;
	ecs1.controls = &ec1;
	ecs1.count = 1;
	ecs1.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);
	ut_v4l2_cmd(fd, VIDIOC_S_EXT_CTRLS, (void *)&ecs1);
}

struct cvi_vip_isp_dpc_config dpc_cfg;
struct cvi_vip_isp_ge_config  ge_cfg[2];
struct cvi_vip_isp_lsc_config lsc_cfg;
struct cvi_vip_isp_lscr_config lscr_cfg;
struct cvi_vip_isp_cnr_config cnr_cfg;
struct cvi_vip_isp_pfc_config pfc_cfg;
struct cvi_vip_isp_tnr_config tnr_cfg;
struct cvi_vip_isp_bnr_config bnr_cfg;
struct cvi_vip_isp_gamma_config gamma_cfg;
struct cvi_vip_isp_ynr_config ynr_cfg;
struct cvi_vip_isp_demosiac_config demosiac_cfg;
struct cvi_vip_isp_dci_config dci_cfg;
struct cvi_vip_isp_ee_config ee_cfg;
struct cvi_vip_isp_3dlut_config thredlut_cfg;
struct cvi_vip_isp_ae_config ae_cfg[2];
struct cvi_vip_isp_awb_config awb_cfg;
struct cvi_vip_isp_fswdr_config fswdr_cfg;

void _isp_tun_setting_init(const struct size s)
{
	uint8_t i = 0;

	static uint8_t is_tun_setting_init;

	if (is_tun_setting_init)
		return;

	is_tun_setting_init = 1;

	for (i = 0; i < 2; i++) {
		ae_cfg[i].inst = i;
		ae_cfg[i].ae_enable = 1;
		ae_cfg[i].hist_enable = 1;
		ae_cfg[i].ae1_enable = 1;
		ae_cfg[i].hist1_enable = 1;
		ae_cfg[i].ae_numx = 31;
		ae_cfg[i].ae_numy = 24;
		ae_cfg[i].ae1_numx = 31;
		ae_cfg[i].ae1_numy = 24;
		ae_cfg[i].ae_offsetx = 0;
		ae_cfg[i].ae_offsety = 0;
		ae_cfg[i].ae1_offsetx = 0;
		ae_cfg[i].ae1_offsety = 0;
		ae_cfg[i].ae_sub_win_w = s.w / 31;
		ae_cfg[i].ae_sub_win_h = s.h / 24;
		ae_cfg[i].ae1_sub_win_w = s.w / 31;
		ae_cfg[i].ae1_sub_win_h = s.h / 24;

		ge_cfg[i].inst = i;
		ge_cfg[i].enable = 1;
		ge_cfg[i].ge_cfg.DPC_10.bits.GE_STRENGTH = 0xa;
		ge_cfg[i].ge_cfg.DPC_10.bits.GE_COMBINEWEIGHT = 0x3;
		ge_cfg[i].ge_cfg.DPC_11.bits.GE_THRE1    = 0x100;
		ge_cfg[i].ge_cfg.DPC_11.bits.GE_THRE2    = 0x100;
		ge_cfg[i].ge_cfg.DPC_12.bits.GE_THRE3    = 0x200;
		ge_cfg[i].ge_cfg.DPC_12.bits.GE_THRE4    = 0x200;
		ge_cfg[i].ge_cfg.DPC_13.bits.GE_THRE11   = 0x300;
		ge_cfg[i].ge_cfg.DPC_13.bits.GE_THRE21   = 0x300;
		ge_cfg[i].ge_cfg.DPC_14.bits.GE_THRE31   = 0x400;
		ge_cfg[i].ge_cfg.DPC_14.bits.GE_THRE41   = 0x400;
		ge_cfg[i].ge_cfg.DPC_15.bits.GE_THRE12   = 0x500;
		ge_cfg[i].ge_cfg.DPC_15.bits.GE_THRE22   = 0x500;
		ge_cfg[i].ge_cfg.DPC_16.bits.GE_THRE32   = 0x600;
		ge_cfg[i].ge_cfg.DPC_16.bits.GE_THRE42   = 0x600;
	}

	awb_cfg.inst = 0;
	awb_cfg.enable = 1;
	awb_cfg.awb_numx = 64;
	awb_cfg.awb_numy = 48;
	awb_cfg.awb_offsetx = 0;
	awb_cfg.awb_offsety = 0;
	awb_cfg.awb_sub_win_w = s.w / 64;
	awb_cfg.awb_sub_win_h = s.h / 48;
	awb_cfg.corner_avg_en = 0;
	awb_cfg.corner_size = 1;
	awb_cfg.r_lower_bound = 0x100;
	awb_cfg.r_upper_bound = 0x200;
	awb_cfg.g_lower_bound = 0x100;
	awb_cfg.g_upper_bound = 0x200;
	awb_cfg.b_lower_bound = 0x100;
	awb_cfg.b_upper_bound = 0x200;

	dpc_cfg.inst = 0;
	dpc_cfg.enable = 1;
	dpc_cfg.cluster_size = 3;
	dpc_cfg.staticbpc_enable = 0;

	dpc_cfg.dpc_cfg.DPC_3.bits.DPC_R_BRIGHT_PIXEL_RATIO = 512;
	dpc_cfg.dpc_cfg.DPC_3.bits.DPC_G_BRIGHT_PIXEL_RATIO = 512;
	dpc_cfg.dpc_cfg.DPC_4.bits.DPC_B_BRIGHT_PIXEL_RATIO = 512;
	dpc_cfg.dpc_cfg.DPC_4.bits.DPC_R_DARK_PIXEL_RATIO = 256;
	dpc_cfg.dpc_cfg.DPC_5.bits.DPC_G_DARK_PIXEL_RATIO = 256;
	dpc_cfg.dpc_cfg.DPC_5.bits.DPC_B_DARK_PIXEL_RATIO = 256;
	dpc_cfg.dpc_cfg.DPC_6.bits.DPC_R_DARK_PIXEL_MINDIFF = 128;
	dpc_cfg.dpc_cfg.DPC_6.bits.DPC_G_DARK_PIXEL_MINDIFF = 128;
	dpc_cfg.dpc_cfg.DPC_6.bits.DPC_B_DARK_PIXEL_MINDIFF = 128;
	dpc_cfg.dpc_cfg.DPC_7.bits.DPC_R_BRIGHT_PIXEL_UPBOUD_RATIO = 64;
	dpc_cfg.dpc_cfg.DPC_7.bits.DPC_G_BRIGHT_PIXEL_UPBOUD_RATIO = 64;
	dpc_cfg.dpc_cfg.DPC_7.bits.DPC_B_BRIGHT_PIXEL_UPBOUD_RATIO = 64;
	dpc_cfg.dpc_cfg.DPC_8.bits.DPC_FLAT_THRE_MIN_RB = 32;
	dpc_cfg.dpc_cfg.DPC_8.bits.DPC_FLAT_THRE_MIN_G = 32;
	dpc_cfg.dpc_cfg.DPC_9.bits.DPC_FLAT_THRE_R = 64;
	dpc_cfg.dpc_cfg.DPC_9.bits.DPC_FLAT_THRE_G = 64;
	dpc_cfg.dpc_cfg.DPC_9.bits.DPC_FLAT_THRE_B = 64;

	fswdr_cfg.enable = 1;
	fswdr_cfg.mc_enable = 1;
	fswdr_cfg.out_sel = 0;
	fswdr_cfg.mmap_mrg_mode = 1;
	fswdr_cfg.mmap_mrg_alph = 128;
	fswdr_cfg.s_max = 32768;
	fswdr_cfg.fswdr_cfg.FS_LUMA_THD.bits.FS_LUMA_THD_L = 1024;
	fswdr_cfg.fswdr_cfg.FS_LUMA_THD.bits.FS_LUMA_THD_H = 1024;
	fswdr_cfg.fswdr_cfg.FS_SE_GAIN.bits.FS_LS_GAIN = 4096;
	fswdr_cfg.fswdr_cfg.FS_WGT.bits.FS_WGT_MIN = 64;
	fswdr_cfg.fswdr_cfg.FS_WGT.bits.FS_WGT_MAX = 128;
	fswdr_cfg.fswdr_cfg.FS_WGT_SLOPE.bits.FS_WGT_SLP = 262143;

	ee_cfg.enable = 1;
	ee_cfg.dbg_mode = 0;
	ee_cfg.total_coring = 128;
	ee_cfg.total_gain = 16;

	memcpy((void *)ee_cfg.luma_coring_lut, (void *)luma_coring_lut, sizeof(luma_coring_lut));
	memcpy((void *)ee_cfg.luma_adptctrl_lut, (void *)luma_adptctrl_lut, sizeof(luma_adptctrl_lut));
	memcpy((void *)ee_cfg.delta_adptctrl_lut, (void *)delta_adptctrl_lut, sizeof(delta_adptctrl_lut));
	memcpy((void *)ee_cfg.luma_shtctrl_lut, (void *)luma_shtctrl_lut, sizeof(luma_shtctrl_lut));
	memcpy((void *)ee_cfg.delta_shtctrl_lut, (void *)delta_shtctrl_lut, sizeof(delta_shtctrl_lut));

	ee_cfg.ee_cfg.REG_04.raw = 100;
	ee_cfg.ee_cfg.REG_08.raw = 110;
	ee_cfg.ee_cfg.REG_0C.raw = 120;
	ee_cfg.ee_cfg.REG_10.raw = 130;
	ee_cfg.ee_cfg.REG_14.raw = 140;
	ee_cfg.ee_cfg.REG_18.raw = 150;
	ee_cfg.ee_cfg.REG_1C.raw = 160;
	ee_cfg.ee_cfg.REG_20.raw = 170;
	ee_cfg.ee_cfg.REG_24.raw = 180;
	ee_cfg.ee_cfg.REG_28.raw = 190;
	ee_cfg.ee_cfg.REG_2C.raw = 200;
	ee_cfg.ee_cfg.REG_30.raw = 210;
	ee_cfg.ee_cfg.REG_34.raw = 220;
	ee_cfg.ee_cfg.REG_38.raw = 230;
	ee_cfg.ee_cfg.REG_3C.raw = 240;
	ee_cfg.ee_cfg.REG_40.raw = 250;
	ee_cfg.ee_cfg.REG_58.raw = 260;
	ee_cfg.ee_cfg.REG_5C.raw = 270;
	ee_cfg.ee_cfg.REG_60.raw = 280;
	ee_cfg.ee_cfg.REG_64.raw = 290;
	ee_cfg.ee_cfg.REG_68.raw = 300;
	ee_cfg.ee_cfg.REG_6C.raw = 310;
	ee_cfg.ee_cfg.REG_70.raw = 320;
	ee_cfg.ee_cfg.REG_74.raw = 330;
	ee_cfg.ee_cfg.REG_78.raw = 340;

	gamma_cfg.enable = 1;
	memcpy(gamma_cfg.lut, gamma_lut, sizeof(gamma_lut));

	cnr_cfg.diff_gain = 4;
	cnr_cfg.diff_shift_val = 0;
	cnr_cfg.enable = 1;
	cnr_cfg.flag_neighbor_max_weight = 1;
	cnr_cfg.fusion_intensity_weight = 4;
	cnr_cfg.ratio = 220;
	cnr_cfg.strength_mode = 32;
	memcpy(cnr_cfg.weight_lut_inter, cnr_weight_lut, sizeof(cnr_weight_lut));
	memcpy(cnr_cfg.intensity_sel, cnr_intensity_sel, sizeof(cnr_intensity_sel));

	pfc_cfg.enable = 1;
	pfc_cfg.out_sel = 1;
	pfc_cfg.var_th = 256;
	pfc_cfg.correct_strength = 128;
	pfc_cfg.purple_th = 45;
	pfc_cfg.pfc_cfg.CNR_03.bits.CNR_PURPLE_CR = 177;
	pfc_cfg.pfc_cfg.CNR_03.bits.CNR_GREEN_CB = 43;
	pfc_cfg.pfc_cfg.CNR_04.bits.CNR_GREEN_CR = 21;
	pfc_cfg.pfc_cfg.CNR_04.bits.CNR_PURPLE_CB = 232;

	tnr_cfg.manr_enable = 1;
	tnr_cfg.mmap_mrg_mode = 1;
	tnr_cfg.mmap_mrg_alph = 128;
	tnr_cfg.rgbmap_w_bit = 3;
	tnr_cfg.rgbmap_h_bit = 3;

	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_00 = 3;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_01 = 4;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_02 = 3;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_10 = 4;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_11 = 4;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_12 = 4;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_20 = 3;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_21 = 4;
	tnr_cfg.tnr_cfg.REG_04.bits.MMAP_0_LPF_22 = 3;

	tnr_cfg.tnr_cfg.REG_08.bits.MMAP_0_MAP_CORING = 0;
	tnr_cfg.tnr_cfg.REG_08.bits.MMAP_0_MAP_GAIN   = 64;
	tnr_cfg.tnr_cfg.REG_08.bits.MMAP_0_MAP_THD_L  = 0;
	tnr_cfg.tnr_cfg.REG_08.bits.MMAP_0_MAP_THD_H  = 255;

	tnr_cfg.tnr_cfg.REG_0C.bits.MMAP_0_LUMA_ADAPT_LUT_IN_0 = 0;
	tnr_cfg.tnr_cfg.REG_0C.bits.MMAP_0_LUMA_ADAPT_LUT_IN_1 = 600;
	tnr_cfg.tnr_cfg.REG_10.bits.MMAP_0_LUMA_ADAPT_LUT_IN_2 = 1500;
	tnr_cfg.tnr_cfg.REG_10.bits.MMAP_0_LUMA_ADAPT_LUT_IN_3 = 2500;

	tnr_cfg.tnr_cfg.REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_0 = 63;
	tnr_cfg.tnr_cfg.REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_1 = 48;
	tnr_cfg.tnr_cfg.REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_2 = 8;
	tnr_cfg.tnr_cfg.REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_3 = 2;

	tnr_cfg.tnr_cfg.REG_18.bits.MMAP_0_LUMA_ADAPT_LUT_SLOPE_0 = 27;
	tnr_cfg.tnr_cfg.REG_18.bits.MMAP_0_LUMA_ADAPT_LUT_SLOPE_1 = 2;
	tnr_cfg.tnr_cfg.REG_1C.bits.MMAP_0_LUMA_ADAPT_LUT_SLOPE_2 = 3;
	tnr_cfg.tnr_cfg.REG_1C.bits.MMAP_0_MAP_DSHIFT_BIT = 5;

	tnr_cfg.tnr_cfg.REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_0 = 0;
	tnr_cfg.tnr_cfg.REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_1 = 45;
	tnr_cfg.tnr_cfg.REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_2 = 90;
	tnr_cfg.tnr_cfg.REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_3 = 255;

	tnr_cfg.tnr_cfg.REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_0 = 6;
	tnr_cfg.tnr_cfg.REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_1 = 10;
	tnr_cfg.tnr_cfg.REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_2 = 9;
	tnr_cfg.tnr_cfg.REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_3 = 2;

	tnr_cfg.tnr_cfg.REG_28.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_0 = 12;
	tnr_cfg.tnr_cfg.REG_28.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_1 = 4;
	tnr_cfg.tnr_cfg.REG_2C.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_2 = 4;

	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_00 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_01 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_02 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_10 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_11 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_12 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_20 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_21 = 1;
	tnr_cfg.tnr_1_cfg.REG_34.bits.MMAP_1_LPF_22 = 1;

	tnr_cfg.tnr_1_cfg.REG_38.bits.MMAP_1_MAP_CORING = 0;
	tnr_cfg.tnr_1_cfg.REG_38.bits.MMAP_1_MAP_GAIN   = 13;
	tnr_cfg.tnr_1_cfg.REG_38.bits.MMAP_1_MAP_THD_L  = 0;
	tnr_cfg.tnr_1_cfg.REG_38.bits.MMAP_1_MAP_THD_H  = 255;

	tnr_cfg.tnr_1_cfg.REG_3C.bits.MMAP_1_LUMA_ADAPT_LUT_IN_0 = 0;
	tnr_cfg.tnr_1_cfg.REG_3C.bits.MMAP_1_LUMA_ADAPT_LUT_IN_1 = 600;
	tnr_cfg.tnr_1_cfg.REG_40.bits.MMAP_1_LUMA_ADAPT_LUT_IN_2 = 1500;
	tnr_cfg.tnr_1_cfg.REG_40.bits.MMAP_1_LUMA_ADAPT_LUT_IN_3 = 2500;

	tnr_cfg.tnr_1_cfg.REG_44.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_0 = 32;
	tnr_cfg.tnr_1_cfg.REG_44.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_1 = 16;
	tnr_cfg.tnr_1_cfg.REG_44.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_2 = 16;
	tnr_cfg.tnr_1_cfg.REG_44.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_3 = 16;

	tnr_cfg.tnr_1_cfg.REG_48.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_0 = 1;
	tnr_cfg.tnr_1_cfg.REG_48.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_1 = 1;

	tnr_cfg.tnr_1_cfg.REG_4C.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_2 = 1;
	tnr_cfg.tnr_1_cfg.REG_4C.bits.MMAP_1_MAP_DSHIFT_BIT = 5;

	tnr_cfg.tnr_1_cfg.REG_50.bits.MMAP_1_IIR_PRTCT_LUT_IN_0 = 0;
	tnr_cfg.tnr_1_cfg.REG_50.bits.MMAP_1_IIR_PRTCT_LUT_IN_1 = 45;
	tnr_cfg.tnr_1_cfg.REG_50.bits.MMAP_1_IIR_PRTCT_LUT_IN_2 = 90;
	tnr_cfg.tnr_1_cfg.REG_50.bits.MMAP_1_IIR_PRTCT_LUT_IN_3 = 255;

	tnr_cfg.tnr_1_cfg.REG_54.bits.MMAP_1_IIR_PRTCT_LUT_OUT_0 = 12;
	tnr_cfg.tnr_1_cfg.REG_54.bits.MMAP_1_IIR_PRTCT_LUT_OUT_1 = 15;
	tnr_cfg.tnr_1_cfg.REG_54.bits.MMAP_1_IIR_PRTCT_LUT_OUT_2 = 12;
	tnr_cfg.tnr_1_cfg.REG_54.bits.MMAP_1_IIR_PRTCT_LUT_OUT_3 = 1;

	tnr_cfg.tnr_1_cfg.REG_58.bits.MMAP_1_IIR_PRTCT_LUT_SLOPE_0 = 1;
	tnr_cfg.tnr_1_cfg.REG_58.bits.MMAP_1_IIR_PRTCT_LUT_SLOPE_1 = 1;
	tnr_cfg.tnr_1_cfg.REG_5C.bits.MMAP_1_IIR_PRTCT_LUT_SLOPE_2 = 1;

	tnr_cfg.tnr_2_cfg.REG_70.bits.MMAP_0_GAIN_RATIO_R = 4096;
	tnr_cfg.tnr_2_cfg.REG_70.bits.MMAP_0_GAIN_RATIO_G = 4096;
	tnr_cfg.tnr_2_cfg.REG_74.bits.MMAP_0_GAIN_RATIO_B = 4096;

	tnr_cfg.tnr_2_cfg.REG_78.bits.MMAP_0_NS_SLOPE_R = 5;
	tnr_cfg.tnr_2_cfg.REG_78.bits.MMAP_0_NS_SLOPE_G = 4;
	tnr_cfg.tnr_2_cfg.REG_7C.bits.MMAP_0_NS_SLOPE_B = 6;

	tnr_cfg.tnr_2_cfg.REG_80.bits.MMAP_0_NS_LUMA_TH0_R = 16;
	tnr_cfg.tnr_2_cfg.REG_80.bits.MMAP_0_NS_LUMA_TH0_G = 16;
	tnr_cfg.tnr_2_cfg.REG_84.bits.MMAP_0_NS_LUMA_TH0_B = 16;

	tnr_cfg.tnr_2_cfg.REG_84.bits.MMAP_0_NS_LOW_OFFSET_R = 1;
	tnr_cfg.tnr_2_cfg.REG_88.bits.MMAP_0_NS_LOW_OFFSET_G = 2;
	tnr_cfg.tnr_2_cfg.REG_88.bits.MMAP_0_NS_LOW_OFFSET_B = 1;
	tnr_cfg.tnr_2_cfg.REG_8C.bits.MMAP_0_NS_HIGH_OFFSET_R = 724;
	tnr_cfg.tnr_2_cfg.REG_8C.bits.MMAP_0_NS_HIGH_OFFSET_G = 724;
	tnr_cfg.tnr_2_cfg.REG_90.bits.MMAP_0_NS_HIGH_OFFSET_B = 724;

	tnr_cfg.tnr_3_cfg.REG_A0.bits.MMAP_1_GAIN_RATIO_R = 4096;
	tnr_cfg.tnr_3_cfg.REG_A0.bits.MMAP_1_GAIN_RATIO_G = 4096;
	tnr_cfg.tnr_3_cfg.REG_A4.bits.MMAP_1_GAIN_RATIO_B = 4096;

	tnr_cfg.tnr_3_cfg.REG_A8.bits.MMAP_1_NS_SLOPE_R = 5 * 4;
	tnr_cfg.tnr_3_cfg.REG_A8.bits.MMAP_1_NS_SLOPE_G = 4 * 4;
	tnr_cfg.tnr_3_cfg.REG_AC.bits.MMAP_1_NS_SLOPE_B = 6 * 4;

	tnr_cfg.tnr_3_cfg.REG_B0.bits.MMAP_1_NS_LUMA_TH0_R = 16;
	tnr_cfg.tnr_3_cfg.REG_B0.bits.MMAP_1_NS_LUMA_TH0_G = 16;
	tnr_cfg.tnr_3_cfg.REG_B4.bits.MMAP_1_NS_LUMA_TH0_B = 16;

	tnr_cfg.tnr_3_cfg.REG_B4.bits.MMAP_1_NS_LOW_OFFSET_R = 1;
	tnr_cfg.tnr_3_cfg.REG_B8.bits.MMAP_1_NS_LOW_OFFSET_G = 1;
	tnr_cfg.tnr_3_cfg.REG_B8.bits.MMAP_1_NS_LOW_OFFSET_B = 1;

	tnr_cfg.tnr_3_cfg.REG_BC.bits.MMAP_1_NS_HIGH_OFFSET_R = 724;
	tnr_cfg.tnr_3_cfg.REG_BC.bits.MMAP_1_NS_HIGH_OFFSET_G = 724;
	tnr_cfg.tnr_3_cfg.REG_C0.bits.MMAP_1_NS_HIGH_OFFSET_B = 724;

	bnr_cfg.enable = 1;
	memcpy(bnr_cfg.intensity_sel, bnr_intensity_sel, sizeof(bnr_intensity_sel));
	memcpy(bnr_cfg.weight_lut, bnr_weight_lut, sizeof(bnr_weight_lut));
	bnr_cfg.k_smooth = 2;
	bnr_cfg.k_texture = 4;
	bnr_cfg.weight_intra_0 = 11;
	bnr_cfg.weight_intra_1 = 11;
	bnr_cfg.weight_intra_2 = 11;
	bnr_cfg.weight_norm_1 = 64;
	bnr_cfg.weight_norm_2 = 128;
	bnr_cfg.out_sel = 8;
	bnr_cfg.lsc_en = 1;
	bnr_cfg.lsc_strenth = 128;
	bnr_cfg.lsc_centerx = 2048;
	bnr_cfg.lsc_centery = 1024;
	bnr_cfg.lsc_norm = 12000;
	memcpy(bnr_cfg.lsc_gain_lut, bnr_lsc_lut, sizeof(bnr_lsc_lut));

	bnr_cfg.bnr_1_cfg.NS_GAIN.raw = 100;
	bnr_cfg.bnr_1_cfg.NS_LUMA_TH_B.raw = 22;
	bnr_cfg.bnr_1_cfg.NS_LUMA_TH_GB.raw = 22;
	bnr_cfg.bnr_1_cfg.NS_LUMA_TH_GR.raw = 22;
	bnr_cfg.bnr_1_cfg.NS_LUMA_TH_R.raw = 22;
	bnr_cfg.bnr_1_cfg.NS_OFFSET0_B.raw = 10;
	bnr_cfg.bnr_1_cfg.NS_OFFSET0_GB.raw = 10;
	bnr_cfg.bnr_1_cfg.NS_OFFSET0_GR.raw = 10;
	bnr_cfg.bnr_1_cfg.NS_OFFSET0_R.raw = 10;
	bnr_cfg.bnr_1_cfg.NS_OFFSET1_B.raw = 128;
	bnr_cfg.bnr_1_cfg.NS_OFFSET1_GB.raw = 128;
	bnr_cfg.bnr_1_cfg.NS_OFFSET1_GR.raw = 128;
	bnr_cfg.bnr_1_cfg.NS_OFFSET1_R.raw = 128;
	bnr_cfg.bnr_1_cfg.NS_SLOPE_B.raw  = 100;
	bnr_cfg.bnr_1_cfg.NS_SLOPE_GB.raw  = 100;
	bnr_cfg.bnr_1_cfg.NS_SLOPE_GR.raw  = 100;
	bnr_cfg.bnr_1_cfg.NS_SLOPE_R.raw  = 100;
	bnr_cfg.bnr_1_cfg.STRENGTH_MODE.raw = 128;

	bnr_cfg.bnr_2_cfg.LSC_RATIO.raw = 16;
	bnr_cfg.bnr_2_cfg.NEIGHBOR_MAX.raw = 1;
	bnr_cfg.bnr_2_cfg.VAR_TH.raw = 255;
	bnr_cfg.bnr_2_cfg.WEIGHT_D135.raw = 16;
	bnr_cfg.bnr_2_cfg.WEIGHT_D45.raw = 16;
	bnr_cfg.bnr_2_cfg.WEIGHT_H.raw = 16;
	bnr_cfg.bnr_2_cfg.WEIGHT_V.raw = 16;
	bnr_cfg.bnr_2_cfg.WEIGHT_SM.raw = 16;

	ynr_cfg.enable = 1;
	ynr_cfg.alpha_gain = 256;
	ynr_cfg.k_smooth = 67;
	ynr_cfg.k_texture = 88;
	ynr_cfg.out_sel = 0;
	ynr_cfg.var_th = 32;
	ynr_cfg.weight_intra_0 = 6;
	ynr_cfg.weight_intra_1 = 6;
	ynr_cfg.weight_intra_2 = 6;
	ynr_cfg.weight_norm_1 = 4;
	ynr_cfg.weight_norm_2 = 8;

	ynr_cfg.ynr_1_cfg.NS_GAIN.raw = 16;
	ynr_cfg.ynr_1_cfg.STRENGTH_MODE.raw = 0;
	ynr_cfg.ynr_1_cfg.MOTION_POS_GAIN.raw = 4;
	ynr_cfg.ynr_1_cfg.MOTION_NS_TH.raw = 4;
	ynr_cfg.ynr_1_cfg.MOTION_NEG_GAIN.raw = 2;
	ynr_cfg.ynr_2_cfg.NEIGHBOR_MAX.raw = 1;
	ynr_cfg.ynr_2_cfg.WEIGHT_D135.raw = 7;
	ynr_cfg.ynr_2_cfg.WEIGHT_D45.raw = 15;
	ynr_cfg.ynr_2_cfg.WEIGHT_H.raw = 20;
	ynr_cfg.ynr_2_cfg.WEIGHT_SM.raw = 29;
	ynr_cfg.ynr_2_cfg.WEIGHT_V.raw = 23;

	memcpy(ynr_cfg.weight_lut_h, ynr_weight_lut, sizeof(ynr_weight_lut));
	memcpy(ynr_cfg.intensity_sel, ynr_intensity_sel, sizeof(ynr_intensity_sel));
	memcpy(ynr_cfg.ns0_slope, ynr_ns0_slope, sizeof(ynr_ns0_slope));
	memcpy(ynr_cfg.ns0_luma_th, ynr_ns0_luma_th, sizeof(ynr_ns0_luma_th));
	memcpy(ynr_cfg.ns0_offset_th, ynr_ns0_offset, sizeof(ynr_ns0_offset));
	memcpy(ynr_cfg.ns1_slope, ynr_ns1_slope, sizeof(ynr_ns1_slope));
	memcpy(ynr_cfg.ns1_luma_th, ynr_ns1_luma_th, sizeof(ynr_ns1_luma_th));
	memcpy(ynr_cfg.ns1_offset_th, ynr_ns1_offset, sizeof(ynr_ns1_offset));

	demosiac_cfg.cfa_enable = 1;
	demosiac_cfg.cfa_moire_enable = 1;
	demosiac_cfg.cfa_out_sel = 1;
	memcpy(demosiac_cfg.cfa_ghp_lut, cfa_ghp_lut, sizeof(cfa_ghp_lut));
	demosiac_cfg.rgbee_enable = 1;
	demosiac_cfg.rgbee_osgain = 0x100;
	demosiac_cfg.rgbee_usgain = 0x128;
	memcpy(demosiac_cfg.rgbee_ac_lut, rgbee_ac_lut, sizeof(rgbee_ac_lut));
	memcpy(demosiac_cfg.rgbee_edge_lut, rgbee_edge_lut, sizeof(rgbee_edge_lut));
	memcpy(demosiac_cfg.rgbee_np_lut, rgbee_np_lut, sizeof(rgbee_np_lut));

	demosiac_cfg.demosiac_cfg.REG_3.bits.CFA_EDGEE_THD = 0x100;
	demosiac_cfg.demosiac_cfg.REG_3.bits.CFA_SIGE_THD = 0x100;
	demosiac_cfg.demosiac_cfg.REG_4.bits.CFA_GSIG_TOL = 0x200;
	demosiac_cfg.demosiac_cfg.REG_4.bits.CFA_RBSIG_TOL = 0x200;
	demosiac_cfg.demosiac_cfg.REG_4_1.bits.CFA_EDGE_TOL = 0x300;
	demosiac_cfg.demosiac_cfg.REG_5.bits.CFA_GHP_THD = 0x310;

	demosiac_cfg.demosiac_1_cfg.REG_10.bits.CFA_MOIRE_STRTH = 0x10;
	demosiac_cfg.demosiac_1_cfg.REG_10.bits.CFA_MOIRE_WGHT_GAIN = 0x20;
	demosiac_cfg.demosiac_1_cfg.REG_10.bits.CFA_MOIRE_NP_YSLOPE = 0x30;
	demosiac_cfg.demosiac_1_cfg.REG_11.bits.CFA_MOIRE_NP_YMIN = 0x40;
	demosiac_cfg.demosiac_1_cfg.REG_11.bits.CFA_MOIRE_NP_LOW = 0x50;
	demosiac_cfg.demosiac_1_cfg.REG_11.bits.CFA_MOIRE_NP_HIGH = 0x60;
	demosiac_cfg.demosiac_1_cfg.REG_12.bits.CFA_MOIRE_DIFFTHD_MIN = 0x70;
	demosiac_cfg.demosiac_1_cfg.REG_12.bits.CFA_MOIRE_DIFFTHD_SLOPE = 0x80;
	demosiac_cfg.demosiac_1_cfg.REG_13.bits.CFA_MOIRE_DIFFW_LOW = 0x90;
	demosiac_cfg.demosiac_1_cfg.REG_13.bits.CFA_MOIRE_DIFFW_HIGH = 0xa0;
	demosiac_cfg.demosiac_1_cfg.REG_13.bits.CFA_MOIRE_SADTHD_MIN = 0xb0;
	demosiac_cfg.demosiac_1_cfg.REG_14.bits.CFA_MOIRE_SADTHD_SLOPE = 0xc0;
	demosiac_cfg.demosiac_1_cfg.REG_14.bits.CFA_MOIRE_SADW_LOW = 0xd0;
	demosiac_cfg.demosiac_1_cfg.REG_14.bits.CFA_MOIRE_SADW_HIGH = 0xe0;
	demosiac_cfg.demosiac_1_cfg.REG_15.bits.CFA_MOIRE_LUMAW_LOW = 0xf0;
	demosiac_cfg.demosiac_1_cfg.REG_15.bits.CFA_MOIRE_LUMAW_HIGH = 0xf1;
	demosiac_cfg.demosiac_1_cfg.REG_16.bits.CFA_MOIRE_LUMATHD_MIN = 0xf2;
	demosiac_cfg.demosiac_1_cfg.REG_16.bits.CFA_MOIRE_LUMATHD_SLOPE = 0xf3;

	dci_cfg.enable = 1;
	dci_cfg.dither_enable = 1;
	dci_cfg.demo_mode = 1;
	memcpy(dci_cfg.map_lut, dci_lut, sizeof(dci_lut));

	thredlut_cfg.enable = 1;
	thredlut_cfg.h_clamp_wrap_opt = 1;
	memcpy(thredlut_cfg.h_lut, C_lut, sizeof(C_lut));
	memcpy(thredlut_cfg.s_lut, C_lut, sizeof(C_lut));
	memcpy(thredlut_cfg.v_lut, C_lut, sizeof(C_lut));

	lsc_cfg.enable = 1;
	lsc_cfg.gain_base = 0;
	lsc_cfg.strength = 2048;
	lsc_cfg.debug = 1;

	//LSCR
	lscr_cfg.enable = 1;
	lscr_cfg.strength = 2048;
	lscr_cfg.centerx = s.w / 2;
	lscr_cfg.centery = s.h / 2;
	memcpy(lscr_cfg.gain_lut, lscr_gain_lut, sizeof(lscr_gain_lut));
}

void isp_tuning_ctrl(int fd, const struct size s)
{
	uint8_t op;

	_isp_tun_setting_init(s);

	ut_pr(UT_INFO, "00) cnr      01) pfc     02) tnr    03) bnr    04) ynr\n");
	ut_pr(UT_INFO, "05) demosiac 06) dci     07) 3dlut  08) lsc    09) tun_buf\n");
	ut_pr(UT_INFO, "10) set snr info 11) dpc 12) lsc    13) lscr   14) ae\n");
	ut_pr(UT_INFO, "15) awb      16) fusion  255) exit\n");
	scanf("%d", (int *)&op);

	ut_pr(UT_INFO, "op=%d\n", op);

	switch (op) {
	case 0: {
		ISP_EXT_CTRL(fd, &cnr_cfg, V4L2_CID_DV_VIP_ISP_CNR_CFG);
	}
	break;

	case 1: {
		ISP_EXT_CTRL(fd, &pfc_cfg, V4L2_CID_DV_VIP_ISP_PFC_CFG);
	}
	break;

	case 2: {
		ISP_EXT_CTRL(fd, &tnr_cfg, V4L2_CID_DV_VIP_ISP_TNR_CFG);
	}
	break;

	case 3: {
		ISP_EXT_CTRL(fd, &bnr_cfg, V4L2_CID_DV_VIP_ISP_BNR_CFG);
	}
	break;

	case 4: {
		ISP_EXT_CTRL(fd, &ynr_cfg, V4L2_CID_DV_VIP_ISP_YNR_CFG);
	}
	break;

	case 5: {
		ISP_EXT_CTRL(fd, &demosiac_cfg, V4L2_CID_DV_VIP_ISP_DEMOSIAC_CFG);
	}
	break;

	case 6: {
		ISP_EXT_CTRL(fd, &dci_cfg, V4L2_CID_DV_VIP_ISP_DCI_CFG);
	}
	break;

	case 7: {
		ISP_EXT_CTRL(fd, &thredlut_cfg, V4L2_CID_DV_VIP_ISP_3DLUT_CFG);
	}
	break;

	case 8: {
		struct cvi_vip_memblock cfg[2];

		memset(cfg, 0, sizeof(cfg));

		ISP_EXT_CTRL(fd, &cfg[0], V4L2_CID_DV_VIP_ISP_GET_LSC_PHY_BUF);

		ut_pr(UT_INFO, "lsc phy addr=0x%llx\n", cfg[0].phy_addr);
		ut_pr(UT_INFO, "lsc size=0x%x\n", cfg[0].size);

		int _fd = devm_open();

		cfg[0].vir_addr = devm_map(_fd, cfg[0].phy_addr, cfg[0].size);

		uint16_t i = 0;
		uint16_t *vir_addr;

		vir_addr = (uint16_t *)cfg[0].vir_addr;

		ut_pr(UT_INFO, "vir_addr=0x%p, cfg.vir_addr=0x%p\n", vir_addr, cfg[0].vir_addr);

		for (i = 0; i < 0x100; i++) {
			vir_addr[i] = 0xa;
			ut_pr(UT_INFO, "[5566] val=0x%x\n", vir_addr[i]);
		}
	}
	break;

	case 9: {
		struct isp_tuning_cfg cfg;
		struct cvi_vip_isp_pre_cfg *pre_cfg;
		struct cvi_vip_isp_post_cfg *post_cfg;
		uint8_t i = 0;

		memset(&cfg, 0, sizeof(cfg));

		int _fd = devm_open();

		ISP_EXT_CTRL(fd, &cfg, V4L2_CID_DV_VIP_ISP_GET_TUN_ADDR);

		for (i = 0; i < 2; i++) {
			cfg.pre_vir[i] = devm_map(_fd, cfg.pre_addr[i], sizeof(struct cvi_vip_isp_pre_cfg));
			cfg.post_vir[i] = devm_map(_fd, cfg.post_addr[i], sizeof(struct cvi_vip_isp_post_cfg));
		}

		pre_cfg = (struct cvi_vip_isp_pre_cfg *)cfg.pre_vir[0];
		pre_cfg->tun_idx = 1;
		pre_cfg->tun_update[pre_cfg->tun_idx] = 1;

		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[0].inst = 0;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[0].enable = 1;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[0].bgain = 128;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[0].rgain = 64;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[0].grgain = 64;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[1].inst = 1;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[1].enable = 1;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[1].bgain = 128;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[1].rgain = 64;
		pre_cfg->tun_cfg[pre_cfg->tun_idx].blc_cfg[1].grgain = 64;

		post_cfg = (struct cvi_vip_isp_post_cfg *)cfg.post_vir[0];
		post_cfg->tun_idx = 1;
		post_cfg->tun_update[post_cfg->tun_idx] = 1;

		ut_pr(UT_INFO, "pre_tun_update/idx=%d:%d\n",
				pre_cfg->tun_update[pre_cfg->tun_idx], pre_cfg->tun_idx);
		ut_pr(UT_INFO, "post_tun_update/idx=%d:%d\n",
				post_cfg->tun_update[post_cfg->tun_idx], post_cfg->tun_idx);
	}
	break;

	case 10: {
		struct cvi_isp_snr_info cfg;

		cfg.color_mode = MEDIA_BUS_FMT_SGBRG12_1X12;
		cfg.snr_fmt.frm_num = 1;
		cfg.snr_fmt.img_size[0].width = s.w;
		cfg.snr_fmt.img_size[0].height = s.h;
		cfg.snr_fmt.img_size[0].start_x = 0;
		cfg.snr_fmt.img_size[0].start_y = 0;
		cfg.snr_fmt.img_size[0].active_w = s.w;
		cfg.snr_fmt.img_size[0].active_h = s.h;

		ISP_EXT_CTRL(fd, &cfg, V4L2_CID_DV_VIP_ISP_SET_SNR_INFO);
	}
	break;

	case 11: {
		ISP_EXT_CTRL(fd, &dpc_cfg, V4L2_CID_DV_VIP_ISP_DPC_CFG);
	}
	break;

	case 12: {
		ISP_EXT_CTRL(fd, &lsc_cfg, V4L2_CID_DV_VIP_ISP_LSC_CFG);
	}
	break;

	case 13: {
		ISP_EXT_CTRL(fd, &lscr_cfg, V4L2_CID_DV_VIP_ISP_LSCR_CFG);
	}
	break;

	case 14: {
		ISP_EXT_CTRL(fd, &ae_cfg[0], V4L2_CID_DV_VIP_ISP_AE_CFG);
	}
	break;

	case 15: {
		ISP_EXT_CTRL(fd, &awb_cfg, V4L2_CID_DV_VIP_ISP_AWB_CFG);
	}
	break;

	case 16: {
		ISP_EXT_CTRL(fd, &fswdr_cfg, V4L2_CID_DV_VIP_ISP_FSWDR_CFG);
	}
	break;

	case 255:
	default:
		break;
	}

	//ut_pr(UT_INFO, "press any number to con:");
	//scanf("%d", (int *)&op);
}

