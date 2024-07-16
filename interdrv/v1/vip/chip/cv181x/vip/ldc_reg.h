#ifndef _CVI_LDC_REG_H_
#define _CVI_LDC_REG_H_

#if defined(ENV_CVITEST) || defined(ENV_EMU)
#define REG_LDC_TOP_BASE 0x0A0C0000
#else
#define REG_LDC_TOP_BASE 0
#endif

#define REG_LDC_DATA_FORMAT             (REG_LDC_TOP_BASE + 0x00)
#define REG_LDC_RAS_MODE                (REG_LDC_TOP_BASE + 0x04)
#define REG_LDC_RAS_XSIZE               (REG_LDC_TOP_BASE + 0x08)
#define REG_LDC_RAS_YSIZE               (REG_LDC_TOP_BASE + 0x0C)
#define REG_LDC_MAP_BASE                (REG_LDC_TOP_BASE + 0x10)
#define REG_LDC_MAP_BYPASS              (REG_LDC_TOP_BASE + 0x14)

#define REG_LDC_SRC_BASE_Y              (REG_LDC_TOP_BASE + 0x20)
#define REG_LDC_SRC_BASE_C              (REG_LDC_TOP_BASE + 0x24)
#define REG_LDC_SRC_XSIZE               (REG_LDC_TOP_BASE + 0x28)
#define REG_LDC_SRC_YSIZE               (REG_LDC_TOP_BASE + 0x2C)
#define REG_LDC_SRC_XSTART              (REG_LDC_TOP_BASE + 0x30)
#define REG_LDC_SRC_XEND                (REG_LDC_TOP_BASE + 0x34)
#define REG_LDC_SRC_BG                  (REG_LDC_TOP_BASE + 0x38)

#define REG_LDC_DST_BASE_Y              (REG_LDC_TOP_BASE + 0x40)
#define REG_LDC_DST_BASE_C              (REG_LDC_TOP_BASE + 0x44)
#define REG_LDC_DST_MODE                (REG_LDC_TOP_BASE + 0x48)

#define REG_LDC_IRQEN                   (REG_LDC_TOP_BASE + 0x50)
#define REG_LDC_START                   (REG_LDC_TOP_BASE + 0x54)
#define REG_LDC_IRQSTAT                 (REG_LDC_TOP_BASE + 0x58)
#define REG_LDC_IRQCLR                  (REG_LDC_TOP_BASE + 0x5C)

#endif  // _CVI_LDC_REG_H_
