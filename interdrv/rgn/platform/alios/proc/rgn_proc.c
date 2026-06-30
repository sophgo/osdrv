#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "rgn_proc.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define RGN_PROC_NAME "soph/rgn"

static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
extern struct rgn_ctx rgn_prc_ctx[RGN_MAX_NUM];

/*************************************************************************
 *	Region proc functions
 *************************************************************************/
static void _pix_fmt_to_string(enum _pixel_format_e pix_fmt, char *str, int len)
{
	switch (pix_fmt) {
	case PIXEL_FORMAT_8BIT_MODE:
		strncpy(str, "256LUT", len);
		break;
	case PIXEL_FORMAT_4BIT_MODE:
		strncpy(str, "16LUT", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

int rgn_proc_show(s32 argc, char **argv)
{
	struct rgn_ctx *prgn_ctx = rgn_prc_ctx;
	int i;
	char c[32];

	(void)argc;
	(void)argv;
	if (!prgn_ctx) {
		printf("rgn_prc_ctx = NULL\n");
		return -1;
	}

	printf("\nModule: [RGN]\n");
	// Region status of overlay
	printf("\n------REGION STATUS OF OVERLAY--------------------------------------------\n");
	printf("%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s%7s%12s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum",
		"Cmpr", "MaxNeedIon");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAY_RGN && prgn_ctx[i].created) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(prgn_ctx[i].region.unattr.overlay.pixel_format, c, sizeof(c));

			printf("%7s%3d%10d%10s%20s%10d%10d%10x%20lx%20lx%10d%10d%7s%12d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N",
				c,
				prgn_ctx[i].region.unattr.overlay.size.width,
				prgn_ctx[i].region.unattr.overlay.size.height,
				prgn_ctx[i].region.unattr.overlay.bg_color,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				(uintptr_t)prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].stride,
				prgn_ctx[i].region.unattr.overlay.canvas_num,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].compressed ? "Y" : "N",
				prgn_ctx[i].max_need_ion);
		}
	}

	// Region chn status of overlay
	printf("\n------REGION CHN STATUS OF OVERLAY----------------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAY_RGN && prgn_ctx[i].created && prgn_ctx[i].used) {
			printf("%7s%3d%10d%10s%10d%10d%10s%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.overlay_chn.point.x,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_chn.point.y);
		}
	}

	// Region status of cover
	printf("\n------REGION STATUS OF COVER----------------------------------------------\n");
	printf("%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVER_RGN && prgn_ctx[i].created) {
			printf("%7s%3d%10d%10s\n", "#", prgn_ctx[i].handle, prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N");
		}
	}

	// Region chn status of rect cover
	printf("\n------REGION CHN STATUS OF RECT COVER-------------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "CoorType");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVER_RGN && prgn_ctx[i].created && prgn_ctx[i].used
			&& prgn_ctx[i].chn_attr.unchn_attr.cover_chn.cover_type == AREA_RECT) {
			printf("%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10s\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.x,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.y,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.width,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.rect.height,
				prgn_ctx[i].chn_attr.unchn_attr.cover_chn.color,
				(prgn_ctx[i].chn_attr.unchn_attr.cover_chn.coordinate == RGN_ABS_COOR) ?
					"ABS" : "RATIO");
		}
	}

	// Region status of coverex
	printf("\n------REGION STATUS OF COVEREX--------------------------------------------\n");
	printf("%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVEREX_RGN && prgn_ctx[i].created) {
			printf("%7s%3d%10d%10s\n", "#", prgn_ctx[i].handle, prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N");
		}
	}

	// Region chn status of rect coverex
	printf("\n------REGION CHN STATUS OF RECT COVEREX-----------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == COVEREX_RGN && prgn_ctx[i].created && prgn_ctx[i].used
			&& prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.cover_type == AREA_RECT) {
			printf("%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.x,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.y,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.width,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.rect.height,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.color,
				prgn_ctx[i].chn_attr.unchn_attr.cover_ex_chn.layer);
		}
	}

	// Region status of overlayex
	printf("\n------REGION STATUS OF OVERLAYEX------------------------------------------\n");
	printf("%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAYEX_RGN && prgn_ctx[i].created) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(prgn_ctx[i].region.unattr.overlay_ex.pixel_format, c, sizeof(c));

			printf("%7s%3d%10d%10s%20s%10d%10d%10x%20lx%20lx%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N",
				c,
				prgn_ctx[i].region.unattr.overlay_ex.size.width,
				prgn_ctx[i].region.unattr.overlay_ex.size.height,
				prgn_ctx[i].region.unattr.overlay_ex.bg_color,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				(uintptr_t)prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].phy_addr,
				prgn_ctx[i].canvas_info[prgn_ctx[i].canvas_idx].stride,
				prgn_ctx[i].region.unattr.overlay_ex.canvas_num);
		}
	}

	// Region chn status of overlayex
	printf("\n------REGION CHN STATUS OF OVERLAYEX--------------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == OVERLAYEX_RGN && prgn_ctx[i].created && prgn_ctx[i].used) {
			printf("%7s%3d%10d%10s%10d%10d%10s%10d%10d%10d\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.point.x,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.point.y,
				prgn_ctx[i].chn_attr.unchn_attr.overlay_ex_chn.layer);
		}
	}

	// Region status of mosaic
	printf("\n------REGION STATUS OF MOSAIC--------------------------------------------\n");
	printf("%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == MOSAIC_RGN && prgn_ctx[i].created) {
			printf("%7s%3d%10d%10s\n", "#", prgn_ctx[i].handle, prgn_ctx[i].region.type,
				(prgn_ctx[i].used) ? "Y" : "N");
		}
	}

	// Region chn status of mosaic
	printf("\n------REGION CHN STATUS OF MOSAIC-----------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "BlkSize");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgn_ctx[i].region.type == MOSAIC_RGN && prgn_ctx[i].created && prgn_ctx[i].used) {
			printf("%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10s\n",
				"#",
				prgn_ctx[i].handle,
				prgn_ctx[i].region.type,
				MOD_STRING[prgn_ctx[i].chn.mod_id],
				prgn_ctx[i].chn.dev_id,
				prgn_ctx[i].chn.chn_id,
				(prgn_ctx[i].chn_attr.show) ? "Y" : "N",
				prgn_ctx[i].chn_attr.unchn_attr.mosaic_chn.rect.x,
				prgn_ctx[i].chn_attr.unchn_attr.mosaic_chn.rect.y,
				prgn_ctx[i].chn_attr.unchn_attr.mosaic_chn.rect.width,
				prgn_ctx[i].chn_attr.unchn_attr.mosaic_chn.rect.height,
				(prgn_ctx[i].chn_attr.unchn_attr.mosaic_chn.blk_size) ? "16*16" : "8*8");
		}
	}

	return 0;
}
ALIOS_CLI_CMD_REGISTER(rgn_proc_show, proc_rgn, rgn info);
