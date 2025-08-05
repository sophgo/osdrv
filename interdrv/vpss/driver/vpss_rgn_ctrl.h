#ifndef _VPSS_RGN_CTRL_H_
#define _VPSS_RGN_CTRL_H_

s32 vpss_get_rgn_hdls(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
			rgn_type_e type, rgn_handle hdls[], struct vpss_ctx *ctx);
s32 vpss_set_rgn_hdls(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
			rgn_type_e type, rgn_handle hdls[], struct vpss_ctx *ctx);
s32 vpss_set_rgn_cfg(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
		struct rgn_cfg *cfg, struct vpss_ctx *ctx);
s32 vpss_set_rgn_coverex_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_coverex_cfg *cfg, struct vpss_ctx *ctx);
s32 vpss_set_rgn_mosaic_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_mosaic_cfg *cfg, struct vpss_ctx *ctx);
s32 vpss_get_rgn_ow_addr(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
		rgn_handle handle, u64 *addr, struct vpss_ctx *ctx, u8 dev_idx);
s32 vpss_set_rgn_lut_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_lut_cfg *cfg, struct vpss_ctx *ctx);

#endif /* _VPSS_RGN_CTRL_H_ */
