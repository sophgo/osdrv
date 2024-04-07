#ifndef _VPSS_RGN_CTRL_H_
#define _VPSS_RGN_CTRL_H_

s32 vpss_get_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u8 layer,
	RGN_TYPE_E enType, RGN_HANDLE hdls[]);
s32 vpss_set_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer,
		RGN_TYPE_E enType, RGN_HANDLE hdls[]);
s32 vpss_set_rgn_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer, struct cvi_rgn_cfg *cfg);
s32 vpss_set_rgn_coverex_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_coverex_cfg *cfg);
s32 vpss_set_rgn_mosaic_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_mosaic_cfg *cfg);
s32 vpss_get_rgn_ow_addr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer,
	RGN_HANDLE handle, u64 *addr);
s32 vpss_set_rgn_lut_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_lut_cfg *cfg);

#endif /* _VPSS_RGN_CTRL_H_ */
