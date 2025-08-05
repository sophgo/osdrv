#ifndef _CIF_H_
#define _CIF_H_

#include "cif_comm.h"
#include "cif_drv.h"
#include "mmio.h"
#include "drv/gpio.h"
#include "pinctrl-mars.h"

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#define CIF_PROC_NAME "cv184x/mipi-rx"
#define MIPI_RX_DEV_NAME "cv184x-mipi-rx"
#define MAX_CIF_PROC_BUF 96
#define CONFIG_PROC_FS
#ifndef BIT
#define BIT(n)       (1UL << (n))
#endif

#if CONFIG_LOG
#define TRACE_CIF(level, fmt, ...) \
	do { \
		if (level <= DBG_DEBUG) { \
			osal_printf("%s:%d(): " fmt, __func__, __LINE__, ##__VA_ARGS__); \
		} \
	} while (0)
#else
#define TRACE_CIF(level, fmt, ...) do {} while (0)
#endif

#define container_of(ptr, type, member) ({            \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})

struct cif_dev;
struct cif_attr_s;


extern int snsr_mclk[MAX_CAM_CLK_NUM];

struct cam_pll_s {
	unsigned int	div_val_sel;
	unsigned int	src_sel;
	unsigned int	div_val;
};

struct csi_status {
	unsigned int			errcnt_ecc;
	unsigned int			errcnt_crc;
	unsigned int			errcnt_hdr;
	unsigned int			errcnt_wc;
	unsigned int			fifo_full;
};

struct lvds_status {
	unsigned int			fifo_full;
};

enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
	OF_GPIO_SINGLE_ENDED = 0x2,
	OF_GPIO_OPEN_DRAIN = 0x4,
	OF_GPIO_TRANSITORY = 0x8,
	OF_GPIO_PULL_UP = 0x10,
	OF_GPIO_PULL_DOWN = 0x20,
};

struct link {
	struct cif_ctx			cif_ctx;
	int				irq_num;
	struct reset_control		*phy_reset;
	// struct reset_control		*phy_apb_reset; //A2 no need phy_apb reset
	unsigned int			is_on;
	struct cif_param		param;
	struct combo_dev_attr_s		attr;
	enum clk_edge_e			clk_edge;
	enum output_msb_e		msb;
	unsigned int			crop_top;
	unsigned int			distance_fp;
	csi_gpio_t			snsr_rst_gpio;
	uint32_t			snsr_rst_port_idx;
	int				snsr_rst_pin;
	enum of_gpio_flags		snsr_rst_pol;
	union {
		struct csi_status	sts_csi;
		struct lvds_status	sts_lvds;
	};
	struct device			*dev;
	enum rx_mac_clk_e		mac_clk;
	enum ttl_bt_fmt_out		bt_fmt_out;
};

struct cam_clk {
	int				is_on;
	struct clk			*clk_o;
};

struct cif_dev {
	struct link			link[MAX_LINK_NUM];
	struct cam_clk		clk_cam[MAX_CAM_CLK_NUM];
	struct cam_clk		vip_sys2;
	struct cam_clk		clk_mipimpll; /* mipipll */
	struct cam_clk		clk_disppll; /* disppll */
	struct cam_clk		clk_fpll; /* fpll */
	unsigned int		max_mac_clk;
	void				*pad_ctrl;
};


int cif_set_dev_attr(struct cif_dev *dev, struct combo_dev_attr_s *attr);
int cif_set_output_clk_edge(struct cif_dev *dev, struct clk_edge_s *clk_edge);
int cif_reset_mipi(struct cif_dev *dev, uint32_t devno);
int cif_set_crop_top(struct cif_dev *dev, struct crop_top_s *crop);
int cif_set_windowing(struct cif_dev *dev, struct cif_crop_win_s *win);
int cif_set_wdr_manual(struct cif_dev *dev, struct manual_wdr_s *manual);
int cif_set_lvds_fp_vs(struct cif_dev *dev, struct vsync_gen_s *vs);
int cif_reset_snsr_gpio(struct cif_dev *dev, sns_rst_config *sns_rst_config, uint8_t on);
int cif_reset_lvds(struct cif_dev *dev, unsigned int devno);
int cif_enable_snsr_clk(struct cif_dev *dev, uint32_t devno, uint8_t on);
int cif_get_cif_attr(struct cif_dev *dev, struct cif_attr_s *cif_attr);
int cif_bt_fmt_out(struct cif_dev *dev, struct bt_fmt_out_s *fmt_out);
int _cif_enable_snsr_clk(struct cif_dev *cdev, uint32_t devno, uint8_t on);
long cif_core_ioctl(struct cif_dev *dev, unsigned int cmd, void *arg);
int proc_cif_show(void);

#endif
