#ifndef __TDE_REG_TAB_H__
#define __TDE_REG_TAB_H__


#define TDE_REG_BASE_ADDR                        (0x0A0A9000)

#define TDE_REG_SURFACE_REG0                     (0x00)

#define TDE_REG_SRC_SURFACE_ADDR_L               (0x04)
#define TDE_REG_SRC_SURFACE_ADDR_H               (0x08)
#define TDE_REG_SRC_SURFACE_PIXEL_4B             (0x0C)
#define TDE_REG_SRC_SURFACE_PIXEL_2B             (0x10)
#define TDE_REG_SRC_SURFACE_STRIDE               (0x14)
#define TDE_REG_SRC_SURFACE_WIDTH                (0x18)
#define TDE_REG_SRC_SURFACE_HEIGHT               (0x1C)

#define TDE_REG_DST_SURFACE_ADDR_L               (0x20)
#define TDE_REG_DST_SURFACE_ADDR_H               (0x24)
#define TDE_REG_DST_SURFACE_WIDTH                (0x28)
#define TDE_REG_DST_SURFACE_HEIGHT               (0x2C)
#define TDE_REG_DST_SURFACE_STRIDE               (0x30)

#define TDE_REG_DRAW_LINE_CTRL0                 (0x34)
#define TDE_REG_DRAW_LINE_CTRL1                 (0x38)
#define TDE_REG_DRAW_LINE_CTRL2                 (0x3C)
#define TDE_REG_DRAW_LINE_CTRL3                 (0x40)
#define TDE_REG_DRAW_LINE_CTRL4                 (0x44)
#define TDE_REG_DRAW_LINE_CTRL5                 (0x48)

#define TDE_REG_SHADOW_RD_SEL                   (0x4C)
#define TDE_REG_START_CTRL                      (0x50)
#define TDE_REG_DBG_BUS                         (0x54)
#define TDE_REG_CLK_GATE                        (0x58)
#define TDE_REG_INTR_STATUS                     (0x5C)
#define TDE_REG_INTR_CLEAR                      (0x60)
#define TDE_REG_BW_LIMIT_RD                     (0x64)
#define TDE_REG_BW_LIMIT_WR                     (0x68)


#endif
