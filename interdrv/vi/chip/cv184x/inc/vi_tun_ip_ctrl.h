#ifndef _VI_TUN_IP_CTRL_H_
#define _VI_TUN_IP_CTRL_H_

#include "vi_ip_comm.h"

/*******************************************************************************
 *	Tuning interfaces
 ******************************************************************************/
void vi_tuning_gamma_ips_update(struct isp_ctx *ctx, const u8 pipe);
void vi_tuning_dci_update(struct isp_ctx *ctx, const u8 pipe);
void vi_tuning_drc_update(struct isp_ctx *ctx, const u8 pipe);
void vi_tuning_clut_update(struct isp_ctx *ctx, const u8 pipe);
int vi_tuning_buf_setup(struct isp_ctx *ctx, const u8 pipe);
void *vi_get_tuning_buf_addr(u32 *size);
void vi_tuning_buf_release(struct isp_ctx *ctx, uint8_t pipe);
void vi_tuning_buf_clear(uint8_t pipe);

/*******************************************************************************
 *	Tuning modules update
 ******************************************************************************/
void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ae_config *cfg);
void pre_fe_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num);
void postraw_tuning_update(
	struct isp_ctx *ctx,
	const u8 pipe);
/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ccm_config *cfg);
void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cacp_config *cfg);
void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ca2_config *cfg);
void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gamma_config *cfg);
void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_demosiac_config *cfg);
void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lsc_config *cfg);
void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_bnr_config *cfg);
void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_clut_config *cfg,
	const u8 pipe);
void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_drc_config *cfg);
void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cnr_config *cfg,
	const u8 pipe);
void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_tnr_config *cfg,
	const u8 pipe);
void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ee_config *cfg);
void ispblk_pre_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_pre_ee_config *cfg);
void ispblk_ldci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ldci_config *cfg);
void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ycur_config *cfg);
void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dci_config *cfg);
void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_csc_config *cfg);
void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dpc_config *cfg);
void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ge_config *cfg);
void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_af_config *cfg);

#endif