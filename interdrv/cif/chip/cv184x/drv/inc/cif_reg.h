#ifndef _CIF_REG_H_
#define _CIF_REG_H_

#include "reg_csi_ctrl_top.h"
#include "reg_phy_4l.h"
#include "reg_phy_2l.h"
#include "reg_phy_top.h"
#include "reg_mac.h"
#include "reg_mac_vi.h"
#include "reg_sublvds_ctrl_top.h"
#include "reg_cmdq_warp.h"

#define DRAM_PHY_BASE           (0x100000000)
#define CIF_BLK_REGS_BITW       (9)
#define CIF_BLK_ID_BITW         (4)

/* when offset = 0x2000, should use new mapping function */
#define MAP_CIF_BLOCK_ID(_ba)   (((_ba) >> CIF_BLK_REGS_BITW) \
				& ((1 << CIF_BLK_ID_BITW) - 1))

/* CIF REG FIELD DEFINE */

/* CIF CSI MAC BLOCK ADDR OFFSET DEFINE */
#define CIF_MAC_BLK_BA_TOP         (0x00000000)	//sensor mac
#define CIF_MAC_BLK_BA_SLVDS       (0x00000200) //subLVDS
#define CIF_MAC_BLK_BA_CSI         (0x00000400) //csi_ctrl_top
#define CIF_MAC_VI_BLK_BA_TOP      (0x00000000)	//sensor mac_vi
#define CIF_MAC_VI_BLK_BA_SLVDS    (0x00000200) //mac_vi subLVDS
#define CIF_MAC_VI_BLK_BA_CSI      (0x00000400) //mac_vi csi_ctrl_top

enum cif_mac_blk_id_t {
	CIF_MAC_BLK_ID_TOP      = MAP_CIF_BLOCK_ID(CIF_MAC_BLK_BA_TOP),
	CIF_MAC_BLK_ID_SLVDS    = MAP_CIF_BLOCK_ID(CIF_MAC_BLK_BA_SLVDS),
	CIF_MAC_BLK_ID_CSI      = MAP_CIF_BLOCK_ID(CIF_MAC_BLK_BA_CSI),
	CIF_MAC_BLK_ID_MAX
};

enum cif_mac_vi_blk_id_t {
	CIF_MAC_VI_BLK_TOP      = MAP_CIF_BLOCK_ID(CIF_MAC_VI_BLK_BA_TOP),
	CIF_MAC_VI_BLK_ID_SLVDS = MAP_CIF_BLOCK_ID(CIF_MAC_VI_BLK_BA_SLVDS),//no use
	CIF_MAC_VI_BLK_ID_CSI   = MAP_CIF_BLOCK_ID(CIF_MAC_VI_BLK_BA_CSI),//no use
	CIF_MAC_VI_BLK_ID_MAX
};

/* CIF CSI WRAP BLOCK ADDR OFFSET DEFINE */
#define CIF_WRAP_BLK_BA_TOP		(0x00000000)
#define CIF_WRAP_BLK_BA_4L		(0x00000300)
#define CIF_WRAP_BLK_BA_2L		(0x00000600)

enum cif_wrap_blk_id_t {
	CIF_WRAP_BLK_ID_TOP		= MAP_CIF_BLOCK_ID(CIF_WRAP_BLK_BA_TOP),
	CIF_WRAP_BLK_ID_4L		= MAP_CIF_BLOCK_ID(CIF_WRAP_BLK_BA_4L),
	CIF_WRAP_BLK_ID_2L		= MAP_CIF_BLOCK_ID(CIF_WRAP_BLK_BA_2L),
	CIF_WRAP_BLK_ID_MAX
};

#endif //_CIF_REG_H_
