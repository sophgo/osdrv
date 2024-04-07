#ifndef __BIND_H__
#define __BIND_H__



int32_t bind_get_dst(MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest);
int32_t bind_get_src(MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn);

void bind_init(void);
void bind_deinit(void);
int32_t bind_set_cfg_user(unsigned long arg);
int32_t bind_get_cfg_user(unsigned long arg);


#endif
