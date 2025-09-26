#include "osal_def.h"
#include "vi_sdk_layer.h"
#include "vi_interfaces.h"
#include "vi_dump_register.h"
#include "base_ctx.h"
#include "comm_buffer.h"
#include "vi_ctx.h"
#include "defines.h"
#include "comm_errno.h"
#include "vi_raw_dump.h"
#include "base_common.h"
#include "sys.h"
#include "vbq.h"
#include "ion.h"
#include "vi_dma_setup.h"
#include "vi_tun_ip_ctrl.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static inline int check_vi_dev_valid(int dev)
{
	if (dev > (VI_MAX_DEV_NUM - 1) || dev < 0) {
		vi_pr(VI_ERR, "dev num expect 0~%d, but now %d Caller is %p\n",
		      VI_MAX_DEV_NUM - 1, dev, __builtin_return_address(0));
		return ERR_VI_INVALID_DEVID;
	}

	return 0;
}

static inline int check_vi_chn_valid(int chn)
{
	if (chn > (VI_MAX_CHN_NUM - 1) || chn < 0) {
		vi_pr(VI_ERR, "chn num expect 0~%d, but now %d Caller is %p\n",
			VI_MAX_CHN_NUM - 1, chn, __builtin_return_address(0));
		return ERR_VI_INVALID_CHNID;
	}

	return 0;
}

static inline int check_vi_chn_enable(struct sop_vi_ctx *vi_ctx, int chn)
{
	if (!vi_ctx->is_chn_enable[chn]) {
		vi_pr(VI_ERR, "chn %d, not created Caller is %p\n",
			chn, __builtin_return_address(0));
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	return 0;
}

static inline int check_vi_pipe_valid(int pipe)
{
	if (pipe < 0 || pipe > (VI_MAX_PIPE_NUM - 1)) {
		vi_pr(VI_ERR, "pipe num expect 0~%d, but now %d Caller is %p\n",
			VI_MAX_PIPE_NUM - 1, pipe, __builtin_return_address(0));
		return ERR_VI_INVALID_PIPEID;
	}

	return 0;
}

static inline int check_vi_pipe_created(struct sop_vi_ctx *vi_ctx, int pipe)
{
	if (!vi_ctx->is_pipe_created[pipe]) {
		vi_pr(VI_ERR, "pipe %d not created Caller is %p\n",
			pipe, __builtin_return_address(0));
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	return 0;
}

static inline int check_vi_size_valid(int w, int h)
{
	if (w > VI_DEV_MAX_WIDTH || h > VI_DEV_MAX_HEIGHT) {
		vi_pr(VI_ERR, "size is too large w:%d h:%d Caller is %p\n",
			w, h, __builtin_return_address(0));
		return ERR_VI_INVALID_PARA;
	}

	return 0;
}

/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/
static int _vi_sdk_drv_qbuf(struct sop_vi_dev *vdev, struct video_buffer *buf, int pipe, int chn_id)
{
	struct sop_isp_buf *qbuf;
	u8 i = 0;

	qbuf = osal_kzalloc(sizeof(struct sop_isp_buf), OSAL_GFP_ATOMIC);
	if (qbuf == NULL) {
		vi_pr(VI_ERR, "qbuf kzalloc size(%zu) failed\n", sizeof(struct sop_isp_buf));
		return ERR_VI_NOMEM;
	}

	qbuf->buf.index = pipe;
	qbuf->buf.reserved = chn_id;
	switch (buf->pixel_format) {
	default:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
		qbuf->buf.length = 3;
		break;
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		qbuf->buf.length = 2;
		break;
	case PIXEL_FORMAT_YUV_400:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_VYUY:
		qbuf->buf.length = 1;
		break;
	}

	for (i = 0; i < qbuf->buf.length; i++) {
		qbuf->buf.planes[i].addr = buf->phy_addr[i];
	}

	sop_isp_rdy_buf_queue(vdev, qbuf);

	return 0;
}

static bool is_valid_yuv_bt_mode(struct sop_vi_ctx *vi_ctx, int dev)
{
	const vi_dev_attr_s *attr = &vi_ctx->dev_attr[dev];
	bool is_yuv = (attr->input_data_type == VI_DATA_TYPE_YUV ||
			attr->input_data_type == VI_DATA_TYPE_YUV_EARLY);
	bool valid_mode = (attr->work_mode > 0 &&
			attr->intf_mode >= VI_MODE_BT656 &&
			attr->intf_mode <= VI_MODE_BT1120_INTERLEAVED);

	return is_yuv && valid_mode;
}

static bool is_valid_raw_num(int raw_num, bool is_yuv_bt)
{
	return is_yuv_bt ? raw_num >= ISP_PRERAW_LITE0 :
			(raw_num >= ISP_PRERAW0 && raw_num <= ISP_PRERAW2);
}

static int vi_set_dev_bind_info(struct sop_vi_dev *vdev, int dev, vi_dev_bind_pipe_s *attr)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	int phy_raw = attr->mipi_dev, cur_raw = attr->mipi_dev;
	bool is_yuv_bt = false;
	int pipe = 0, i = 0;

	if (!attr->num || phy_raw >= ISP_PRERAW_VIRT0) {
		vi_pr(VI_ERR, "invalid bind attr num %d phy_raw %d\n", attr->num, phy_raw);
		return -1;
	}

	if (ctx->isp_csi_cfg[phy_raw].is_mux_dev) {
		cur_raw = ctx->virt_raw_offset + ISP_PRERAW_VIRT0;
		ctx->virt_raw_offset++;
	} else {
		is_yuv_bt = is_valid_yuv_bt_mode(vi_ctx, dev);
		if (!is_valid_raw_num(phy_raw, is_yuv_bt))
			return -1;
	}

	for (i = 0; i < attr->num; i++) {
		pipe = attr->pipe_id[i];
		if (pipe <= VI_MAX_PIPE_NUM) {
			ctx->isp_csi_cfg[cur_raw].bind_pipe[i] = pipe;
			ctx->isp_pipe_cfg[pipe].bind_raw = cur_raw;
		}
	}

	ctx->isp_csi_cfg[cur_raw].bind_dev = dev;
	ctx->bind_raw[dev] = cur_raw;
	ctx->isp_csi_cfg[cur_raw].is_patgen_en = ctx->csi_patgen_en[dev];
	vi_ctx->total_chn_num += attr->num;

	vi_pr(VI_INFO, "dev(%d) raw_num_%d total_chn_num=%d\n", dev, cur_raw, vi_ctx->total_chn_num);

	return 0;
}

int vi_sdk_qbuf(mmf_chn_s mmf_chn, void *data)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)data;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	int pipe = mmf_chn.dev_id;
	int chn = mmf_chn.chn_id;
	vb_blk blk = VB_INVALID_HANDLE;
	size_s size = vi_ctx->chn_attr[pipe][chn].size;
	int rc = 0;

	if (!vdev) {
		vi_pr(VI_ERR, "null point\n");
		return ERR_VI_INVALID_NULL_PTR;
	}

	blk = vb_get_block_with_id(vi_ctx->chn_attr[pipe][chn].bind_vb_pool, vi_ctx->blk_size[pipe][chn], ID_VI);
	if (blk == VB_INVALID_HANDLE) {
		vi_ctx->chn_status[pipe][chn].vb_fail++;
		vi_pr(VI_DBG, "Can't acquire VB BLK for VI, size(%d)\n", vi_ctx->blk_size[pipe][chn]);
		return ERR_VI_NOMEM;
	}

	// workaround for ldc 64-align for width/height.
	if (vi_ctx->rotation[pipe][chn] != ROTATION_0 || vi_ctx->ldc_attr[pipe][chn].enable) {
		size.width = ALIGN(size.width, LDC_ALIGN);
		size.height = ALIGN(size.height, LDC_ALIGN);
	}

	base_get_frame_info(vi_ctx->chn_attr[pipe][chn].pixel_format
			   , size
			   , &((struct vb_s *)(uintptr_t)blk)->buf
			   , vb_handle2phys_addr(blk)
			   , VI_DEFAULT_ALIGN);

	((struct vb_s *)(uintptr_t)blk)->buf.offset_top = 0;
	((struct vb_s *)(uintptr_t)blk)->buf.offset_right = size.width - vi_ctx->chn_attr[pipe][chn].size.width;
	((struct vb_s *)(uintptr_t)blk)->buf.offset_left = 0;
	((struct vb_s *)(uintptr_t)blk)->buf.offset_bottom = size.height - vi_ctx->chn_attr[pipe][chn].size.height;

	rc = vb_qbuf(mmf_chn, CHN_TYPE_OUT, &vdev->vi_jobs[pipe][chn], blk);
	if (rc != 0) {
		vi_pr(VI_ERR, "vb_qbuf failed\n");
		return rc;
	}

	rc = _vi_sdk_drv_qbuf(vdev, &((struct vb_s *)(uintptr_t)blk)->buf, pipe, chn);
	if (rc != 0) {
		vi_pr(VI_ERR, "_vi_sdk_drv_qbuf failed\n");
		return rc;
	}

	rc = vb_release_block(blk);
	if (rc != 0) {
		vi_pr(VI_ERR, "vb_release_block failed\n");
		return rc;
	}

	return rc;
}

int vi_set_bypass_frm(struct sop_vi_dev *vdev, int pipe, u8 bypass_num)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	struct isp_ctx *ctx = &vdev->ctx;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	vi_ctx->bypass_frm[pipe] = bypass_num;
	ctx->isp_pipe_cfg[pipe].bypass_num = bypass_num;

	vi_pr(VI_INFO, "set pipe(%d) bypass_frm=%d\n", pipe, bypass_num);

	return 0;
}

int vi_set_dev_attr(struct sop_vi_dev *vdev, int dev, const vi_dev_attr_s *dev_attr)
{
	int ret = 0;
	u32 chn_num = 1;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	ret = check_vi_size_valid(dev_attr->size.width, dev_attr->size.height);
	if (ret != 0)
		return ret;

	if (vi_ctx->is_dev_enable[dev]) {
		vi_pr(VI_ERR, "vi dev(%d) is already enabled, cannot set attr.\n", dev);
		return ERR_VI_FAILED_NOT_DISABLED;
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	osal_memcpy(&vi_ctx->dev_attr[dev], dev_attr, sizeof(vi_dev_attr_s));

	if (dev_attr->input_data_type == VI_DATA_TYPE_YUV) {
		switch (dev_attr->work_mode) {
		case VI_WORK_MODE_1MULTIPLEX:
			chn_num = 1;
			break;
		case VI_WORK_MODE_2MULTIPLEX:
			chn_num = 2;
			break;
		case VI_WORK_MODE_3MULTIPLEX:
			chn_num = 3;
			break;
		case VI_WORK_MODE_4MULTIPLEX:
			chn_num = 4;
			break;
		default:
			vi_pr(VI_ERR, "SNR work mode(%d) is wrong\n", dev_attr->work_mode);
			osal_mutex_unlock(&vi_ctx->dev_lock[dev]);
			return ERR_VI_INVALID_PARA;
		}
	}

	vi_ctx->dev_attr[dev].chn_num = chn_num;

	vi_pr(VI_DBG, "dev=%d chn_num=%d\n", dev, chn_num);

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

int vi_get_dev_attr(struct sop_vi_dev *vdev, int vi_dev, vi_dev_attr_s *dev_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	*dev_attr = vi_ctx->dev_attr[vi_dev];

	return ret;
}

int vi_set_dev_attr_ex(struct sop_vi_dev *vdev, int dev, const vi_dev_attr_ex_s *dev_attr_ex)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (dev_attr_ex->phy_dev > VI_MAX_PHY_DEV_NUM - 1) {
		vi_pr(VI_ERR, "phy_dev num expect 0~%d, but now %d\n",
			VI_MAX_PHY_DEV_NUM - 1, dev_attr_ex->phy_dev);
		return ERR_VI_INVALID_PARA;
	}

	if (vi_ctx->is_dev_enable[dev]) {
		vi_pr(VI_ERR, "vi dev(%d) is already enabled, cannot set attr.\n", dev);
		return ERR_VI_FAILED_NOT_DISABLED;
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	osal_memcpy(&vi_ctx->dev_attr_ex[dev], dev_attr_ex, sizeof(vi_dev_attr_ex_s));

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

int vi_get_dev_attr_ex(struct sop_vi_dev *vdev, int vi_dev, vi_dev_attr_ex_s *dev_attr_ex)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	*dev_attr_ex = vi_ctx->dev_attr_ex[vi_dev];

	return ret;
}

int vi_set_dev_bind_attr(struct sop_vi_dev *vdev, int dev, const vi_dev_bind_pipe_s *dev_bind_attr)
{
	int ret = 0, i = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (dev_bind_attr->num > VI_MAX_PIPE_NUM) {
		vi_pr(VI_ERR, "bind pipe num(%d) is too large\n", dev_bind_attr->num);
		return ERR_VI_INVALID_PARA;
	}

	if (dev_bind_attr->mipi_dev > VI_MAX_PHY_DEV_NUM - 1) {
		vi_pr(VI_ERR, "mipi_dev num expect 0~%d, but now %d\n",
			VI_MAX_PHY_DEV_NUM - 1, dev_bind_attr->mipi_dev);
		return ERR_VI_INVALID_PARA;
	}

	if (vi_ctx->is_dev_enable[dev]) {
		vi_pr(VI_ERR, "vi dev(%d) is already enabled, cannot set attr.\n", dev);
		return ERR_VI_FAILED_NOT_DISABLED;
	}

	for (i = 0; i < dev_bind_attr->num; i++) {
		if (dev_bind_attr->pipe_id[i] >= VI_MAX_PIPE_NUM || dev_bind_attr->pipe_id[i] < 0) {
			vi_pr(VI_ERR, "bind pipe id(%d) is invalid\n", dev_bind_attr->pipe_id[i]);
			return ERR_VI_INVALID_PARA;
		}
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	osal_memcpy(&vi_ctx->bind_pipe_attr[dev], dev_bind_attr, sizeof(vi_dev_bind_pipe_s));

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

int vi_get_dev_bind_attr(struct sop_vi_dev *vdev, int vi_dev, vi_dev_bind_pipe_s *dev_bind_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	*dev_bind_attr = vi_ctx->bind_pipe_attr[vi_dev];

	return ret;
}

int vi_set_dev_unbind_attr(struct sop_vi_dev *vdev, int dev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	int raw_num = 0;
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (vi_ctx->is_dev_enable[dev]) {
		vi_pr(VI_DBG, "vi dev(%d) is already enabled, cannot set unbind.", dev);
		return 0;
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	raw_num = ctx->bind_raw[dev];
	ctx->isp_csi_cfg[raw_num].is_bind = false;

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

static void vi_configure_csi(struct sop_vi_dev *vdev, uint8_t raw_num, vi_dev_attr_s *dev_attr)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t fe_chn = 0;

	if (dev_attr->intf_mode == VI_MODE_LVDS) {
		ctx->is_sublvds_path = true;
		vi_pr(VI_WARN, "SUBLVDS_PATH_ON(%d)\n", ctx->is_sublvds_path);
	}

	if (dev_attr->input_data_type == VI_DATA_TYPE_YUV ||
		dev_attr->input_data_type == VI_DATA_TYPE_YUV_EARLY) {
		ctx->isp_csi_cfg[raw_num].is_yuv_sensor	= true;
		ctx->isp_csi_cfg[raw_num].inf_mode = dev_attr->intf_mode;
		ctx->isp_csi_cfg[raw_num].mux_mode = dev_attr->work_mode;
		ctx->isp_csi_cfg[raw_num].data_seq = dev_attr->data_seq;
		ctx->isp_csi_cfg[raw_num].yuv_scene_mode = (enum isp_yuv_scene_e)dev_attr->yuv_scene_mode;
		ctx->isp_csi_cfg[raw_num].chn_num = dev_attr->work_mode + 1;
	} else {
		ctx->isp_csi_cfg[raw_num].is_hdr_on = (dev_attr->wdr_attr.wdr_mode != WDR_MODE_NONE);
		ctx->isp_csi_cfg[raw_num].chn_num = ctx->isp_csi_cfg[raw_num].is_hdr_on ? 2 : 1;
	}

	//config the csi if no sensor
	ctx->isp_csi_cfg[raw_num].csibdg_width  = dev_attr->size.width;
	ctx->isp_csi_cfg[raw_num].csibdg_height = dev_attr->size.height;
	ctx->isp_csi_cfg[raw_num].max_width     = dev_attr->size.width;
	ctx->isp_csi_cfg[raw_num].max_height    = dev_attr->size.height;

	for (fe_chn = 0; fe_chn < ctx->isp_csi_cfg[raw_num].chn_num; fe_chn++) {
		ctx->isp_csi_cfg[raw_num].crop[fe_chn].x = 0;
		ctx->isp_csi_cfg[raw_num].crop[fe_chn].y = 0;
		ctx->isp_csi_cfg[raw_num].crop[fe_chn].w = dev_attr->size.width;
		ctx->isp_csi_cfg[raw_num].crop[fe_chn].h = dev_attr->size.height;
	}

	//for patgen, the crop is same as csibdg
	ctx->isp_csi_cfg[raw_num].patgen_fps = dev_attr->snr_fps;
	ctx->isp_csi_cfg[raw_num].rgb_color_mode = bayer_type_mapping((enum isp_bayer_type_e)dev_attr->bayer_format);
	ctx->isp_csi_cfg[raw_num].rgb_color_mode_pre_crop = ctx->isp_csi_cfg[raw_num].rgb_color_mode;

	ctx->is_hdr_on |= ctx->isp_csi_cfg[raw_num].is_hdr_on;

	ctx->isp_csi_cfg[raw_num].is_enable = true;
}

static void vi_configure_mux_dev(struct sop_vi_dev *vdev, int dev)
{
	int i = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	enum sop_isp_raw cur_raw = ctx->bind_raw[dev];
	int phy_dev = vi_ctx->dev_attr_ex[dev].phy_dev;
	enum sop_isp_raw phy_raw = ctx->bind_raw[phy_dev];
	struct _csi_switch_info *info = &ctx->isp_csi_cfg[phy_raw].switch_info;
	uint8_t idx = info->cur_nums;

	if (!vi_ctx->dev_attr_ex[dev].mux_dev) {
		ctx->isp_csi_cfg[cur_raw].phy_raw = cur_raw;
		vi_ctx->is_dev_enable[dev] = true;
		vi_ctx->total_dev_num++;
		return;
	}

	for (i = 0; i < VI_MAX_DEV_SWITCH_DEPTH; i++) {
		info->snr_arr[idx].cur_raw = cur_raw;
		if (vi_ctx->dev_attr_ex[dev].gpio_cfg[i].enable) {
			info->snr_arr[idx].gpio[i].enable = true;
			info->snr_arr[idx].gpio[i].port = vi_ctx->dev_attr_ex[dev].gpio_cfg[i].gpio_port;
			info->snr_arr[idx].gpio[i].pin = vi_ctx->dev_attr_ex[dev].gpio_cfg[i].gpio_pin;
			info->snr_arr[idx].gpio[i].pol = vi_ctx->dev_attr_ex[dev].gpio_cfg[i].gpio_pol;
		}

		vi_pr(VI_DBG, "gpio_enable=%d, port(%d), pin(%d), pol(%d)\n",
				info->snr_arr[idx].gpio[i].enable,
				info->snr_arr[idx].gpio[i].port,
				info->snr_arr[idx].gpio[i].pin,
				info->snr_arr[idx].gpio[i].pol);
	}

	ctx->isp_csi_cfg[cur_raw].phy_raw = phy_raw;
	ctx->isp_csi_cfg[cur_raw].is_mux_dev = true;
	info->cur_nums++;
	vi_ctx->total_dev_num++;
	vi_ctx->is_dev_enable[dev] = true;
}

int vi_enable_patgen(struct sop_vi_dev *vdev, int dev)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (dev > VI_MAX_DEV_NUM - 1 || dev < 0) {
		vi_pr(VI_ERR, "wrong dev_%d\n", dev);
		return -1;
	}

	ctx->csi_patgen_en[dev] = true;

	vi_pr(VI_INFO, "dev_%d enable Patgen\n", dev);

	return 0;
}

int vi_enable_dev(struct sop_vi_dev *vdev, int dev)
{
	int ret = 0;
	int raw_num = ISP_PRERAW0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (vi_ctx->dev_attr[dev].size.width == 0 && vi_ctx->dev_attr[dev].size.height == 0) {
		vi_pr(VI_ERR, "Call Setdev_attr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	if (!_is_fe_post_offline(ctx) && vi_ctx->total_dev_num) {
		vi_pr(VI_ERR, "onthefly or slice only support single sensor\n");
		return ERR_VI_NOT_SUPPORT;
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	ret = vi_set_dev_bind_info(vdev, dev, &vi_ctx->bind_pipe_attr[dev]);
	if (ret != 0) {
		vi_pr(VI_ERR, "bind failed\n");
		osal_mutex_unlock(&vi_ctx->dev_lock[dev]);
		return ERR_VI_INVALID_PARA;
	}

	vi_configure_mux_dev(vdev, dev);

	raw_num = ctx->bind_raw[dev];
	vi_configure_csi(vdev, raw_num, &vi_ctx->dev_attr[dev]);

	_vi_csi_ctrl_init(vdev, raw_num);

	_vi_scene_ctrl(vdev);

	vi_pr(VI_DBG, "dev_%d, raw_num(%d) enable=%d, total_dev_num=%d\n",
		dev, raw_num, vi_ctx->is_dev_enable[dev], vi_ctx->total_dev_num);

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return 0;
}

int vi_disable_dev(struct sop_vi_dev *vdev, int dev)
{
	int ret = 0;
	u8 raw_num;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (vi_ctx->is_dev_enable[dev] == 0) {
		vi_pr(VI_DBG, "vi dev(%d) is already disabled.", dev);
		return 0;
	}

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	ctx->csi_patgen_en[dev] = false;
	raw_num = ctx->bind_raw[dev];
	if (vi_csi_stop_streaming(vdev, raw_num)) {
		vi_pr(VI_ERR, "Failed to vi start streaming\n");
		osal_mutex_unlock(&vi_ctx->dev_lock[dev]);
		return -1;
	}

	vi_ctx->total_dev_num--;
	vi_ctx->is_dev_enable[dev] = false;
	osal_memset(&vi_ctx->dev_attr[dev], 0, sizeof(vi_dev_attr_s));
	osal_memset(&vi_ctx->dev_attr_ex[dev], 0, sizeof(vi_dev_attr_ex_s));
	osal_memset(&vi_ctx->bind_pipe_attr[dev], 0, sizeof(vi_dev_bind_pipe_s));

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

int vi_create_pipe(struct sop_vi_dev *vdev, int pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	if (vi_ctx->is_pipe_created[pipe]) {
		vi_pr(VI_ERR, "Pipe(%d) has been created\n", pipe);
		return 0;
	}

	ret = check_vi_size_valid(pipe_attr->max_width, pipe_attr->max_height);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	//Clear pipe_attr first.
	osal_memset(&vi_ctx->pipe_attr[pipe], 0, sizeof(vi_ctx->pipe_attr[pipe]));

	vi_ctx->is_pipe_created[pipe] = true;
	if (!(vi_ctx->source[pipe] == VI_PIPE_FRAME_SOURCE_USER_FE ||
	      vi_ctx->source[pipe] == VI_PIPE_FRAME_SOURCE_USER_BE))
		vi_ctx->source[pipe] = VI_PIPE_FRAME_SOURCE_DEV;

	vi_ctx->pipe_attr[pipe] = *pipe_attr;

	vi_pr(VI_DBG, "pipe_%d pipeCreated=%d\n", pipe, vi_ctx->is_pipe_created[pipe]);

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return 0;
}

int vi_destroy_pipe(struct sop_vi_dev *vdev, int pipe)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	struct isp_ctx *ctx = &vdev->ctx;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	if (!vi_ctx->is_pipe_created[pipe]) {
		vi_pr(VI_INFO, "Pipe(%d) has been not created\n", pipe);
		return 0;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	osal_memset(&vi_ctx->pipe_attr[pipe], 0, sizeof(vi_pipe_attr_s));
	osal_memset(&vi_ctx->dump_attr[pipe], 0, sizeof(vi_dump_attr_s));
	osal_memset(&vi_ctx->pipe_crop[pipe], 0, sizeof(crop_info_s));

	ret = vi_free_ion_buf(vdev, &vdev->ctx.isp_mempool[pipe]);
	if (ret != 0) {
		vi_pr(VI_ERR, "free ion buf failed\n");
		osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);
		return ret;
	}

	vi_tuning_buf_release(&vdev->ctx, pipe);

	vi_ctx->is_pipe_created[pipe] = false;
	vi_ctx->bypass_frm[pipe] = 0;

	ctx->isp_pipe_cfg[pipe].is_enable = false;

	vi_pr(VI_INFO, "pipe_%d destroy\n", pipe);

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return 0;
}

int vi_start_pipe(struct sop_vi_dev *vdev, int pipe)
{
	int ret = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	if (ctx->isp_pipe_cfg[pipe].is_enable) {
		vi_pr(VI_ERR, "Pipe(%d) has been started\n", pipe);
		return ret;
	}

	if (vi_ctx->pipe_attr[pipe].compress_mode == COMPRESS_MODE_TILE) {
		ctx->is_dpcm_on = true;
		vi_pr(VI_INFO, "ISP_COMPRESS_ON(%d)\n", ctx->is_dpcm_on);
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	ret = vi_csi_start_streaming(vdev, raw_num);
	if (ret != 0) {
		osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);
		vi_pr(VI_ERR, "Failed to vi start streaming\n");
		goto fail_to_start_csi;
	}

	_vi_isp_ctrl_init(vdev, pipe);

	ret = vi_get_isp_ion_buf(vdev, pipe);
	if (ret != 0) {
		osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);
		vi_pr(VI_ERR, "VI getIonBuf is failed\n");
		goto fail_to_get_ion_buf;
	}

	vi_tuning_buf_setup(ctx, pipe);

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	vi_pr(VI_INFO, "pipe_%d start_pipe\n", pipe);

	return ret;

fail_to_get_ion_buf:
	vi_csi_stop_streaming(vdev, raw_num);
	ctx->isp_pipe_cfg[pipe].is_enable = false;
fail_to_start_csi:

	return ret;
}

int vi_set_chn_attr(struct sop_vi_dev *vdev, int pipe, int chn, vi_chn_attr_s *chn_attr)
{
	int ret = 0;
	vb_cal_config_s vb_cal_config;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	chn_attr->bind_vb_pool = VB_INVALID_POOLID;

	vi_ctx->chn_attr[pipe][chn] = *chn_attr;

	if (chn_attr->pixel_format == PIXEL_FORMAT_NV12) {
		ctx->isp_pipe_cfg[pipe].is_uv_swap = true;
	}

	common_getpicbufferconfig(chn_attr->size.width, chn_attr->size.height,
					chn_attr->pixel_format, DATA_BITWIDTH_8,
					COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

	vi_ctx->blk_size[pipe][chn] = vb_cal_config.vb_size;

	vi_pr(VI_DBG, "pipe_%d chn_%d w:h(%d:%d) blksize(%d)\n", pipe, chn,
			chn_attr->size.width, chn_attr->size.height, vb_cal_config.vb_size);

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return ret;
}

int vi_get_chn_attr(struct sop_vi_dev *vdev, int pipe, int chn, vi_chn_attr_s *chn_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	*chn_attr = vi_ctx->chn_attr[pipe][chn];

	return ret;
}

int vi_set_dev_timing_attr(struct sop_vi_dev *vdev, int dev, const vi_dev_timing_attr_s *timing_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	vi_ctx->timing_attr[dev] = *timing_attr;

	vdev->usr_pic_delay = 0;

	if (timing_attr->enable) {
		if (timing_attr->frm_rate > 30)
			vdev->usr_pic_delay = 33;
		else if (timing_attr->frm_rate > 0)
			vdev->usr_pic_delay = (1000 / timing_attr->frm_rate);
	}

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return ret;
}

int vi_get_dev_timing_attr(struct sop_vi_dev *vdev, int dev, vi_dev_timing_attr_s *timing_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (vi_ctx->timing_attr[dev].enable == 0) {
		vi_pr(VI_DBG, "SetDevTimingAttr auto trig disable\n");
	}

	*timing_attr = vi_ctx->timing_attr[dev];

	return ret;
}

int vi_get_pipe_status(struct sop_vi_dev *vdev, int pipe, vi_pipe_status_s *pipe_status)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;
	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	pipe_status->enable = vdev->ctx.isp_pipe_cfg[pipe].is_enable;
	pipe_status->size.height = vi_ctx->pipe_attr[pipe].max_height;
	pipe_status->size.width = vi_ctx->pipe_attr[pipe].max_width;
	pipe_status->frame_rate = vi_ctx->pipe_attr[pipe].frame_rate.src_frame_rate;

	return ret;
}

int vi_get_chn_status(struct sop_vi_dev *vdev, int pipe, int chn, vi_chn_status_s *chn_status)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;
	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_ctx, chn);
	if (ret != 0)
		return ret;

	*chn_status = vi_ctx->chn_status[pipe][chn];

	return ret;
}

int vi_set_pipe_frame_source(struct sop_vi_dev *vdev, int pipe, const vi_pipe_frame_source_e source)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	vi_ctx->source[pipe] = source;
	vdev->isp_source = (u32)source;
	vdev->ctx.is_rawreplay = (vdev->isp_source != ISP_SOURCE_DEV);

	vi_pr(VI_INFO, "isp_source=%d\n", vdev->isp_source);
	vi_pr(VI_INFO, "ISP_PRERAW0.is_rawreplay=%d\n",
			vdev->ctx.is_rawreplay);

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return ret;
}

int vi_get_pipe_frame_source(struct sop_vi_dev *vdev, int pipe, vi_pipe_frame_source_e *psource)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	*psource = vi_ctx->source[pipe];

	return ret;
}

int vi_send_pipe_raw(struct sop_vi_dev *vdev, int pipe, const video_frame_info_s *pvideo_frame)
{
	int ret = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	u64 phy_addr;
	u32 dmaid_le, dmaid_se;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	if (vi_ctx->source[pipe] == VI_PIPE_FRAME_SOURCE_DEV) {
		vi_pr(VI_ERR, "Pipe(%d) source(%d) incorrect.\n", pipe, vi_ctx->source[pipe]);
		return ERR_VI_INVALID_PARA;
	}

	if (IS_FRAME_OFFSET_INVALID(pvideo_frame->video_frame)) {
		vi_pr(VI_ERR, "Pipe(%d) frame size (%d %d) offset (%d %d %d %d) invalid\n",
			pipe, pvideo_frame->video_frame.width, pvideo_frame->video_frame.height,
			pvideo_frame->video_frame.offset_left, pvideo_frame->video_frame.offset_right,
			pvideo_frame->video_frame.offset_top, pvideo_frame->video_frame.offset_bottom);
		return ERR_VI_INVALID_PARA;
	}

	if (!pvideo_frame->video_frame.phyaddr[0]) {
		vi_pr(VI_ERR, "auto replay le buf_addr is 0, The current operation trig failed\n");
		return ret;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	if (vi_ctx->source[pipe] == VI_PIPE_FRAME_SOURCE_USER_BE) {
		dmaid_le = ISP_BLK_ID_DMA_CTL_PRE_VI_SEL_LE;
		dmaid_se = ISP_BLK_ID_DMA_CTL_PRE_VI_SEL_SE;
	}

	if (pvideo_frame->video_frame.dynamic_range == DYNAMIC_RANGE_HDR10) {
		ctx->is_hdr_on = true;
		ctx->isp_csi_cfg[ISP_PRERAW0].is_hdr_on = true;
		ctx->isp_pipe_cfg[ISP_PRERAW0].is_hdr_on = true;
		vi_pr(VI_INFO, "HDR_ON(%d) for raw replay\n", ctx->is_hdr_on);
	}

	vdev->usr_fmt.width	= pvideo_frame->video_frame.width;
	vdev->usr_fmt.height	= pvideo_frame->video_frame.height;
	vdev->usr_fmt.code	= pvideo_frame->video_frame.bayer_format;
	vdev->usr_crop.left	= pvideo_frame->video_frame.offset_left;
	vdev->usr_crop.top	= pvideo_frame->video_frame.offset_top;
	vdev->usr_crop.width	= (pvideo_frame->video_frame.offset_right != 0)
					? pvideo_frame->video_frame.offset_right
					: pvideo_frame->video_frame.width;
	vdev->usr_crop.height	= (pvideo_frame->video_frame.offset_bottom != 0)
					? pvideo_frame->video_frame.offset_bottom
					: pvideo_frame->video_frame.height;
	ctx->isp_pipe_cfg[pipe].crop.w = vdev->usr_crop.width;
	ctx->isp_pipe_cfg[pipe].crop.h = vdev->usr_crop.height;
	ctx->isp_pipe_cfg[pipe].crop.x = vdev->usr_crop.left;
	ctx->isp_pipe_cfg[pipe].crop.y = vdev->usr_crop.top;

	phy_addr = pvideo_frame->video_frame.phyaddr[0];
	ispblk_dma_config(ctx, pipe, dmaid_le, phy_addr);
	vdev->usr_pic_phy_addr[ISP_RAW_PATH_LE] = phy_addr;
	vi_pr(VI_INFO, "raw_replay le(0x%llx)\n", vdev->usr_pic_phy_addr[ISP_RAW_PATH_LE]);

	if (ctx->is_hdr_on || pvideo_frame->video_frame.pixel_format) {
		phy_addr = pvideo_frame->video_frame.phyaddr[1];
		ispblk_dma_config(ctx, pipe, dmaid_se, phy_addr);
		vdev->usr_pic_phy_addr[ISP_RAW_PATH_SE] = phy_addr;
		vi_pr(VI_INFO, "raw_replay se(0x%llx)\n", vdev->usr_pic_phy_addr[ISP_RAW_PATH_SE]);
	}

	if (vdev->usr_pic_delay) {
		usr_pic_timer_start(vdev);
	} else {
		user_pic_trig(vdev, (pvideo_frame->video_frame.time_ref == 1));
	}

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return ret;
}

int vi_enable_chn(struct sop_vi_dev *vdev, int pipe, int chn)
{
	int rc = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	u8 num_buffers = 0;
	u8 create_thread = false;
	mmf_chn_s mmf_chn = {.mod_id = ID_VI, .dev_id = pipe, .chn_id = chn};

	if (!ctx->isp_pipe_cfg[pipe].is_enable) {
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) {
		create_thread = true;
	}

	mmf_chn.dev_id = pipe;

	num_buffers = vi_ctx->chn_attr[pipe][chn].single_vb ? 1 : VI_CHN_0_BUF;

	vi_ctx->chn_status[pipe][chn].enable = 1;
	vi_ctx->chn_status[pipe][chn].size.width = vi_ctx->chn_attr[pipe][chn].size.width;
	vi_ctx->chn_status[pipe][chn].size.height = vi_ctx->chn_attr[pipe][chn].size.height;
	vi_ctx->chn_status[pipe][chn].int_cnt = 0;
	vi_ctx->chn_status[pipe][chn].frame_num = 0;
	vi_ctx->chn_status[pipe][chn].prev_time = 0;
	vi_ctx->chn_status[pipe][chn].frame_rate = 0;

	base_mod_jobs_init(&vdev->vi_jobs[pipe][chn], 0, num_buffers, vi_ctx->chn_attr[pipe][chn].depth);

	if (ctx->isp_pipe_cfg[pipe].is_offline_scaler) {
		u8 j = 0;

		for (j = 0; j < num_buffers; j++) {
			rc = vi_sdk_qbuf(mmf_chn, vdev);
			if (rc) {
				vi_pr(VI_ERR, "vi_qbuf error (%d)", rc);
				goto ERR_QBUF;
			}
		}
	}

	if (create_thread) {
		rc = vi_create_thread(vdev, E_VI_TH_EVENT_HANDLER);
		if (rc) {
			vi_pr(VI_ERR, "Failed to create VI_EVENT_HANDLER thread\n");
			goto ERR_CREATE_THREAD;
		}
	}

	if (vi_isp_start_streaming(vdev, pipe, chn)) {
		vi_pr(VI_ERR, "Failed to vi start streaming\n");
		rc = ERR_VI_SYS_NOTREADY;
		goto ERR_ISP_START_STREAMING;
	}

	return rc;

ERR_ISP_START_STREAMING:
	vi_destroy_thread(vdev, E_VI_TH_EVENT_HANDLER);
ERR_CREATE_THREAD:
ERR_QBUF:
	base_mod_jobs_exit(&vdev->vi_jobs[pipe][chn]);

	return rc;
}

int vi_disable_chn(struct sop_vi_dev *vdev, int pipe, int chn)
{
	int rc = 0;
	int i = 0, j = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	bool is_streaming = false;

	if (vi_isp_stop_streaming(vdev, pipe, chn)) {
		vi_pr(VI_INFO, "vi stop streaming\n");
		return 0;
	}

	for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
		for (j = 0; j < VI_MAX_CHN_NUM; j++) {
			if (vi_ctx->is_chn_enable[i][j])
				is_streaming = true;
		}
	}

	if (!is_streaming) {
		vi_destroy_thread(vdev, E_VI_TH_EVENT_HANDLER);
		vi_destory_dbg_thread(vdev);
	}

	if (vdev->vi_jobs[pipe][chn].inited) {
		base_mod_jobs_exit(&vdev->vi_jobs[pipe][chn]);
	}

	vdev->mesh[pipe][chn].paddr = 0;
	vdev->mesh[pipe][chn].vaddr = 0;

	return rc;
}

static int vi_sdk_enable_chn(struct sop_vi_dev *vdev, int pipe, int chn)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	if (vi_ctx->is_chn_enable[pipe][chn]) {
		vi_pr(VI_ERR, "vi pipe(%d) chn(%d) is already enabled.\n", pipe, chn);
		return ERR_VI_FAILED_NOT_DISABLED;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	ret = vi_enable_chn(vdev, pipe, chn);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI enable chn is failed\n");
		osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);
		return ret;
	}

	vi_ctx->is_chn_enable[pipe][chn] = true;

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return 0;
}

static int vi_sdk_disable_chn(struct sop_vi_dev *vdev, int pipe, int chn)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	if (!vi_ctx->is_chn_enable[pipe][chn]) {
		vi_pr(VI_INFO, "vi chn(%d) is already disabled.", chn);
		return 0;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	vi_ctx->is_chn_enable[pipe][chn] = 0;
	ret = vi_disable_chn(vdev, pipe, chn);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI disable chn is failed\n");
		osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);
		return ret;
	}

	vi_ctx->blk_size[pipe][chn] = 0;
	osal_memset(&vi_ctx->chn_crop[pipe][chn], 0, sizeof(vi_crop_info_s));
	osal_memset(&vi_ctx->chn_attr[pipe][chn], 0, sizeof(vi_chn_attr_s));
	osal_memset(&vi_ctx->chn_status[pipe][chn], 0, sizeof(vi_chn_status_s));
	osal_memset(&vi_ctx->rotation[pipe][chn], 0, sizeof(rotation_e));
	osal_memset(&vi_ctx->ldc_attr[pipe][chn], 0, sizeof(vi_ldc_attr_s));

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return 0;
}

static int vi_get_chn_frame(struct sop_vi_dev *vdev, int pipe, int chn, video_frame_info_s *frame_info, int millisec)
{
	vb_blk blk;
	struct vb_s *vb;
	int ret;
	mmf_chn_s mmf_chn = {.mod_id = ID_VI, .dev_id = pipe, .chn_id = chn};
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	int i = 0;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_ctx, chn);
	if (ret != 0)
		return ret;

	osal_memset(frame_info, 0, sizeof(*frame_info));
	ret = base_get_chn_buffer(mmf_chn, &vdev->vi_jobs[pipe][chn], &blk, millisec);
	if (ret != 0) {
		vi_pr(VI_ERR, "vi get pipe_%d, chn_%d buf fail\n", pipe, chn);
		return ERR_VI_BUF_EMPTY;
	}

	vb = (struct vb_s *)(uintptr_t)blk;
	if (chn >= VI_EXT_CHN_START)
		frame_info->video_frame.pixel_format = vi_ctx->ext_chn_attr[chn - VI_EXT_CHN_START].pixel_format;
	else
		frame_info->video_frame.pixel_format = vi_ctx->chn_attr[pipe][chn].pixel_format;
	frame_info->video_frame.width = vb->buf.size.width;
	frame_info->video_frame.height = vb->buf.size.height;
	frame_info->video_frame.time_ref = vb->buf.frm_num;
	frame_info->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		frame_info->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		frame_info->video_frame.length[i] = vb->buf.length[i];
		frame_info->video_frame.stride[i] = vb->buf.stride[i];
	}

	frame_info->video_frame.offset_top = vb->buf.offset_top;
	frame_info->video_frame.offset_bottom = vb->buf.offset_bottom;
	frame_info->video_frame.offset_left = vb->buf.offset_left;
	frame_info->video_frame.offset_right = vb->buf.offset_right;
	frame_info->video_frame.private_data = vb;

	vi_pr(VI_DBG, "pipe(%d) chn(%d) pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
			pipe, chn,
			frame_info->video_frame.pixel_format, frame_info->video_frame.width,
			frame_info->video_frame.height, frame_info->video_frame.pts,
			frame_info->video_frame.phyaddr[0], frame_info->video_frame.phyaddr[1],
			frame_info->video_frame.phyaddr[2]);
	vi_pr(VI_DBG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
			frame_info->video_frame.length[0], frame_info->video_frame.length[1],
			frame_info->video_frame.length[2], frame_info->video_frame.stride[0],
			frame_info->video_frame.stride[1], frame_info->video_frame.stride[2]);

	return ret;
}

int vi_release_chn_frame(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, video_frame_info_s *frame_info)
{
	vb_blk blk;
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_ctx, vi_chn);
	if (ret != 0)
		return ret;

	blk = vb_phys_addr2handle(frame_info->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		vi_pr(VI_ERR, "Invalid phy-address(%llx) in pvideo_frame. Can't find vb_blk.\n"
			    , frame_info->video_frame.phyaddr[0]);
		return -1;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	vi_pr(VI_DBG, "release chn frame, addr(0x%llx)\n",
			frame_info->video_frame.phyaddr[0]);

	return 0;
}

int vi_set_chn_crop(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, vi_crop_info_s *chn_crop)
{
	struct isp_ctx *ctx = &vdev->ctx;
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	if (!ctx->isp_pipe_cfg[vi_pipe].is_offline_scaler) { //online2sc
		vi_pr(VI_ERR, "not support online2sc");
		return ERR_VI_NOT_SUPPORT;
	}

	if ((chn_crop->crop_rect.x + chn_crop->crop_rect.width) >
		vi_ctx->pipe_attr[vi_pipe].max_width ||
		(chn_crop->crop_rect.y + chn_crop->crop_rect.height) >
		vi_ctx->pipe_attr[vi_pipe].max_height) {
		vi_pr(VI_ERR, "crop_x(%d)+w(%d) or y(%d)+h(%d) is bigger than chn_w(%d)_h(%d)\n",
					chn_crop->crop_rect.x,
					chn_crop->crop_rect.width,
					chn_crop->crop_rect.y,
					chn_crop->crop_rect.height,
					vi_ctx->pipe_attr[vi_pipe].max_width,
					vi_ctx->pipe_attr[vi_pipe].max_height);
		return ERR_VI_INVALID_PARA;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_pr(VI_INFO, "set chn crop x(%d), y(%d), w(%d), h(%d)\n",
						chn_crop->crop_rect.x,
						chn_crop->crop_rect.y,
						chn_crop->crop_rect.width,
						chn_crop->crop_rect.height);

	vi_ctx->chn_attr[vi_pipe][vi_chn].size.width	= chn_crop->crop_rect.width;
	vi_ctx->chn_attr[vi_pipe][vi_chn].size.height	= chn_crop->crop_rect.height;
	vi_ctx->chn_crop[vi_pipe][vi_chn]		= *chn_crop;

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

int vi_get_chn_crop(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, vi_crop_info_s *chn_crop)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	*chn_crop = vi_ctx->chn_crop[vi_pipe][vi_chn];

	return ret;
}

//TODO need refactor
int vi_set_pipe_crop(struct sop_vi_dev *vdev, int vi_pipe, crop_info_s *crop_info)
{
	int ret = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct vi_rect crop;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	if (crop_info->rect.x % 2 || crop_info->rect.y % 2 ||
	    crop_info->rect.width % 2 || crop_info->rect.height % 2) {
		vi_pr(VI_ERR, "crop_x(%d)_y(%d)_w(%d)_h(%d) must be multiple of 2.\n",
		      crop_info->rect.x, crop_info->rect.y,
		      crop_info->rect.width, crop_info->rect.height);
		return ERR_VI_INVALID_PARA;
	}

	if (crop_info->rect.x < 0 || crop_info->rect.y < 0 ||
	    crop_info->rect.x + crop_info->rect.width > vi_ctx->pipe_attr[vi_pipe].max_width ||
	    crop_info->rect.y + crop_info->rect.height > vi_ctx->pipe_attr[vi_pipe].max_height) {
		vi_pr(VI_ERR, "crop_x(%d)_y(%d) is invalid.\n",
					crop_info->rect.x,
					crop_info->rect.y);
		return ERR_VI_INVALID_PARA;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_ctx->pipe_crop[vi_pipe] = *crop_info;

	crop.x = crop_info->rect.x;
	crop.y = crop_info->rect.y;
	crop.w = crop_info->rect.width;
	crop.h = crop_info->rect.height;

	vi_pr(VI_INFO, "set chn crop x(%d), y(%d), w(%d), h(%d)\n", crop.x, crop.y, crop.w, crop.h);

	ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
	crop.w >>= 1;
	crop.h >>= 1;
	ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);

	vi_ctx->chn_attr[vi_pipe][0].size.width	= crop_info->rect.width;
	vi_ctx->chn_attr[vi_pipe][0].size.height	= crop_info->rect.height;

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

int vi_get_pipe_crop(struct sop_vi_dev *vdev, int pipe, crop_info_s *crop_info)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	*crop_info = vi_ctx->pipe_crop[pipe];

	return 0;
}

int vi_set_pipe_attr(struct sop_vi_dev *vdev, int pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[pipe]);

	vi_ctx->pipe_attr[pipe] = *pipe_attr;

	osal_mutex_unlock(&vi_ctx->pipe_lock[pipe]);

	return 0;
}

int vi_get_pipe_attr(struct sop_vi_dev *vdev, int pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, pipe);
	if (ret != 0)
		return ret;

	if (vi_ctx->pipe_attr[pipe].max_width == 0 &&
	    vi_ctx->pipe_attr[pipe].max_height == 0) {
		vi_pr(VI_ERR, "Setpipe_attr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	*pipe_attr = vi_ctx->pipe_attr[pipe];

	return 0;
}

int vi_set_pipe_dump_attr(struct sop_vi_dev *vdev, int vi_pipe, vi_dump_attr_s *dump_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_ctx->dump_attr[vi_pipe] = *dump_attr;

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

int vi_get_pipe_dump_attr(struct sop_vi_dev *vdev, int vi_pipe, vi_dump_attr_s *dump_attr)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	*dump_attr = vi_ctx->dump_attr[vi_pipe];

	return 0;
}

int vi_get_pipe_frame(struct sop_vi_dev *vdev, int pipe, video_frame_info_s *frame_info, int millisec)
{
	int ret;
	struct raw_dump_info dump[2];
	u32 dev_frm_w, dev_frm_h, frm_w, frm_h, raw_num;
	u64 phyaddr = 0, addr = 0x00;
	vb_blk  tmp_vb = VB_INVALID_HANDLE;
	int blk_size;
	int dev_id = 0, frm_num = 1, i = 0;
	struct vi_rect rawdump_crop;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	if (osal_atomic_read(&vdev->stream.isp_streamon[pipe]) == 0) {
		vi_pr(VI_ERR, "StartStream first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	if (vi_ctx->dump_attr[pipe].enable == 0) {
		vi_pr(VI_ERR, "SetPipeDumpAttr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	if (vi_ctx->dump_attr[pipe].dump_type == VI_DUMP_TYPE_YUV ||
	    vi_ctx->dump_attr[pipe].dump_type == VI_DUMP_TYPE_IR) {
		vi_pr(VI_ERR, "IR or yuv raw dump is not supported.\n");
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	osal_memset(dump, 0, sizeof(dump));
	raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	dump[0].raw_dump.raw_num = raw_num;

	dev_frm_w = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w;
	dev_frm_h = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h;

	osal_memset(&rawdump_crop, 0, sizeof(rawdump_crop));
	if ((frame_info[0].video_frame.offset_top != 0) ||
	     frame_info[0].video_frame.offset_bottom != 0 ||
	     frame_info[0].video_frame.offset_left != 0 ||
	     frame_info[0].video_frame.offset_right != 0) {
		rawdump_crop.x = frame_info[0].video_frame.offset_left;
		rawdump_crop.y = frame_info[0].video_frame.offset_top;
		rawdump_crop.w = dev_frm_w - rawdump_crop.x - frame_info[0].video_frame.offset_right;
		rawdump_crop.h = dev_frm_h - rawdump_crop.y - frame_info[0].video_frame.offset_bottom;

		vi_pr(VI_INFO, "set rawdump crop x(%d), y(%d), w(%d), h(%d)\n",
			rawdump_crop.x, rawdump_crop.y, rawdump_crop.w, rawdump_crop.h);

		frm_w = rawdump_crop.w;
		frm_h = rawdump_crop.h;
	} else {
		frm_w = dev_frm_w;
		frm_h = dev_frm_h;
	}
	ctx->isp_csi_cfg[raw_num].rawdump_crop[ISP_FE_CH0] =
		ctx->isp_csi_cfg[raw_num].rawdump_crop[ISP_FE_CH1] = rawdump_crop;

	vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;

	blk_size = vi_getrawbuffersize(frm_w, frm_h, PIXEL_FORMAT_RGB_BAYER_12BPP,
				       vi_ctx->pipe_attr[0].compress_mode, 16, false);
	vi_pr(VI_INFO, "frm_w(%d), frm_h(%d), blk_size: %d\n", frm_w, frm_h, blk_size);

	/* Check if it is valid, such as the frame from VPSS. */
	if (frame_info[0].video_frame.phyaddr[0]) {
		/* If it is valid, we can use its VB to receive raw frames. */
		tmp_vb = vb_phys_addr2handle(frame_info[0].video_frame.phyaddr[0]);
	}

	if (tmp_vb == VB_INVALID_HANDLE) {
		vi_ctx->vi_raw_blk[0] = vb_get_block_with_id(VB_INVALID_POOLID, blk_size, ID_VI);
		if (vi_ctx->vi_raw_blk[0] == VB_INVALID_HANDLE) {
			vi_pr(VI_ERR, "Alloc VB blk for RAW_LE dump failed\n");
			return -1;
		}

		phyaddr = vb_handle2phys_addr(vi_ctx->vi_raw_blk[0]);
	} else {
		if (blk_size > frame_info[0].video_frame.length[0]) {
			vi_pr(VI_ERR, "input vb blk too small, in: %d, rq: %d\n",
				blk_size, frame_info[0].video_frame.length[0]);
			return -1;
		}

		phyaddr = frame_info[0].video_frame.phyaddr[0];
		vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	}

	dump[0].raw_dump.phy_addr = phyaddr;

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		frm_num = 2;
		tmp_vb = VB_INVALID_HANDLE;

		if (frame_info[1].video_frame.phyaddr[0] != 0) {
			tmp_vb = vb_phys_addr2handle(frame_info[1].video_frame.phyaddr[0]);

			if (tmp_vb == VB_INVALID_HANDLE) {
				addr = frame_info[0].video_frame.phyaddr[0] +
					frame_info[0].video_frame.length[0];
			}

			/* You can use the second half of the same VB to receive SE raw frame. */
			if (addr == frame_info[1].video_frame.phyaddr[0]) {
				tmp_vb = 0;
			}
		}

		if (tmp_vb == VB_INVALID_HANDLE) {

			vi_ctx->vi_raw_blk[1] = vb_get_block_with_id(VB_INVALID_POOLID, blk_size, ID_VI);
			if (vi_ctx->vi_raw_blk[1] == VB_INVALID_HANDLE) {
				vb_release_block(vi_ctx->vi_raw_blk[0]);
				vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
				vi_pr(VI_ERR, "Alloc VB blk for RAW_SE dump failed\n");
				return -1;
			}

			phyaddr = vb_handle2phys_addr(vi_ctx->vi_raw_blk[1]);

		} else {

			if (blk_size > frame_info[1].video_frame.length[0]) {
				vi_pr(VI_ERR, "input vb blk too small, in: %d, rq: %d\n",
					blk_size, frame_info[1].video_frame.length[0]);
				return -1;
			}

			phyaddr = frame_info[1].video_frame.phyaddr[0];
			vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		dump[1].raw_dump.phy_addr = phyaddr;
	}

	if (millisec >= 0)
		dump[0].time_out = dump[1].time_out = millisec;

	ret = isp_raw_dump(vdev, &dump[0]);
	if (ret != 0) {
		vi_pr(VI_ERR, "_isp_raw_dump fail\n");
		return -1;
	}

	if (dump[0].is_b_not_rls) {
		if (vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[0]);
			vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}
		if (vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[1]);
			vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Release pipe frame first, buffer not release.\n");
		return -1;
	}

	if (dump[0].is_timeout) {
		if (vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[0]);
			vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}

		if (vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[1]);
			vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Get pipe frame time out(%d)\n", millisec);
		return -1;
	}

	if (dump[0].is_sig_int) {
		if (vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[0]);
			vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}

		if (vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(vi_ctx->vi_raw_blk[1]);
			vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Get pipe frame signal interrupt\n");
		return -1;
	}


	for (; i < frm_num; i++) {
		frame_info[i].video_frame.phyaddr[0]  = dump[i].raw_dump.phy_addr;
		frame_info[i].video_frame.length[0]   = blk_size;
		frame_info[i].video_frame.bayer_format  = vi_ctx->dev_attr[dev_id].bayer_format;
		frame_info[i].video_frame.compress_mode = vi_ctx->pipe_attr[0].compress_mode;
		frame_info[i].video_frame.width       = dump[i].src_w;
		frame_info[i].video_frame.height      = dump[i].src_h;
		frame_info[i].video_frame.offset_left  = dump[i].crop_x;
		frame_info[i].video_frame.offset_top   = dump[i].crop_y;
		frame_info[i].video_frame.time_ref     = dump[i].frm_num;
		frame_info[i].video_frame.pts          = dump[i].pts;
	}

	return 0;
}

int vi_release_pipe_frame(struct sop_vi_dev *vdev, int vi_pipe, video_frame_info_s *frame_info)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	free_isp_byr(vdev, vi_pipe);

	if (vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
		vb_release_block(vi_ctx->vi_raw_blk[0]);
		vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	}

	if (vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
		vb_release_block(vi_ctx->vi_raw_blk[1]);
		vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
	}

	return 0;
}

int vi_get_smooth_rawdump(struct sop_vi_dev *vdev, int vi_pipe, video_frame_info_s *frame_info, int millisec)
{
	int ret;
	struct isp_ctx *ctx = &vdev->ctx;
	struct raw_dump_info dump[2];
	int dev_id = 0, frm_num = 1, i = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	osal_memset(dump, 0, sizeof(dump));
	dump[0].raw_dump.raw_num = dump[1].raw_dump.raw_num = ctx->isp_pipe_cfg[vi_pipe].bind_raw;
	dump[0].time_out = dump[1].time_out = millisec;

	ret = isp_get_smooth_raw_dump(vdev, dump);
	if (ret != 0) {
		vi_pr(VI_ERR, "isp_smooth_raw_dump failed\n");
		return -1;
	}

	if (dump[0].is_b_not_rls) {
		vi_pr(VI_ERR, "Release pipe frame first, buffer not release.\n");
		return -1;
	}

	if (dump[0].is_timeout) {
		vi_pr(VI_ERR, "Get pipe frame time out(%d)\n", millisec);
		return -1;
	}

	if (dump[0].is_sig_int) {
		vi_pr(VI_ERR, "Get pipe frame signal interrupt\n");
		return -1;
	}

	if (vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_LINE ||
	    vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME ||
	    vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME_FULL_RATE)
		frm_num = 2;
	else
		frm_num = 1;

	for (; i < frm_num; i++) {
		frame_info[i].video_frame.phyaddr[0]  = dump[i].raw_dump.phy_addr;
		frame_info[i].video_frame.length[0]   = dump[i].raw_dump.size;
		frame_info[i].video_frame.stride[0]   = (ctx->is_dpcm_on)
							? 3 * UPPER(dump[i].src_w, 2)
							: 3 * UPPER(dump[i].src_w, 1);
		frame_info[i].video_frame.bayer_format  = vi_ctx->dev_attr[dev_id].bayer_format;
		frame_info[i].video_frame.compress_mode = vi_ctx->pipe_attr[0].compress_mode;
		frame_info[i].video_frame.width       = dump[i].src_w;
		frame_info[i].video_frame.height      = dump[i].src_h;
		frame_info[i].video_frame.offset_left  = dump[i].crop_x;
		frame_info[i].video_frame.offset_top   = dump[i].crop_y;
		frame_info[i].video_frame.time_ref     = dump[i].frm_num;
		frame_info[i].video_frame.pts          = dump[i].pts;

		vi_pr(VI_DBG, "Get paddr(0x%llx) size(%d) frm_num(%d) pts(%lld)\n",
			frame_info[i].video_frame.phyaddr[0],
			frame_info[i].video_frame.length[0],
			frame_info[i].video_frame.time_ref,
			frame_info[i].video_frame.pts);
	}

	return 0;
}

int vi_put_smooth_rawdump(struct sop_vi_dev *vdev, int vi_pipe, video_frame_info_s *frame_info)
{
	int ret;
	struct isp_ctx *ctx = &vdev->ctx;
	struct raw_dump_info dump[2];
	vb_blk vb;
	int frm_num, i, pipe;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	osal_memset(dump, 0, sizeof(dump));

	if (vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_LINE ||
	    vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME ||
	    vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME_FULL_RATE)
		frm_num = 2;
	else
		frm_num = 1;

	for (i = 0; i < frm_num; i++) {
		vb = vb_phys_addr2handle(frame_info[i].video_frame.phyaddr[0]);
		if (vb == VB_INVALID_HANDLE) {
			vi_pr(VI_ERR, "Can't get valid vb_blk.\n");
			return -1;
		}

		dump[i].raw_dump.phy_addr = frame_info[i].video_frame.phyaddr[0];
		vi_pr(VI_DBG, "Put paddr(0x%llx)\n", dump[i].raw_dump.phy_addr);

		pipe = dump[i].raw_dump.raw_num;
		dump[i].raw_dump.raw_num = ctx->isp_pipe_cfg[pipe].bind_raw;
	}

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	ret = isp_put_smooth_raw_dump(vdev, dump);
	if (ret != 0) {
		vi_pr(VI_ERR, "isp_put_smooth_raw_dump failed\n");
		osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);
		return -1;
	}

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

static int _vi_update_rotation_mesh(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, rotation_e rotation)
{
	struct gdc_mesh *pmesh = &vdev->mesh[vi_pipe][vi_chn];
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	osal_mutex_lock(&pmesh->lock);
	pmesh->paddr = 0x80000000;
	vi_ctx->rotation[vi_pipe][vi_chn] = rotation;
	osal_mutex_unlock(&pmesh->lock);

	return 0;
}

static int _vi_update_ldc_mesh(struct sop_vi_dev *vdev,
			       vi_pipe pipe,
			       vi_chn chn,
			       const vi_ldc_attr_s *ldc_attr,
			       u64 paddr)
{
	struct gdc_mesh *pmesh = &vdev->mesh[pipe][chn];
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	osal_mutex_lock(&pmesh->lock);
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;

	vi_ctx->ldc_attr[pipe][chn] = *ldc_attr;
	osal_mutex_unlock(&pmesh->lock);

	vi_pr(VI_DBG, "Pipe(%d) Chn(%d) mesh base(0x%llx)\n", pipe, chn, (unsigned long long)paddr);
	vi_pr(VI_DBG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d, rotation=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.x_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio,
			ldc_attr->attr.rotation);
	return 0;
}

int vi_set_chn_rotation(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, rotation_e rotation)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	vi_pr(VI_DBG, "Chn(%d) rotation(%d).\n", vi_chn, rotation);

	return _vi_update_rotation_mesh(vdev, vi_pipe, vi_chn, rotation);
}

int vi_get_chn_rotation(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, rotation_e *rotation)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_ctx, vi_pipe);
	if (ret != 0)
		return ret;

	*rotation = vi_ctx->rotation[vi_pipe][vi_chn];

	return ret;
}

int vi_set_chn_ldc_attr(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, const vi_ldc_attr_s *ldc_attr, u64 mesh_addr)
{
	return _vi_update_ldc_mesh(vdev, vi_pipe, vi_chn, ldc_attr, mesh_addr);
}

int vi_get_chn_ldc_attr(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, struct vi_chn_ldc_cfg *cfg)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	cfg->vi_pipe = vi_pipe;
	cfg->vi_chn = vi_chn;
	cfg->ldc_attr = vi_ctx->ldc_attr[vi_pipe][vi_chn];
	cfg->mesh_handle = vdev->mesh[vi_pipe][vi_chn].paddr;

	return ret;
}

int vi_set_chn_flip_mirror(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, struct vi_chn_flip_mirror_cfg *cfg)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_ctx, vi_chn);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_ctx->chn_attr[vi_pipe][vi_chn].flip = cfg->flip;
	vi_ctx->chn_attr[vi_pipe][vi_chn].mirror = cfg->mirror;

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return ret;
}

int vi_get_chn_flip_mirror(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, struct vi_chn_flip_mirror_cfg *cfg)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_ctx, vi_chn);
	if (ret != 0)
		return ret;

	cfg->flip = vi_ctx->chn_attr[vi_pipe][vi_chn].flip;
	cfg->mirror = vi_ctx->chn_attr[vi_pipe][vi_chn].mirror;

	return ret;
}

int vi_attach_vb_pool(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn, vb_pool vbp)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_ctx->chn_attr[vi_pipe][vi_chn].bind_vb_pool = vbp;

	vi_pr(VI_DBG, "Chn(%d) attach VbPool(%d)\n", vi_chn, vi_ctx->chn_attr[vi_pipe][vi_chn].bind_vb_pool);

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

int vi_detach_vb_pool(struct sop_vi_dev *vdev, int vi_pipe, int vi_chn)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->pipe_lock[vi_pipe]);

	vi_ctx->chn_attr[vi_pipe][vi_chn].bind_vb_pool = VB_INVALID_POOLID;

	vi_pr(VI_DBG, "Chn(%d) detach VbPool\n", vi_chn);

	osal_mutex_unlock(&vi_ctx->pipe_lock[vi_pipe]);

	return 0;
}

int vi_set_dev_rx_frame_count(struct sop_vi_dev *vdev, int dev, uint32_t frame_count)
{
	int ret = 0;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	enum sop_isp_raw raw_num = 0;
	struct isp_ctx *ctx = &vdev->ctx;

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&vi_ctx->dev_lock[dev]);

	//stop the device first
	raw_num = ctx->bind_raw[dev];
	ctx->isp_csi_cfg[raw_num].rx_ref_frm_num =
		vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] - frame_count;
	ctx->isp_csi_cfg[raw_num].rx_frm_cnt = frame_count;

	osal_mutex_unlock(&vi_ctx->dev_lock[dev]);

	return 0;
}

int vi_get_dev_rx_frame_count(struct sop_vi_dev *vdev, int dev, uint32_t *frame_count)
{
	int ret = 0;
	enum sop_isp_raw raw_num = 0;
	struct isp_ctx *ctx = &vdev->ctx;

	ret = check_vi_dev_valid(dev);
	if (ret != 0)
		return ret;

	raw_num = ctx->bind_raw[dev];
	*frame_count = ctx->isp_csi_cfg[raw_num].rx_frm_cnt;

	return 0;
}

void vi_sdk_release(struct sop_vi_dev *vdev)
{
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);
	uint8_t dev = 0, pipe = 0, chn = 0;

	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			vi_disable_chn(vdev, pipe, chn);
			vi_ctx->is_chn_enable[pipe][chn] = 0;
			vi_ctx->blk_size[pipe][chn] = 0;
		}
		vi_destroy_pipe(vdev, pipe);
	}

	for (dev = 0; dev < VI_MAX_DEV_NUM; dev++) {
		vi_disable_dev(vdev, dev);
	}

	vi_sw_deinit(vdev);

}

/*****************************************************************************
 *  SDK layer ioctl operations for vi.c
 ****************************************************************************/
long vi_sdk_ctrl(struct sop_vi_dev *vdev, struct vi_ctrl *ctrl)
{
	u32 id = ctrl->id;
	long rc = ERR_VI_INVALID_PARA;
	struct sop_vi_ctx *vi_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	switch (id) {
	case VI_SDK_GET_DEV_NUM:
	{
		ctrl->val = vi_ctx->total_dev_num;
		rc = 0;
		break;
	}
	case VI_SDK_ENABLE_PATGEN:
	{
		rc = vi_enable_patgen(vdev, ctrl->dev);
		break;
	}
	case VI_SDK_SET_DEV_ATTR:
	{
		vi_dev_attr_s *p_dev_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_attr_s));

		p_dev_attr = (vi_dev_attr_s *)ctrl->ptr;
		rc = vi_set_dev_attr(vdev, ctrl->dev, p_dev_attr);

		break;
	}
	case VI_SDK_GET_DEV_ATTR:
	{
		vi_dev_attr_s *p_dev_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_attr_s));

		p_dev_attr = (vi_dev_attr_s *)ctrl->ptr;
		rc = vi_get_dev_attr(vdev, ctrl->dev, p_dev_attr);

		break;
	}
	case VI_SDK_SET_DEV_ATTR_EX:
	{
		vi_dev_attr_ex_s *p_dev_attr_ex;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_attr_ex_s));

		p_dev_attr_ex = (vi_dev_attr_ex_s *)ctrl->ptr;
		rc = vi_set_dev_attr_ex(vdev, ctrl->dev, p_dev_attr_ex);

		break;
	}
	case VI_SDK_GET_DEV_ATTR_EX:
	{
		vi_dev_attr_ex_s *p_dev_attr_ex;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_attr_ex_s));

		p_dev_attr_ex = (vi_dev_attr_ex_s *)ctrl->ptr;
		rc = vi_get_dev_attr_ex(vdev, ctrl->dev, p_dev_attr_ex);

		break;
	}
	case VI_SDK_SET_DEV_BIND_ATTR:
	{
		vi_dev_bind_pipe_s *p_dev_bind_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_bind_pipe_s));

		p_dev_bind_attr = (vi_dev_bind_pipe_s *)ctrl->ptr;
		rc = vi_set_dev_bind_attr(vdev, ctrl->dev, p_dev_bind_attr);

		break;
	}
	case VI_SDK_GET_DEV_BIND_ATTR:
	{
		vi_dev_bind_pipe_s *p_dev_bind_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_bind_pipe_s));

		p_dev_bind_attr = (vi_dev_bind_pipe_s *)ctrl->ptr;
		rc = vi_get_dev_bind_attr(vdev, ctrl->dev, p_dev_bind_attr);

		break;
	}
	case VI_SDK_SET_DEV_UNBIND_ATTR:
	{
		vi_pr(VI_ERR, "not support now\n");
		break;
	}
	case VI_SDK_ENABLE_DEV:
	{
		rc = vi_enable_dev(vdev, ctrl->dev);
		break;
	}
	case VI_SDK_DISABLE_DEV:
	{
		rc = vi_disable_dev(vdev, ctrl->dev);
		break;
	}
	case VI_SDK_CREATE_PIPE:
	{
		vi_pipe_attr_s *p_pipe_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_attr_s));

		p_pipe_attr = (vi_pipe_attr_s *)ctrl->ptr;
		rc = vi_create_pipe(vdev, ctrl->pipe, p_pipe_attr);
		break;
	}
	case VI_SDK_DESTROY_PIPE:
	{
		rc = vi_destroy_pipe(vdev, ctrl->pipe);
		break;
	}
	case VI_SDK_SET_PIPE_ATTR:
	{
		vi_pipe_attr_s *p_pipe_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_attr_s));

		p_pipe_attr = (vi_pipe_attr_s *)ctrl->ptr;
		rc = vi_set_pipe_attr(vdev, ctrl->pipe, p_pipe_attr);
		break;
	}
	case VI_SDK_GET_PIPE_ATTR:
	{
		vi_pipe_attr_s *p_pipe_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_attr_s));

		p_pipe_attr = (vi_pipe_attr_s *)ctrl->ptr;
		rc = vi_get_pipe_attr(vdev, ctrl->pipe, p_pipe_attr);
		break;
	}
	case VI_SDK_START_PIPE:
	{
		rc = vi_start_pipe(vdev, ctrl->pipe);
		break;
	}
	case VI_SDK_STOP_PIPE:
	{
		rc = 0;
		break;
	}
	case VI_SDK_SET_CHN_ATTR:
	{
		vi_chn_attr_s *p_chn_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_chn_attr_s));

		p_chn_attr = (vi_chn_attr_s *)ctrl->ptr;
		rc = vi_set_chn_attr(vdev, ctrl->pipe, ctrl->chn, p_chn_attr);
		break;
	}
	case VI_SDK_GET_CHN_ATTR:
	{
		vi_chn_attr_s *p_chn_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_chn_attr_s));

		p_chn_attr = (vi_chn_attr_s *)ctrl->ptr;
		rc = vi_get_chn_attr(vdev, ctrl->pipe, ctrl->chn, p_chn_attr);
		break;
	}
	case VI_SDK_ENABLE_CHN:
	{
		rc = vi_sdk_enable_chn(vdev, ctrl->pipe, ctrl->chn);
		break;
	}
	case VI_SDK_DISABLE_CHN:
	{
		rc = vi_sdk_disable_chn(vdev, ctrl->pipe, ctrl->chn);
		break;
	}
	case VI_SDK_SET_BYPASS_FRM:
	{
		rc = vi_set_bypass_frm(vdev, ctrl->pipe, ctrl->val);
		break;
	}
	case VI_SDK_SET_PIPE_FRM_SRC:
	{
		vi_pipe_frame_source_e src;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_frame_source_e));

		src = *(vi_pipe_frame_source_e *)ctrl->ptr;
		rc = vi_set_pipe_frame_source(vdev, ctrl->pipe, src);
		break;
	}
	case VI_SDK_GET_PIPE_FRM_SRC:
	{
		vi_pipe_frame_source_e *p_src;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_frame_source_e));

		p_src = (vi_pipe_frame_source_e *)ctrl->ptr;
		rc = vi_get_pipe_frame_source(vdev, ctrl->pipe, p_src);
		break;
	}
	case VI_SDK_SET_DEV_TIMING_ATTR:
	{
		vi_dev_timing_attr_s *p_dev_timing_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_timing_attr_s));

		p_dev_timing_attr = (vi_dev_timing_attr_s *)ctrl->ptr;
		rc = vi_set_dev_timing_attr(vdev, ctrl->dev, p_dev_timing_attr);
		break;
	}
	case VI_SDK_GET_DEV_TIMING_ATTR:
	{
		vi_dev_timing_attr_s *p_dev_timing_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dev_timing_attr_s));

		p_dev_timing_attr = (vi_dev_timing_attr_s *)ctrl->ptr;
		rc = vi_get_dev_timing_attr(vdev, ctrl->dev, p_dev_timing_attr);
		break;
	}
	case VI_SDK_SEND_PIPE_RAW:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s));

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_send_pipe_raw(vdev, ctrl->pipe, p_frm_info);
		break;
	}
	case VI_SDK_GET_DEV_STATUS:
	{
		bool *status;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(bool));

		status = (bool *)ctrl->ptr;
		*status = vi_ctx->is_dev_enable[ctrl->dev];
		rc = 0;
		break;
	}
	case VI_SDK_GET_PIPE_STATUS:
	{
		vi_pipe_status_s *p_status;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_pipe_status_s));

		p_status = (vi_pipe_status_s *)ctrl->ptr;
		rc = vi_get_pipe_status(vdev, ctrl->pipe, p_status);
		break;
	}
	case VI_SDK_GET_CHN_STATUS:
	{
		vi_chn_status_s *p_status;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_chn_status_s));

		p_status = (vi_chn_status_s *)ctrl->ptr;
		rc = vi_get_chn_status(vdev, ctrl->pipe, ctrl->chn, p_status);
		break;
	}
	case VI_SDK_GET_CHN_FRAME:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s));

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_get_chn_frame(vdev, ctrl->pipe, ctrl->chn, p_frm_info, ctrl->reserved[1]);
		break;
	}
	case VI_SDK_RELEASE_CHN_FRAME:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s));

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_release_chn_frame(vdev, ctrl->pipe, ctrl->chn, p_frm_info);
		break;
	}
	case VI_SDK_SET_CHN_CROP:
	{
		vi_crop_info_s *p_chn_crop;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_crop_info_s));

		p_chn_crop = (vi_crop_info_s *)ctrl->ptr;
		rc = vi_set_chn_crop(vdev, ctrl->pipe, ctrl->chn, p_chn_crop);
		break;
	}
	case VI_SDK_GET_CHN_CROP:
	{
		vi_crop_info_s *p_chn_crop;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_crop_info_s));

		p_chn_crop = (vi_crop_info_s *)ctrl->ptr;
		rc = vi_get_chn_crop(vdev, ctrl->pipe, ctrl->chn, p_chn_crop);
		break;
	}
	case VI_SDK_SET_PIPE_CROP:
	{
		crop_info_s *p_crop_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(crop_info_s));

		p_crop_info = (crop_info_s *)ctrl->ptr;
		rc = vi_set_pipe_crop(vdev, ctrl->pipe, p_crop_info);
		break;
	}
	case VI_SDK_GET_PIPE_CROP:
	{
		crop_info_s *p_crop_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(crop_info_s));

		p_crop_info = (crop_info_s *)ctrl->ptr;
		rc = vi_get_pipe_crop(vdev, ctrl->pipe, p_crop_info);
		break;
	}
	case VI_SDK_GET_PIPE_FRAME:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s) * 2);

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_get_pipe_frame(vdev, ctrl->pipe, p_frm_info, ctrl->reserved[1]);
		break;
	}
	case VI_SDK_RELEASE_PIPE_FRAME:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s) * 2);

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_release_pipe_frame(vdev, ctrl->pipe, p_frm_info);
		break;
	}
	case VI_SDK_GET_PIPE_DUMP_ATTR:
	{
		vi_dump_attr_s *p_dump_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dump_attr_s));

		p_dump_attr = (vi_dump_attr_s *)ctrl->ptr;
		rc = vi_get_pipe_dump_attr(vdev, ctrl->pipe, p_dump_attr);
		break;
	}
	case VI_SDK_SET_PIPE_DUMP_ATTR:
	{
		vi_dump_attr_s *p_dump_attr;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(vi_dump_attr_s));

		p_dump_attr = (vi_dump_attr_s *)ctrl->ptr;
		rc = vi_set_pipe_dump_attr(vdev, ctrl->pipe, p_dump_attr);
		break;
	}
	case VI_SDK_START_SMOOTH_RAWDUMP:
	{
		struct sop_vip_isp_smooth_raw_param *param;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_vip_isp_smooth_raw_param));

		param = (struct sop_vip_isp_smooth_raw_param *)ctrl->ptr;
		rc = isp_start_smooth_raw_dump(vdev, param);

		break;
	}
	case VI_SDK_STOP_SMOOTH_RAWDUMP:
	{
		struct sop_vip_isp_smooth_raw_param *param;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct sop_vip_isp_smooth_raw_param));

		param = (struct sop_vip_isp_smooth_raw_param *)ctrl->ptr;
		rc = isp_stop_smooth_raw_dump(vdev, param);
		break;
	}
	case VI_SDK_GET_SMOOTH_RAWDUMP:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s) * 2);

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_get_smooth_rawdump(vdev, ctrl->pipe, p_frm_info, ctrl->reserved[1]);
		break;
	}
	case VI_SDK_PUT_SMOOTH_RAWDUMP:
	{
		video_frame_info_s *p_frm_info;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(video_frame_info_s) * 2);

		p_frm_info = (video_frame_info_s *)ctrl->ptr;
		rc = vi_put_smooth_rawdump(vdev, ctrl->pipe, p_frm_info);
		break;
	}
	case VI_SDK_SET_CHN_ROTATION:
	{
		struct vi_chn_rot_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_rot_cfg));

		p_cfg = (struct vi_chn_rot_cfg *)ctrl->ptr;
		rc = vi_set_chn_rotation(vdev, ctrl->pipe, ctrl->chn, p_cfg->rotation);
		break;
	}
	case VI_SDK_GET_CHN_ROTATION:
	{
		struct vi_chn_rot_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_rot_cfg));

		p_cfg = (struct vi_chn_rot_cfg *)ctrl->ptr;
		rc = vi_get_chn_rotation(vdev, ctrl->pipe, ctrl->chn, &p_cfg->rotation);

		break;
	}
	case VI_SDK_SET_CHN_LDC:
	{
		struct vi_chn_ldc_cfg *p_cfg;
		const vi_ldc_attr_s *ldc_attr = NULL;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_ldc_cfg));

		p_cfg = (struct vi_chn_ldc_cfg *)ctrl->ptr;
		ldc_attr = &p_cfg->ldc_attr;
		rc = vi_set_chn_ldc_attr(vdev, p_cfg->vi_pipe, p_cfg->vi_chn, ldc_attr, p_cfg->mesh_handle);
		break;
	}
	case VI_SDK_GET_CHN_LDC:
	{
		struct vi_chn_ldc_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_ldc_cfg));

		p_cfg = (struct vi_chn_ldc_cfg *)ctrl->ptr;
		rc = vi_get_chn_ldc_attr(vdev, ctrl->pipe, ctrl->chn, p_cfg);
		break;
	}
	case VI_SDK_SET_CHN_FLIP_MIRROR:
	{
		struct vi_chn_flip_mirror_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_flip_mirror_cfg));

		p_cfg = (struct vi_chn_flip_mirror_cfg *)ctrl->ptr;
		rc = vi_set_chn_flip_mirror(vdev, ctrl->pipe, ctrl->chn, p_cfg);
		break;
	}
	case VI_SDK_GET_CHN_FLIP_MIRROR:
	{
		struct vi_chn_flip_mirror_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_chn_flip_mirror_cfg));

		p_cfg = (struct vi_chn_flip_mirror_cfg *)ctrl->ptr;
		rc = vi_get_chn_flip_mirror(vdev, ctrl->pipe, ctrl->chn, p_cfg);
		break;
	}
	case VI_SDK_ATTACH_VB_POOL:
	{
		struct vi_vb_pool_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_vb_pool_cfg));

		p_cfg = (struct vi_vb_pool_cfg *)ctrl->ptr;
		rc = vi_attach_vb_pool(vdev, p_cfg->vi_pipe, p_cfg->vi_chn, (vb_pool)p_cfg->vb_pool);
		break;
	}
	case VI_SDK_DETACH_VB_POOL:
	{
		struct vi_vb_pool_cfg *p_cfg;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct vi_vb_pool_cfg));

		p_cfg = (struct vi_vb_pool_cfg *)ctrl->ptr;
		rc = vi_detach_vb_pool(vdev, p_cfg->vi_pipe, p_cfg->vi_chn);
		break;
	}
	case VI_SDK_DUMP_REGISTER:
	{
		struct ip_info *ip_info;
		void *json_vir = NULL;
		int size = 0;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(struct ip_info));

		ip_info = (struct ip_info *)ctrl->ptr;
		json_vir = osal_phys_to_virt(ip_info->phy_addr);

		//only update pipe tuning
		vi_set_tuning_dis(ctrl->pipe, 0, 0);

		osal_usleep_range(200 * 1000, 500 * 1000);

		//stop recivice csi
		osal_atomic_set(&vdev->is_drop, 0);

		osal_usleep_range(200 * 1000, 500 * 1000);

		rc = vi_dump_register(vdev, ctrl->pipe, json_vir, &size);
		if (size > ip_info->size) {
			rc = -1;
			vi_pr(VI_ERR, "size is too small.expect size(%d) but(%d\n", size, ip_info->size);
			goto err_sdk_dump_register;
		} else {
			vi_pr(VI_DBG, "size(%d) actual(%d\n", size, ip_info->size);
		}

		ip_info->size = size;

err_sdk_dump_register:

		vi_set_tuning_dis(0, 0, 0);

		osal_atomic_set(&vdev->is_drop, 0);
		break;
	}

	case VI_SDK_SET_DEV_RX_FRAME_COUNT:
	{
		u32 *p_frame_count;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(u32));

		p_frame_count = (u32 *)ctrl->ptr;
		rc = vi_set_dev_rx_frame_count(vdev, ctrl->dev, *p_frame_count);
		break;
	}

	case VI_SDK_GET_DEV_RX_FRAME_COUNT:
	{
		u32 *p_frame_count;

		CHK_STRUCT_SIZE(ctrl->size, sizeof(u32));

		p_frame_count = (u32 *)ctrl->ptr;
		rc = vi_get_dev_rx_frame_count(vdev, ctrl->dev, p_frame_count);
		break;
	}
	default:
		break;
	}

	return rc;
}

