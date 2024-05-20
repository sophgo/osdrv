#ifndef _VO_RGN_CTRL_H_
#define _VO_RGN_CTRL_H_

s32 vo_cb_get_rgn_hdls(VO_LAYER VoLayer, RGN_TYPE_E enType, RGN_HANDLE hdls[]);
s32 vo_cb_set_rgn_hdls(VO_LAYER VoLayer, RGN_TYPE_E enType, RGN_HANDLE hdls[]);
s32 vo_cb_set_rgn_cfg(VO_LAYER VoLayer, struct cvi_rgn_cfg *cfg);
// s32 vo_cb_set_rgn_coverex_cfg(VO_LAYER VoLayer, struct cvi_rgn_coverex_cfg *cfg);
s32 vo_cb_get_chn_size(VO_LAYER VoLayer, RECT_S *rect);

#endif /* _VO_RGN_CTRL_H_ */
