#ifndef _DISP_H_
#define _DISP_H_

#include "osal.h"
#include "vo_define.h"

#define DISP_MAX_INST 1
#define DISP_MAX_GOP_INST 1
#define DISP_MAX_GOP_OW_INST 8
#define DISP_MAX_COVER_INST 4
#define DISP_MAX_GOP_FB_INST 2
#define DISP_DEFAULT_BURST 7
#define DISP_DEFAULT_Y_THRESH 0x90
#define DISP_DEFAULT_C_THRESH 0x90
#define DISP_GAMMA_NODE 65
#define DISP_ALIGNMENT 0x40
#define GOP_ALIGNMENT 0x10

#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#define CEIL(x, y) (((x) + ((1 << (y)))) >> (y))   // for alignment

#define IS_YUV_FMT(x) \
	((x == DISP_FMT_YUV420) || (x == DISP_FMT_YUV422) || \
	 (x == DISP_FMT_Y_ONLY) || (x >= DISP_FMT_NV12))
#define IS_PACKED_FMT(x) \
	((x == DISP_FMT_RGB_PACKED) || (x == DISP_FMT_BGR_PACKED) || \
	 (x == DISP_FMT_YVYU) || (x == DISP_FMT_YUYV) || \
	 (x == DISP_FMT_VYUY) || (x == DISP_FMT_UYVY))

struct disp_point {
	unsigned short x;
	unsigned short y;
};

struct disp_size {
	unsigned short w;
	unsigned short h;
};

struct disp_rect {
	unsigned short x;
	unsigned short y;
	unsigned short w;
	unsigned short h;
};

enum disp_drop_mode {
	DISP_DROP_MODE_DITHER = 1,
	DISP_DROP_MODE_ROUNDING,
	DISP_DROP_MODE_DROP,
	DISP_DROP_MODE_MAX,
};

enum disp_format {
	DISP_FMT_YUV420,
	DISP_FMT_YUV422,
	DISP_FMT_RGB_PLANAR,
	DISP_FMT_BGR_PACKED, // B lsb
	DISP_FMT_RGB_PACKED, // R lsb
	DISP_FMT_Y_ONLY,
	DISP_FMT_BF16, // odma only
	DISP_FMT_NV12 = 8,
	DISP_FMT_NV21,
	DISP_FMT_YUV422SP1,
	DISP_FMT_YUV422SP2,
	DISP_FMT_YVYU,
	DISP_FMT_YUYV,
	DISP_FMT_VYUY,
	DISP_FMT_UYVY,
	DISP_FMT_MAX
};

enum disp_gop_format {
	DISP_GOP_FMT_ARGB8888,
	DISP_GOP_FMT_ARGB4444,
	DISP_GOP_FMT_ARGB1555,
	DISP_GOP_FMT_256LUT,
	DISP_GOP_FMT_16LUT,
	DISP_GOP_FMT_FONT,
	DISP_GOP_FMT_MAX
};

enum disp_csc {
	DISP_CSC_NONE,
	DISP_CSC_601_LIMIT_YUV2RGB,
	DISP_CSC_601_FULL_YUV2RGB,
	DISP_CSC_709_LIMIT_YUV2RGB,
	DISP_CSC_709_FULL_YUV2RGB,
	DISP_CSC_601_LIMIT_RGB2YUV,
	DISP_CSC_601_FULL_RGB2YUV,
	DISP_CSC_709_LIMIT_RGB2YUV,
	DISP_CSC_709_FULL_RGB2YUV,
	DISP_CSC_DATATYPE,
	DISP_CSC_MAX,
};

struct disp_csc_matrix {
	unsigned short coef[3][3];
	unsigned char sub[3];
	unsigned char add[3];
};

struct disp_mem {
	unsigned long long addr0;
	unsigned long long addr1;
	unsigned long long addr2;
	unsigned short pitch_y;
	unsigned short pitch_c;
	unsigned short start_x;
	unsigned short start_y;
	unsigned short width;
	unsigned short height;
};

struct disp_cover_cfg {
	union {
		struct {
			unsigned int x       : 16;
			unsigned int y       : 15;
			unsigned int enable  : 1;
		} b;
		unsigned int raw;
	} start;
	struct disp_size img_size;
	union {
		struct {
			unsigned int cover_color_r   : 8;
			unsigned int cover_color_g   : 8;
			unsigned int cover_color_b   : 8;
			unsigned int resv            : 8;
		} b;
		unsigned int raw;
	} color;
};

struct disp_gop_ow_cfg {
	enum disp_gop_format fmt;
	struct disp_point start;
	struct disp_point end;
	u64 addr;
	u16 crop_pixels;
	u16 pitch;
	struct disp_size mem_size;
	struct disp_size img_size;
};

struct disp_gop_fb_cfg {
	union {
		struct {
			u32 width	: 7;
			u32 resv_b7	: 1;
			u32 pix_thr	: 5;
			u32 sample_rate	: 2;
			u32 resv_b15	: 1;
			u32 fb_num	: 5;
			u32 resv_b21	: 3;
			u32 attach_ow	: 3;
			u32 resv_b27	: 1;
			u32 enable	: 1;
		} b;
		u32 raw;
	} fb_ctrl;
	u32 init_st;
};

struct disp_gop_odec_cfg {
	union {
		struct {
			u32 odec_en: 1;
			u32 odec_int_en: 1;
			u32 odec_int_clr: 1;
			u32 wdt_en: 1;
			u32 odec_dbg_ridx: 4;
			u32 odec_done: 1;
			u32 ms_disable: 1;
			u32 resev1: 6;
			u32 odec_attached_idx: 3;
			u32 resev2: 5;
			u32 odec_int_vec: 8;
		} b;
		u32 raw;
	} odec_ctrl;
	u32 odec_debug;
};

enum oenc_gop_format {
	OENC_GOP_FMT_ARGB8888,
	OENC_GOP_FMT_ARGB4444,
	OENC_GOP_FMT_ARGB1555,
	OENC_GOP_FMT_256LUT,
	OENC_GOP_FMT_16LUT,
	OENC_GOP_FMT_FONT,
	OENC_GOP_FMT_MAX
};

struct oenc_size {
	u16 w;
	u16 h;
};

struct oenc_cfg {
	enum oenc_gop_format fmt: 4;
	union {
		struct {
			u32 fmt         : 4;
			u32 resv4       : 4;
			u32 alpha_zero  : 1;
			u32 resv3       : 3;
			u32 rgb_trunc   : 2;
			u32 alpha_trunc : 2;
			u32 limit_bsz_en: 1;
			u32 limit_bsz_bypass: 1;
			u32 wprot_en    : 1;
			u32 resv11      : 11;
			u32 wdog_en     : 1;
			u32 intr_en     : 1;
		} b;
		u32 raw;
	} cfg;
	struct oenc_size src_picture_size;
	struct oenc_size src_mem_size;
	u16 src_pitch;
	u64 src_adr;
	u32 wprot_laddr;
	u32 wprot_uaddr;
	u64 bso_adr;
	u64 bso_adr1;
	u64 bso_adr2;
	u64 bso_adr3;
	u64 bso_adr4;
	u64 bso_adr5;
	u64 bso_adr6;
	u64 bso_adr7;
	u32 limit_bsz;
	u32 bso_sz; //OSD encoder original data of bso_sz
	struct oenc_size bso_mem_size; //for setting VGOP bitstream size
};

struct oenc_int {
	union {
		struct {
			u32 go      : 1;
			u32 resv7   : 7;
			u32 done    : 1;
			u32 resv6   : 6;
			u32 intr_clr: 1;
			u32 intr_vec: 16;
		} b;
		u32 raw;
	} go_intr;
};

struct disp_gop_cfg {
	union {
		struct {
			u32 ow0_en : 1;
			u32 ow1_en : 1;
			u32 ow2_en : 1;
			u32 ow3_en : 1;
			u32 ow4_en : 1;
			u32 ow5_en : 1;
			u32 ow6_en : 1;
			u32 ow7_en : 1;
			u32 hscl_en: 1;
			u32 vscl_en: 1;
			u32 colorkey_en : 1;
			u32 resv   : 1;
			u32 burst  : 4;
			u32 resv_b16 : 15;
			u32 sw_rst : 1;
		} b;
		u32 raw;
	} gop_ctrl;
	u32 colorkey;       // RGB888
	u16 font_fg_color;  // ARGB4444
	u16 font_bg_color;  // ARGB4444
	struct disp_gop_ow_cfg ow_cfg[DISP_MAX_GOP_OW_INST];
	union {
		struct {
			u32 hi_thr	: 6;
			u32 resv_b6	: 2;
			u32 lo_thr	: 6;
			u32 resv_b14	: 2;
			u32 fb_init	: 1;
			u32 lo_thr_inv	: 1;
			u32 resv_b18	: 2;
			u32 detect_fnum	: 6;
		} b;
		u32 raw;
	} fb_ctrl;
	struct disp_gop_fb_cfg fb_cfg[DISP_MAX_GOP_FB_INST];
	struct disp_gop_odec_cfg odec_cfg;
};

struct disp_cfg {
	bool disp_from_sc;  // 0(DRAM), 1(scaler_d)
	bool cache_mode;
	bool sync_ext;
	bool tgen_en;
	enum disp_format fmt;
	enum disp_csc in_csc;
	enum disp_csc out_csc;
	unsigned char burst;       // 0~15
	unsigned char out_bit;     // 6/8/10-bit
	unsigned char y_thresh;
	unsigned char c_thresh;
	enum disp_drop_mode drop_mode;
	struct disp_mem mem;
	struct disp_gop_cfg gop_cfg[DISP_MAX_GOP_INST];
};

enum disp_pat_color {
	PAT_COLOR_WHITE,
	PAT_COLOR_RED,
	PAT_COLOR_GREEN,
	PAT_COLOR_BLUE,
	PAT_COLOR_CYAN,
	PAT_COLOR_MAGENTA,
	PAT_COLOR_YELLOW,
	PAT_COLOR_BAR,
	PAT_COLOR_USR,
	PAT_COLOR_MAX
};

union disp_intr {
	struct {
		unsigned int resv0_1 : 2;
		unsigned int disp_frame_start : 1;
		unsigned int disp_frame_end : 1;
		unsigned int resv4_31 : 28;
	} b;
	unsigned int raw;
};

union disp_intr_sel {
	struct {
		unsigned int disp_frame_end : 1;
		unsigned int disp_frame_start : 1;
		unsigned int resv2_31 : 30;
	} b;
	unsigned int raw;
};

union disp_intr_clr {
	struct {
		unsigned int disp_frame_end : 1;
		unsigned int disp_frame_start : 1;
		unsigned int resv2_31 : 30;
	} b;
	unsigned int raw;
};

enum disp_pat_type {
	PAT_TYPE_FULL,
	PAT_TYPE_H_GRAD,
	PAT_TYPE_V_GRAD,
	PAT_TYPE_AUTO,
	PAT_TYPE_SNOW,
	PAT_TYPE_OFF,
	PAT_TYPE_MAX
};

/**
 * @ vsync_pol: vsync polarity
 * @ hsync_pol: hsync polarity
 * @ vtotal: total line of each frame, should sub 1,
 *           start line is included, end line isn't included
 * @ htotal: total pixel of each line, should sub 1,
 *           start pixel is included, end pixel isn't included
 * @ vsync_start: start line of vsync
 * @ vsync_end: end line of vsync, should sub 1
 * @ vfde_start: start line of actually video data
 * @ vfde_end: end line of actually video data, should sub 1
 * @ vmde_start: equal to vfde_start
 * @ vmde_end: equal to vfde_end
 * @ hsync_start: start pixel of hsync
 * @ hsync_end: end pixel of hsync, should sub 1
 * @ hfde_start: start pixel of actually video data in each line
 * @ hfde_end: end pixel of actually video data in each line, should sub 1
 * @ hmde_start: equal to hfde_start
 * @ hmde_end: equal to hfde_end
 */
struct disp_timing {
	bool vsync_pol;
	bool hsync_pol;
	unsigned short vtotal;
	unsigned short htotal;
	unsigned short vsync_start;
	unsigned short vsync_end;
	unsigned short vfde_start;
	unsigned short vfde_end;
	unsigned short vmde_start;
	unsigned short vmde_end;
	unsigned short hsync_start;
	unsigned short hsync_end;
	unsigned short hfde_start;
	unsigned short hfde_end;
	unsigned short hmde_start;
	unsigned short hmde_end;
};

union disp_dbg_status {
	struct {
		unsigned int bw_fail     : 1;
		unsigned int bw_fail_clr : 1;
		unsigned int osd_bw_fail : 1;
		unsigned int osd_bw_fail_clr : 1;
		unsigned int err_fwr_y   : 1;
		unsigned int err_fwr_u   : 1;
		unsigned int err_fwr_v   : 1;
		unsigned int err_fwr_clr : 1;
		unsigned int err_erd_y   : 1;
		unsigned int err_erd_u   : 1;
		unsigned int err_erd_v   : 1;
		unsigned int err_erd_clr : 1;
		unsigned int lb_full_y   : 1;
		unsigned int lb_full_u   : 1;
		unsigned int lb_full_v   : 1;
		unsigned int resv1       : 1;
		unsigned int lb_empty_y  : 1;
		unsigned int lb_empty_u  : 1;
		unsigned int lb_empty_v  : 1;
		unsigned int resv2       : 13;
	} b;
	unsigned int raw;
};

struct disp_checksum_status {
	union{
		struct {
			unsigned int data_in_from_sc_d   : 8;
			unsigned int data_out            : 8;
			unsigned int reserv              : 15;
			unsigned int enable              : 1;
		} b;
		unsigned int raw;
	} checksum_base;
	unsigned int axi_read_from_dram;
	unsigned int axi_read_from_gop;
};

/**
 * @ enable: gamma enbale
 * @ pre_osd: 0:osd-->gamma 1:gamma-->osd
 * @ r: LUT gamma red
 * @ g: LUT gamma green
 * @ b: LUT gamma blue
 */
struct disp_gamma_attr {
	bool enable;
	bool pre_osd;
	unsigned char  table[DISP_GAMMA_NODE];
};

void disp_ctrl_init(bool is_resume);
void disp_ctrl_deinit(void);
void disp_set_intr_mask(unsigned char inst, union disp_intr_sel disp_intr);
void disp_get_intr_mask(unsigned char inst, union disp_intr_sel *disp_intr);

void disp_intr_clr(unsigned char inst, union disp_intr_clr disp_intr);

union disp_intr disp_intr_status(unsigned char inst);
union disp_dbg_status disp_get_dbg_status(unsigned char inst, bool clr);

void disp_set_disp_base_addr(unsigned char inst, void *base);

void disp_reg_shadow_sel(unsigned char inst, bool read_shadow);
void disp_reg_force_up(unsigned char inst);

void disp_set_bw_cfg(unsigned char inst, enum disp_format fmt);
void disp_set_cfg(unsigned char inst, struct disp_cfg *cfg);
struct disp_cfg *disp_get_cfg(unsigned char inst);
int disp_set_rect(unsigned char inst, struct disp_rect rect);
void disp_set_mem(unsigned char inst, struct disp_mem *mem);
void disp_set_addr(unsigned char inst, unsigned long long addr0, unsigned long long addr1, unsigned long long addr2);
void _disp_set_in_csc(unsigned char inst, struct disp_csc_matrix *cfg);
void disp_set_in_csc(unsigned char inst, enum disp_csc csc);
void disp_set_out_csc(unsigned char inst, enum disp_csc csc);
void disp_set_pattern(unsigned char inst, enum disp_pat_type type,
			   enum disp_pat_color color, const unsigned short *rgb);
void disp_set_frame_bgcolor(unsigned char inst, unsigned short r, unsigned short g, unsigned short b);
void disp_set_window_bgcolor(unsigned char inst, unsigned short r, unsigned short g, unsigned short b);
void disp_enable_window_bgcolor(unsigned char inst, bool enable);
bool disp_tgen_enable(unsigned char inst, bool enable);
bool disp_check_tgen_enable(unsigned char inst);

void disp_timing_setup_from_reg(unsigned char inst);
void disp_cfg_setup_from_reg(unsigned char inst);
void disp_checksum_en(unsigned char inst, bool enable);
void disp_get_checksum_status(unsigned char inst, struct disp_checksum_status *status);

void disp_gamma_ctrl(unsigned char inst, bool enable, bool pre_osd);
void disp_gamma_lut_update(unsigned char inst, const unsigned char *b, const unsigned char *g, const unsigned char *r);
void disp_gamma_lut_read(unsigned char inst, struct disp_gamma_attr *gamma_attr);

int ctrl_set_disp_src(unsigned char inst, bool disp_from_sc);

void disp_set_intf(unsigned char inst, enum vo_disp_intf intf);

void disp_set_timing(unsigned char inst, struct disp_timing *timing);
struct disp_timing *disp_get_timing(unsigned char inst);
void disp_get_hw_timing(unsigned char inst, struct disp_timing *timing);


struct disp_gop_cfg *disp_gop_get_cfg(u8 inst, u8 layer);
void disp_gop_set_cfg(u8 inst, u8 layer, struct disp_gop_cfg *cfg, bool update);
void disp_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct disp_gop_ow_cfg *cfg, bool update);
int disp_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data);
int disp_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data);

#endif  //_DISP_H_
