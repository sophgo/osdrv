#ifndef __BIND_H__
#define __BIND_H__

#include "common.h"
#include "comm_sys.h"

int32_t bind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn);
int32_t unbind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn);

int32_t bind_get_dst(mmf_chn_s *src_chn, mmf_bind_dest_s *bind_dest);
int32_t bind_get_src(mmf_chn_s *dest_chn, mmf_chn_s *src_chn);

void bind_init(void);
void bind_deinit(void);


#endif
