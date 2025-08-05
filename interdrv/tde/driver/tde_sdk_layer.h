#ifndef __TDE_SDK_LAYER_H__
#define __TDE_SDK_LAYER_H__

#include "comm_tde.h"
#include "tde_core.h"

tde_handle tde_begin_job(struct tde_core *core);
int tde_end_job(struct tde_core *core, tde_handle handle, bool is_sync, bool is_block, unsigned int timeout);
int tde_wait_all_done(struct tde_core *core);
int tde_cancel_job(struct tde_core *core, tde_handle handle);
int tde_rotate(struct tde_core *core, tde_handle handle, tde_surface_s *src,
	tde_surface_s *dst, tde_rotate_angle_e angle);
int tde_draw_line(struct tde_core *core, tde_handle handle, tde_surface_s *src,
	tde_surface_s *dst, tde_line_s *line);
int tde_quick_copy(struct tde_core *core, tde_handle handle, tde_surface_s *src, tde_surface_s *dst);

#endif
