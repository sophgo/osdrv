#ifndef _CVI_SCL_REG_H_
#define _CVI_SCL_REG_H_

#if defined(ENV_CVITEST) || defined(ENV_EMU)
#define REG_SCL_TOP_BASE 0x0A080000
#define REG_DSI_WRAP_BASE 0x0A0D1000
#else
#define REG_SCL_TOP_BASE 0
#define REG_DSI_WRAP_BASE 0
#endif

#define REG_SCL_CMDQ_TOP_BASE 0x0A080000
#define REG_PRI_OFFSET 0x300
#define REG_SCL_GOP0_OFFSET 0x600
#define REG_SCL_GOP1_OFFSET 0x800
#define REG_DISP_GOP_OFFSET 0x800
#define REG_OENC_OFFSET 0xc00

#define REG_MAP_BASE (REG_SCL_TOP_BASE + 0x1000)
#define REG_SCL_IMG_BASE(x) (REG_SCL_TOP_BASE + 0x2000 + 0x1000*x)
#define REG_SCL_CORE_BASE(x) (REG_SCL_TOP_BASE + 0x4000 + 0x1000*x)
#define REG_SCL_DISP_BASE (REG_SCL_TOP_BASE + 0x8000)
#define REG_SCL_BT_BASE (REG_SCL_TOP_BASE + 0x9000)
#define REG_SCL_DSI_BASE (REG_SCL_TOP_BASE + 0xA000)
#define REG_SCL_CMDQ_BASE (REG_SCL_TOP_BASE + 0xB000)

// ============== TOP ============== //
#define REG_SCL_TOP_CFG0 (REG_SCL_TOP_BASE + 0x00)
#define REG_SCL_TOP_CFG1 (REG_SCL_TOP_BASE + 0x04)
#define REG_SCL_TOP_AXI (REG_SCL_TOP_BASE + 0x08)
#define REG_SCL_TOP_BT_CFG (REG_SCL_TOP_BASE + 0x0C)
#define REG_SCL_TOP_SHD (REG_SCL_TOP_BASE + 0x10)
#define REG_SC_TOP_SB_CBAR (REG_SCL_TOP_BASE + 0x1C)
#define REG_SCL_TOP_INTR_MASK (REG_SCL_TOP_BASE + 0x30)
#define REG_SCL_TOP_INTR_STATUS (REG_SCL_TOP_BASE + 0x34)
#define REG_SCL_TOP_INTR_ENABLE (REG_SCL_TOP_BASE + 0x38)
#define REG_SCL_TOP_IMG_CTRL (REG_SCL_TOP_BASE + 0x40)
#define REG_SCL_TOP_CMDQ_START (REG_SCL_TOP_BASE + 0x44)
#define REG_SCL_TOP_CMDQ_STOP (REG_SCL_TOP_BASE + 0x48)
#define REG_SCL_TOP_PG (REG_SCL_TOP_BASE + 0x4C)
#define REG_SCL_TOP_LVDSTX (REG_SCL_TOP_BASE + 0x50)
#define REG_SCL_TOP_BT_ENC (REG_SCL_TOP_BASE + 0x60)
#define REG_SCL_TOP_BT_SYNC_CODE (REG_SCL_TOP_BASE + 0x64)
#define REG_SCL_TOP_BT_BLK_DATA (REG_SCL_TOP_BASE + 0x68)
#define REG_SCL_TOP_VO_MUX (REG_SCL_TOP_BASE + 0x70)
#define REG_SCL_TOP_VO_MUX0 (REG_SCL_TOP_BASE + 0x90)
#define REG_SCL_TOP_VO_MUX1 (REG_SCL_TOP_BASE + 0x94)
#define REG_SCL_TOP_VO_MUX2 (REG_SCL_TOP_BASE + 0x98)
#define REG_SCL_TOP_VO_MUX3 (REG_SCL_TOP_BASE + 0x9C)
#define REG_SCL_TOP_VO_MUX4 (REG_SCL_TOP_BASE + 0xA0)
#define REG_SCL_TOP_VO_MUX5 (REG_SCL_TOP_BASE + 0xA4)
#define REG_SCL_TOP_VO_MUX6 (REG_SCL_TOP_BASE + 0xA8)
#define REG_SCL_TOP_VO_MUX7 (REG_SCL_TOP_BASE + 0xAC)

#define REG_SCL_TOP_BLD_CTRL (REG_SCL_TOP_BASE + 0x200)
#define REG_SCL_TOP_BLD_SIZE (REG_SCL_TOP_BASE + 0x204)

// Privacy mask
#define REG_SCL_TOP_PRI_CFG (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x00)
#define REG_SCL_TOP_PRI_START (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x04)
#define REG_SCL_TOP_PRI_END (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x08)
#define REG_SCL_TOP_PRI_ALPHA (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x0C)
#define REG_SCL_TOP_PRI_MAP_ADDR_L (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x10)
#define REG_SCL_TOP_PRI_MAP_ADDR_H (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x14)
#define REG_SCL_TOP_PRI_MAP_AXI_CFG (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x18)
#define REG_SCL_TOP_PRI_GRID_CFG (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x1C)
#define REG_SCL_TOP_PRI_SRC_SIZE (REG_SCL_TOP_BASE + REG_PRI_OFFSET + 0x24)

// OSD encoder
#define REG_SCL_TOP_OENC_RST (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x00)
#define REG_SCL_TOP_OENC_INT_GO (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x04)
#define REG_SCL_TOP_OENC_HEADER_CFG (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x08)
#define REG_SCL_TOP_OENC_CFG (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x10)
#define REG_SCL_TOP_OENC_RANGE (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x14)
#define REG_SCL_TOP_OENC_PITCH (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x18)
#define REG_SCL_TOP_OENC_SRC_ADDR (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x20)
#define REG_SCL_TOP_OENC_WPROT_LADDR (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x30)
#define REG_SCL_TOP_OENC_WPROT_UADDR (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x38)
#define REG_SCL_TOP_OENC_BSO_ADDR (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x40)
#define REG_SCL_TOP_OENC_LIMIT_BSZ (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x48)
#define REG_SCL_TOP_OENC_BSO_SZ (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x4C)
#define REG_SCL_TOP_OENC_WPROT_DEBUG (REG_SCL_TOP_BASE + REG_OENC_OFFSET + 0x50)

// ============== IMG ============== //
#define REG_SCL_IMG_CFG(x) (REG_SCL_IMG_BASE(x) + 0x00)
#define REG_SCL_IMG_OFFSET(x) (REG_SCL_IMG_BASE(x) + 0x04)
#define REG_SCL_IMG_SIZE(x) (REG_SCL_IMG_BASE(x) + 0x08)
#define REG_SCL_IMG_PITCH_Y(x) (REG_SCL_IMG_BASE(x) + 0x0C)
#define REG_SCL_IMG_PITCH_C(x) (REG_SCL_IMG_BASE(x) + 0x10)
#define REG_SCL_IMG_SHD(x) (REG_SCL_IMG_BASE(x) + 0x14)
#define REG_SCL_IMG_ADDR0_L(x) (REG_SCL_IMG_BASE(x) + 0x24)
#define REG_SCL_IMG_ADDR0_H(x) (REG_SCL_IMG_BASE(x) + 0x28)
#define REG_SCL_IMG_ADDR1_L(x) (REG_SCL_IMG_BASE(x) + 0x2C)
#define REG_SCL_IMG_ADDR1_H(x) (REG_SCL_IMG_BASE(x) + 0x30)
#define REG_SCL_IMG_ADDR2_L(x) (REG_SCL_IMG_BASE(x) + 0x34)
#define REG_SCL_IMG_ADDR2_H(x) (REG_SCL_IMG_BASE(x) + 0x38)
#define REG_SCL_IMG_CSC_COEF0(x) (REG_SCL_IMG_BASE(x) + 0x40)
#define REG_SCL_IMG_CSC_COEF1(x) (REG_SCL_IMG_BASE(x) + 0x44)
#define REG_SCL_IMG_CSC_COEF2(x) (REG_SCL_IMG_BASE(x) + 0x48)
#define REG_SCL_IMG_CSC_COEF3(x) (REG_SCL_IMG_BASE(x) + 0x4C)
#define REG_SCL_IMG_CSC_COEF4(x) (REG_SCL_IMG_BASE(x) + 0x50)
#define REG_SCL_IMG_CSC_COEF5(x) (REG_SCL_IMG_BASE(x) + 0x54)
#define REG_SCL_IMG_CSC_SUB(x) (REG_SCL_IMG_BASE(x) + 0x58)
#define REG_SCL_IMG_CSC_ADD(x) (REG_SCL_IMG_BASE(x) + 0x5C)
#define REG_SCL_IMG_FIFO_THR(x) (REG_SCL_IMG_BASE(x) + 0x60)
#define REG_SCL_IMG_OUTSTANDING(x) (REG_SCL_IMG_BASE(x) + 0x64)
#define REG_SCL_IMG_DBG(x) (REG_SCL_IMG_BASE(x) + 0x68)
#define REG_SCL_IMG_AXI_ST(x) (REG_SCL_IMG_BASE(x) + 0x70)
#define REG_SCL_IMG_BW_LIMIT(x) (REG_SCL_IMG_BASE(x) + 0x74)
#define REG_SCL_IMG_CATCH(x) (REG_SCL_IMG_BASE(x) + 0x80)
#define REG_SCL_IMG_CHECKSUM0(x) (REG_SCL_IMG_BASE(x) + 0x84)
#define REG_SCL_IMG_CHECKSUM1(x) (REG_SCL_IMG_BASE(x) + 0x88)
#define REG_SCL_IMG_SB_REG_CTRL(x) (REG_SCL_IMG_BASE(x) + 0x90)
#define REG_SCL_IMG_SB_REG_C_STAT(x) (REG_SCL_IMG_BASE(x) + 0x94)
#define REG_SCL_IMG_SB_REG_Y_STAT(x) (REG_SCL_IMG_BASE(x) + 0x98)

// ============== SCL ============== //
#define REG_SCL_BORDER_BASE(x) (REG_SCL_CORE_BASE(x) + 0x080)
#define REG_SCL_CIR_BASE(x) (REG_SCL_CORE_BASE(x) + 0x200)
#define REG_SCL_COVER_BASE(x) (REG_SCL_CORE_BASE(x) + 0x280)
#define REG_SCL_PRI_BASE(x) (REG_SCL_CORE_BASE(x) + REG_PRI_OFFSET)
#define REG_SCL_GOP0_BASE(x) (REG_SCL_CORE_BASE(x) + REG_SCL_GOP0_OFFSET)
#define REG_SCL_GOP1_BASE(x) (REG_SCL_CORE_BASE(x) + REG_SCL_GOP1_OFFSET)
#define REG_SCL_ODMA_BASE(x) (REG_SCL_CORE_BASE(x) + 0xC00)
#define REG_SCL_CSC_BASE(x) (REG_SCL_CORE_BASE(x) + 0xD00)

// SCL
#define REG_SCL_CFG(x) (REG_SCL_CORE_BASE(x) + 0x00)
#define REG_SCL_SHD(x) (REG_SCL_CORE_BASE(x) + 0x04)
#define REG_SCL_STATUS(x) (REG_SCL_CORE_BASE(x) + 0x08)
#define REG_SCL_SRC_SIZE(x) (REG_SCL_CORE_BASE(x) + 0x0c)
#define REG_SCL_CROP_OFFSET(x) (REG_SCL_CORE_BASE(x) + 0x10)
#define REG_SCL_CROP_SIZE(x) (REG_SCL_CORE_BASE(x) + 0x14)
#define REG_SCL_CORE_CFG(x) (REG_SCL_CORE_BASE(x) + 0x40)
#define REG_SCL_2TAP_CFG(x) (REG_SCL_CORE_BASE(x) + 0x44)
#define REG_SCL_2TAP_NOR(x) (REG_SCL_CORE_BASE(x) + 0x48)
#define REG_SCL_2TAP_DBG(x) (REG_SCL_CORE_BASE(x) + 0x4c)
#define REG_SCL_CHECKSUM0(x) (REG_SCL_CORE_BASE(x) + 0x90)
#define REG_SCL_CHECKSUM1(x) (REG_SCL_CORE_BASE(x) + 0x94)
#define REG_SCL_CHECKSUM2(x) (REG_SCL_CORE_BASE(x) + 0x98)
#define REG_SCL_CHECKSUM3(x) (REG_SCL_CORE_BASE(x) + 0x9c)
#define REG_SCL_COEF0(x) (REG_SCL_CORE_BASE(x) + 0x118)
#define REG_SCL_COEF1(x) (REG_SCL_CORE_BASE(x) + 0x11C)
#define REG_SCL_COEF2(x) (REG_SCL_CORE_BASE(x) + 0x120)
#define REG_SCL_COEF3(x) (REG_SCL_CORE_BASE(x) + 0x124)
#define REG_SCL_COEF4(x) (REG_SCL_CORE_BASE(x) + 0x128)
#define REG_SCL_COEF5(x) (REG_SCL_CORE_BASE(x) + 0x12C)
#define REG_SCL_COEF6(x) (REG_SCL_CORE_BASE(x) + 0x130)
#define REG_SCL_SC_CFG(x) (REG_SCL_CORE_BASE(x) + 0x134)
#define REG_SCL_SC_H_CFG(x) (REG_SCL_CORE_BASE(x) + 0x138)
#define REG_SCL_SC_V_CFG(x) (REG_SCL_CORE_BASE(x) + 0x13C)
#define REG_SCL_OUT_SIZE(x) (REG_SCL_CORE_BASE(x) + 0x140)
#define REG_SCL_SC_INI_PH(x) (REG_SCL_CORE_BASE(x) + 0x148)

// CIRCLE
#define REG_SCL_CIR_CFG(x) (REG_SCL_CIR_BASE(x) + 0x00)
#define REG_SCL_CIR_CENTER_X(x) (REG_SCL_CIR_BASE(x) + 0x04)
#define REG_SCL_CIR_CENTER_Y(x) (REG_SCL_CIR_BASE(x) + 0x08)
#define REG_SCL_CIR_RADIUS(x) (REG_SCL_CIR_BASE(x) + 0x0C)
#define REG_SCL_CIR_SIZE(x) (REG_SCL_CIR_BASE(x) + 0x10)
#define REG_SCL_CIR_OFFSET(x) (REG_SCL_CIR_BASE(x) + 0x14)
#define REG_SCL_CIR_COLOR(x) (REG_SCL_CIR_BASE(x) + 0x18)

// COVER
#define REG_SCL_COVER_CFG(x, y) (REG_SCL_COVER_BASE(x) + 0xc*y + 0x00)
#define REG_SCL_COVER_SIZE(x, y) (REG_SCL_COVER_BASE(x) + 0xc*y + 0x04)
#define REG_SCL_COVER_COLOR(x, y) (REG_SCL_COVER_BASE(x) + 0xc*y + 0x08)

// Privacy mask
#define REG_SCL_PRI_CFG(x) (REG_SCL_PRI_BASE(x) + 0x00)
#define REG_SCL_PRI_START(x) (REG_SCL_PRI_BASE(x) + 0x04)
#define REG_SCL_PRI_END(x) (REG_SCL_PRI_BASE(x) + 0x08)
#define REG_SCL_PRI_ALPHA(x) (REG_SCL_PRI_BASE(x) + 0x0C)
#define REG_SCL_PRI_MAP_ADDR_L(x) (REG_SCL_PRI_BASE(x) + 0x10)
#define REG_SCL_PRI_MAP_ADDR_H(x) (REG_SCL_PRI_BASE(x) + 0x14)
#define REG_SCL_PRI_MAP_AXI_CFG(x) (REG_SCL_PRI_BASE(x) + 0x18)
#define REG_SCL_PRI_GRID_CFG(x) (REG_SCL_PRI_BASE(x) + 0x1C)
#define REG_SCL_PRI_DBG(x) (REG_SCL_PRI_BASE(x) + 0x20)

// GOP0
#define REG_SCL_GOP0_FMT(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x00)
#define REG_SCL_GOP0_H_RANGE(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x04)
#define REG_SCL_GOP0_V_RANGE(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x08)
#define REG_SCL_GOP0_ADDR_L(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x0c)
#define REG_SCL_GOP0_ADDR_H(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x10)
#define REG_SCL_GOP0_CROP_PITCH(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x14)
#define REG_SCL_GOP0_SIZE(x, y) (REG_SCL_GOP0_BASE(x) + 0x20*y + 0x18)
#define REG_SCL_GOP0_CFG(x) (REG_SCL_GOP0_BASE(x) + 0x100)
#define REG_SCL_GOP0_256LUT0(x) (REG_SCL_GOP0_BASE(x) + 0x104)
#define REG_SCL_GOP0_256LUT1(x) (REG_SCL_GOP0_BASE(x) + 0x108)
#define REG_SCL_GOP0_COLORKEY(x) (REG_SCL_GOP0_BASE(x) + 0x10c)
#define REG_SCL_GOP0_FONTCOLOR(x) (REG_SCL_GOP0_BASE(x) + 0x110)
#define REG_SCL_GOP0_FONTBOX_CTRL(x) (REG_SCL_GOP0_BASE(x) + 0x120)
#define REG_SCL_GOP0_FONTBOX_CFG(x, y) (REG_SCL_GOP0_BASE(x) + 0x10*y + 0x124)
#define REG_SCL_GOP0_FONTBOX_INIT(x, y) (REG_SCL_GOP0_BASE(x) + 0x10*y + 0x128)
#define REG_SCL_GOP0_FONTBOX_REC(x, y) (REG_SCL_GOP0_BASE(x) + 0x10*y + 0x12c)
#define REG_SCL_GOP0_BW_LIMIT(x) (REG_SCL_GOP0_BASE(x) + 0x140)
#define REG_SCL_GOP0_DEC_CTRL(x) (REG_SCL_GOP0_BASE(x) + 0x150)
#define REG_SCL_GOP0_DEC_DEBUG(x) (REG_SCL_GOP0_BASE(x) + 0x154)
#define REG_SCL_GOP0_16LUT(x, y) (REG_SCL_GOP0_BASE(x) + 0x2*y + 0x160)

// GOP1
#define REG_SCL_GOP1_FMT(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x00)
#define REG_SCL_GOP1_H_RANGE(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x04)
#define REG_SCL_GOP1_V_RANGE(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x08)
#define REG_SCL_GOP1_ADDR_L(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x0c)
#define REG_SCL_GOP1_ADDR_H(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x10)
#define REG_SCL_GOP1_CROP_PITCH(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x14)
#define REG_SCL_GOP1_SIZE(x, y) (REG_SCL_GOP1_BASE(x) + 0x20*y + 0x18)
#define REG_SCL_GOP1_CFG(x) (REG_SCL_GOP1_BASE(x) + 0x100)
#define REG_SCL_GOP1_256LUT0(x) (REG_SCL_GOP1_BASE(x) + 0x104)
#define REG_SCL_GOP1_256LUT1(x) (REG_SCL_GOP1_BASE(x) + 0x108)
#define REG_SCL_GOP1_COLORKEY(x) (REG_SCL_GOP1_BASE(x) + 0x10c)
#define REG_SCL_GOP1_FONTCOLOR(x) (REG_SCL_GOP1_BASE(x) + 0x110)
#define REG_SCL_GOP1_FONTBOX_CTRL(x) (REG_SCL_GOP1_BASE(x) + 0x120)
#define REG_SCL_GOP1_FONTBOX_CFG(x, y) (REG_SCL_GOP1_BASE(x) + 0x10*y + 0x124)
#define REG_SCL_GOP1_FONTBOX_INIT(x, y) (REG_SCL_GOP1_BASE(x) + 0x10*y + 0x128)
#define REG_SCL_GOP1_FONTBOX_REC(x, y) (REG_SCL_GOP1_BASE(x) + 0x10*y + 0x12c)
#define REG_SCL_GOP1_BW_LIMIT(x) (REG_SCL_GOP1_BASE(x) + 0x140)
#define REG_SCL_GOP1_DEC_CTRL(x) (REG_SCL_GOP1_BASE(x) + 0x150)
#define REG_SCL_GOP1_DEC_DEBUG(x) (REG_SCL_GOP1_BASE(x) + 0x154)
#define REG_SCL_GOP1_16LUT(x, y) (REG_SCL_GOP1_BASE(x) + 0x2*y + 0x160)

// BORDER
#define REG_SCL_BORDER_CFG(x) (REG_SCL_BORDER_BASE(x) + 0x00)
#define REG_SCL_BORDER_OFFSET(x) (REG_SCL_BORDER_BASE(x) + 0x04)

// ODMA
#define REG_SCL_ODMA_CFG(x) (REG_SCL_ODMA_BASE(x) + 0x00)
#define REG_SCL_ODMA_ADDR0_L(x) (REG_SCL_ODMA_BASE(x) + 0x04)
#define REG_SCL_ODMA_ADDR0_H(x) (REG_SCL_ODMA_BASE(x) + 0x08)
#define REG_SCL_ODMA_ADDR1_L(x) (REG_SCL_ODMA_BASE(x) + 0x0C)
#define REG_SCL_ODMA_ADDR1_H(x) (REG_SCL_ODMA_BASE(x) + 0x10)
#define REG_SCL_ODMA_ADDR2_L(x) (REG_SCL_ODMA_BASE(x) + 0x14)
#define REG_SCL_ODMA_ADDR2_H(x) (REG_SCL_ODMA_BASE(x) + 0x18)
#define REG_SCL_ODMA_PITCH_Y(x) (REG_SCL_ODMA_BASE(x) + 0x1C)
#define REG_SCL_ODMA_PITCH_C(x) (REG_SCL_ODMA_BASE(x) + 0x20)
#define REG_SCL_ODMA_OFFSET_X(x) (REG_SCL_ODMA_BASE(x) + 0x24)
#define REG_SCL_ODMA_OFFSET_Y(x) (REG_SCL_ODMA_BASE(x) + 0x28)
#define REG_SCL_ODMA_WIDTH(x) (REG_SCL_ODMA_BASE(x) + 0x2C)
#define REG_SCL_ODMA_HEIGHT(x) (REG_SCL_ODMA_BASE(x) + 0x30)
#define REG_SCL_ODMA_DBG(x) (REG_SCL_ODMA_BASE(x) + 0x34)
#define REG_SCL_ODMA_SB_CTRL(x) (REG_SCL_ODMA_BASE(x) + 0x50)

// CSC
#define REG_SCL_CSC_EN(x) (REG_SCL_CSC_BASE(x) + 0x00)
#define REG_SCL_CSC_COEF0(x) (REG_SCL_CSC_BASE(x) + 0x04)
#define REG_SCL_CSC_COEF1(x) (REG_SCL_CSC_BASE(x) + 0x08)
#define REG_SCL_CSC_COEF2(x) (REG_SCL_CSC_BASE(x) + 0x0c)
#define REG_SCL_CSC_COEF3(x) (REG_SCL_CSC_BASE(x) + 0x10)
#define REG_SCL_CSC_COEF4(x) (REG_SCL_CSC_BASE(x) + 0x14)
#define REG_SCL_CSC_OFFSET(x) (REG_SCL_CSC_BASE(x) + 0x18)
#define REG_SCL_CSC_FRAC0(x) (REG_SCL_CSC_BASE(x) + 0x1C)
#define REG_SCL_CSC_FRAC1(x) (REG_SCL_CSC_BASE(x) + 0x20)

// ============== DISP ============== //
#define REG_SCL_DISP_CFG (REG_SCL_DISP_BASE + 0x00)
#define REG_SCL_DISP_TOTAL (REG_SCL_DISP_BASE + 0x04)
#define REG_SCL_DISP_VSYNC (REG_SCL_DISP_BASE + 0x08)
#define REG_SCL_DISP_VFDE (REG_SCL_DISP_BASE + 0x0C)
#define REG_SCL_DISP_VMDE (REG_SCL_DISP_BASE + 0x10)
#define REG_SCL_DISP_HSYNC (REG_SCL_DISP_BASE + 0x14)
#define REG_SCL_DISP_HFDE (REG_SCL_DISP_BASE + 0x18)
#define REG_SCL_DISP_HMDE (REG_SCL_DISP_BASE + 0x1C)
#define REG_SCL_DISP_FIFO_THR (REG_SCL_DISP_BASE + 0x30)
#define REG_SCL_DISP_ADDR0_L (REG_SCL_DISP_BASE + 0x34)
#define REG_SCL_DISP_ADDR0_H (REG_SCL_DISP_BASE + 0x38)
#define REG_SCL_DISP_ADDR1_L (REG_SCL_DISP_BASE + 0x3C)
#define REG_SCL_DISP_ADDR1_H (REG_SCL_DISP_BASE + 0x40)
#define REG_SCL_DISP_ADDR2_L (REG_SCL_DISP_BASE + 0x44)
#define REG_SCL_DISP_ADDR2_H (REG_SCL_DISP_BASE + 0x48)
#define REG_SCL_DISP_PITCH_Y (REG_SCL_DISP_BASE + 0x4C)
#define REG_SCL_DISP_PITCH_C (REG_SCL_DISP_BASE + 0x50)
#define REG_SCL_DISP_OFFSET (REG_SCL_DISP_BASE + 0x54)
#define REG_SCL_DISP_SIZE (REG_SCL_DISP_BASE + 0x58)
#define REG_SCL_DISP_OUT_CSC0 (REG_SCL_DISP_BASE + 0x5C)
#define REG_SCL_DISP_OUT_CSC1 (REG_SCL_DISP_BASE + 0x60)
#define REG_SCL_DISP_OUT_CSC2 (REG_SCL_DISP_BASE + 0x64)
#define REG_SCL_DISP_OUT_CSC3 (REG_SCL_DISP_BASE + 0x68)
#define REG_SCL_DISP_OUT_CSC4 (REG_SCL_DISP_BASE + 0x6C)
#define REG_SCL_DISP_OUT_CSC_SUB (REG_SCL_DISP_BASE + 0x70)
#define REG_SCL_DISP_OUT_CSC_ADD (REG_SCL_DISP_BASE + 0x74)
#define REG_SCL_DISP_IN_CSC0 (REG_SCL_DISP_BASE + 0x78)
#define REG_SCL_DISP_IN_CSC1 (REG_SCL_DISP_BASE + 0x7C)
#define REG_SCL_DISP_IN_CSC2 (REG_SCL_DISP_BASE + 0x80)
#define REG_SCL_DISP_IN_CSC3 (REG_SCL_DISP_BASE + 0x84)
#define REG_SCL_DISP_IN_CSC4 (REG_SCL_DISP_BASE + 0x88)
#define REG_SCL_DISP_IN_CSC_SUB (REG_SCL_DISP_BASE + 0x8C)
#define REG_SCL_DISP_IN_CSC_ADD (REG_SCL_DISP_BASE + 0x90)
#define REG_SCL_DISP_PAT_CFG (REG_SCL_DISP_BASE + 0x94)
#define REG_SCL_DISP_PAT_COLOR0 (REG_SCL_DISP_BASE + 0x98)
#define REG_SCL_DISP_PAT_COLOR1 (REG_SCL_DISP_BASE + 0x9C)
#define REG_SCL_DISP_PAT_COLOR2 (REG_SCL_DISP_BASE + 0xA0)
#define REG_SCL_DISP_PAT_COLOR3 (REG_SCL_DISP_BASE + 0xA4)
#define REG_SCL_DISP_PAT_COLOR4 (REG_SCL_DISP_BASE + 0xA8)
#define REG_SCL_DISP_DBG (REG_SCL_DISP_BASE + 0xAC)
#define REG_SCL_DISP_AXI_ST (REG_SCL_DISP_BASE + 0xB0)
#define REG_SCL_DISP_CACHE (REG_SCL_DISP_BASE + 0xC0)
#define REG_SCL_DISP_CHECKSUM0 (REG_SCL_DISP_BASE + 0xc4)
#define REG_SCL_DISP_CHECKSUM1 (REG_SCL_DISP_BASE + 0xc8)
#define REG_SCL_DISP_CHECKSUM2 (REG_SCL_DISP_BASE + 0xcc)
#define REG_SCL_DISP_DUMMY (REG_SCL_DISP_BASE + 0xF8)

// GAMMA
#define REG_SCL_DISP_GAMMA_CTRL (REG_SCL_DISP_BASE + 0x180)
#define REG_SCL_DISP_GAMMA_WR_LUT (REG_SCL_DISP_BASE + 0x184)
#define REG_SCL_DISP_GAMMA_RD_LUT (REG_SCL_DISP_BASE + 0x188)

// i80
#define REG_SCL_DISP_MCU_IF_CTRL (REG_SCL_DISP_BASE + 0x200)
#define REG_SCL_DISP_MCU_SW_CTRL (REG_SCL_DISP_BASE + 0x204)
#define REG_SCL_DISP_MCU_STATUS (REG_SCL_DISP_BASE + 0x208)
#define REG_SCL_DISP_MCU_HW_AUTO (REG_SCL_DISP_BASE + 0x210)
#define REG_SCL_DISP_MCU_HW_CMD (REG_SCL_DISP_BASE + 0x214)
#define REG_SCL_DISP_MCU_HW_CMD0 (REG_SCL_DISP_BASE + 0x218)
#define REG_SCL_DISP_MCU_HW_CMD1 (REG_SCL_DISP_BASE + 0x21c)
#define REG_SCL_DISP_MCU_HW_CMD2 (REG_SCL_DISP_BASE + 0x220)
#define REG_SCL_DISP_MCU_HW_CMD3 (REG_SCL_DISP_BASE + 0x224)
#define REG_SCL_DISP_MCU_HW_CMD4 (REG_SCL_DISP_BASE + 0x228)
#define REG_SCL_DISP_MCU_HW_CMD5 (REG_SCL_DISP_BASE + 0x22c)
#define REG_SCL_DISP_MCU_HW_CMD6 (REG_SCL_DISP_BASE + 0x230)
#define REG_SCL_DISP_MCU_HW_CMD7 (REG_SCL_DISP_BASE + 0x234)

// COVER
#define REG_SCL_DISP_COVER_CFG(y) (REG_SCL_DISP_BASE + 0xc*y + 0x280)
#define REG_SCL_DISP_COVER_SIZE(y) (REG_SCL_DISP_BASE + 0xc*y + 0x284)
#define REG_SCL_DISP_COVER_COLOR(y) (REG_SCL_DISP_BASE + 0xc*y + 0x288)

// DISP GOP
#define REG_SCL_DISP_GOP_FMT(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x00)
#define REG_SCL_DISP_GOP_H_RANGE(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x04)
#define REG_SCL_DISP_GOP_V_RANGE(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x08)
#define REG_SCL_DISP_GOP_ADDR_L(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x0c)
#define REG_SCL_DISP_GOP_ADDR_H(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x10)
#define REG_SCL_DISP_GOP_CROP_PITCH(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x14)
#define REG_SCL_DISP_GOP_SIZE(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x20*y + 0x18)
#define REG_SCL_DISP_GOP_CFG (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x100)
#define REG_SCL_DISP_GOP_256LUT0 (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x104)
#define REG_SCL_DISP_GOP_256LUT1 (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x108)
#define REG_SCL_DISP_GOP_COLORKEY (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x10c)
#define REG_SCL_DISP_GOP_FONTCOLOR (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x110)
#define REG_SCL_DISP_GOP_FONTBOX_CTRL (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x120)
#define REG_SCL_DISP_GOP_FONTBOX_CFG(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x10*y + 0x124)
#define REG_SCL_DISP_GOP_FONTBOX_INIT(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x10*y + 0x128)
#define REG_SCL_DISP_GOP_FONTBOX_REC(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x10*y + 0x12c)
#define REG_SCL_DISP_GOP_BW_LIMIT (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x140)
#define REG_SCL_DISP_GOP_DEC_CTRL (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x150)
#define REG_SCL_DISP_GOP_DEC_DEBUG (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x154)
#define REG_SCL_DISP_GOP_16LUT(y) (REG_SCL_DISP_BASE + REG_DISP_GOP_OFFSET + 0x2*y + 0x160)

// ============== ROT ============== //
#define REG_SCL_ROT_CFG (REG_SCL_ROT_BASE + 0x00)
#define REG_SCL_ROT_SHD (REG_SCL_ROT_BASE + 0x04)
#define REG_SCL_ROT_IDLE (REG_SCL_ROT_BASE + 0x08)
#define REG_SCL_ROT_FMT (REG_SCL_ROT_BASE + 0x0C)
#define REG_SCL_ROT_SRC_ADDR_L (REG_SCL_ROT_BASE + 0x10)
#define REG_SCL_ROT_SRC_ADDR_H (REG_SCL_ROT_BASE + 0x14)
#define REG_SCL_ROT_DST_ADDR_L (REG_SCL_ROT_BASE + 0x18)
#define REG_SCL_ROT_DST_ADDR_H (REG_SCL_ROT_BASE + 0x1C)
#define REG_SCL_ROT_SRC_PITCH (REG_SCL_ROT_BASE + 0x20)
#define REG_SCL_ROT_DST_PITCH (REG_SCL_ROT_BASE + 0x24)
#define REG_SCL_ROT_SRC_OFFSET_X (REG_SCL_ROT_BASE + 0x28)
#define REG_SCL_ROT_SRC_OFFSET_Y (REG_SCL_ROT_BASE + 0x2C)
#define REG_SCL_ROT_DST_OFFSET_X (REG_SCL_ROT_BASE + 0x30)
#define REG_SCL_ROT_DST_OFFSET_Y (REG_SCL_ROT_BASE + 0x34)
#define REG_SCL_ROT_WIDTH (REG_SCL_ROT_BASE + 0x38)
#define REG_SCL_ROT_HEIGHT (REG_SCL_ROT_BASE + 0x3C)

// ============== DSI ============== //
#define REG_SCL_DSI_MAC_EN  (REG_SCL_DSI_BASE + 0x00)
#define REG_SCL_DSI_HS_0    (REG_SCL_DSI_BASE + 0x04)
#define REG_SCL_DSI_HS_1    (REG_SCL_DSI_BASE + 0x08)
#define REG_SCL_DSI_ESC     (REG_SCL_DSI_BASE + 0x0C)
#define REG_SCL_DSI_ESC_TX0 (REG_SCL_DSI_BASE + 0x10)
#define REG_SCL_DSI_ESC_TX1 (REG_SCL_DSI_BASE + 0x14)
#define REG_SCL_DSI_ESC_TX2 (REG_SCL_DSI_BASE + 0x18)
#define REG_SCL_DSI_ESC_TX3 (REG_SCL_DSI_BASE + 0x1C)
#define REG_SCL_DSI_ESC_RX0 (REG_SCL_DSI_BASE + 0x20)
#define REG_SCL_DSI_ESC_RX1 (REG_SCL_DSI_BASE + 0x24)

// ============== DSI PHY ============== //
#define REG_DSI_PHY_EN             (REG_DSI_WRAP_BASE + 0x00)
#define REG_DSI_PHY_CLK_CFG1       (REG_DSI_WRAP_BASE + 0x04)
#define REG_DSI_PHY_CLK_CFG2       (REG_DSI_WRAP_BASE + 0x08)
#define REG_DSI_PHY_ESC_INIT       (REG_DSI_WRAP_BASE + 0x0C)
#define REG_DSI_PHY_ESC_WAKE       (REG_DSI_WRAP_BASE + 0x10)
#define REG_DSI_PHY_HS_CFG1        (REG_DSI_WRAP_BASE + 0x14)
#define REG_DSI_PHY_HS_CFG2        (REG_DSI_WRAP_BASE + 0x18)
#define REG_DSI_PHY_CAL_CFG        (REG_DSI_WRAP_BASE + 0x1C)
#define REG_DSI_PHY_CAL_NUM        (REG_DSI_WRAP_BASE + 0x20)
#define REG_DSI_PHY_CLK_STATE      (REG_DSI_WRAP_BASE + 0x24)
#define REG_DSI_PHY_DATA0_STATE    (REG_DSI_WRAP_BASE + 0x28)
#define REG_DSI_PHY_DATA12_STATE   (REG_DSI_WRAP_BASE + 0x2C)
#define REG_DSI_PHY_DATA3_STATE    (REG_DSI_WRAP_BASE + 0x30)
#define REG_DSI_PHY_HS_OV          (REG_DSI_WRAP_BASE + 0x38)
#define REG_DSI_PHY_HS_SW1         (REG_DSI_WRAP_BASE + 0x3C)
#define REG_DSI_PHY_HS_SW2         (REG_DSI_WRAP_BASE + 0x40)
#define REG_DSI_PHY_DATA_OV        (REG_DSI_WRAP_BASE + 0x44)
#define REG_DSI_PHY_LPTX_OV        (REG_DSI_WRAP_BASE + 0x4C)
#define REG_DSI_PHY_LPRX_OV        (REG_DSI_WRAP_BASE + 0x4C)
#define REG_DSI_PHY_PD_TXDRV       (REG_DSI_WRAP_BASE + 0x50)
#define REG_DSI_PHY_PD_EN_TX       (REG_DSI_WRAP_BASE + 0x54)
#define REG_DSI_PHY_EN_SUBLVDS     (REG_DSI_WRAP_BASE + 0x5C)
#define REG_DSI_PHY_EN_TXBIAS_OP   (REG_DSI_WRAP_BASE + 0x60)
#define REG_DSI_PHY_PD             (REG_DSI_WRAP_BASE + 0x64)
#define REG_DSI_PHY_TXPLL          (REG_DSI_WRAP_BASE + 0x6C)
#define REG_DSI_PHY_REG_74         (REG_DSI_WRAP_BASE + 0x74)
#define REG_DSI_PHY_REG_8C         (REG_DSI_WRAP_BASE + 0x8C)
#define REG_DSI_PHY_VSEL           (REG_DSI_WRAP_BASE + 0x88)
#define REG_DSI_PHY_REG_SET        (REG_DSI_WRAP_BASE + 0x90)
#define REG_DSI_PHY_LANE_SEL       (REG_DSI_WRAP_BASE + 0x9C)
#define REG_DSI_PHY_LANE_PN_SWAP   (REG_DSI_WRAP_BASE + 0xA0)
#define REG_DSI_PHY_LVDS_EN        (REG_DSI_WRAP_BASE + 0xB4)
#define REG_DSI_PHY_EXT_GPIO       (REG_DSI_WRAP_BASE + 0xC0)
#define REG_DSI_PHY_GPO            (REG_DSI_WRAP_BASE + 0xC4)
#define REG_DSI_PHY_GPI            (REG_DSI_WRAP_BASE + 0xC8)


#endif  // _CVI_SCL_REG_H_