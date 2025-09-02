#include "tde_inter_cb.h"
#include "tde_debug.h"

int tde_do_op(struct tde_core *core, enum tde_usage_e usage
	, const void *usage_param, unsigned int width, unsigned int height
	, unsigned long long src_addr, unsigned long long dst_addr, unsigned char sync_io
	, unsigned char block, enum tde_task_mode_e task_mode)
{
        int ret;
        tde_handle handle;
        tde_surface_s src, dst;
        tde_rotate_angle_e angle;
        int pixel_size = 4; /* 4 bytes per pixel for ARGB8888 */

        handle = tde_begin_job(core);
        if (handle == TDE_INVALID_HANDLE) {
                TRACE_TDE(DBG_ERR, "Failed to begin TDE job.\n");
                return TDE_INVALID_HANDLE;
        }

        src.color_fmt = PIXEL_FORMAT_ARGB_8888; // Assuming ARGB8888 format
        src.width = ALIGN(width, TDE_ALIGN);
        src.height = ALIGN(height, TDE_ALIGN);
        src.stride = src.width * pixel_size;
        src.phy_addr = src_addr;

        dst.color_fmt = PIXEL_FORMAT_ARGB_8888; // Assuming ARGB8888 format
        dst.width = src.height;
        dst.height = src.width;
        dst.stride = dst.width * pixel_size;
        dst.phy_addr = dst_addr;

        if (task_mode == TDE_TASK_ROTATE_90) {
                angle = TDE_ROTATE_90;
        } else if (task_mode == TDE_TASK_ROTATE_270) {
                angle = TDE_ROTATE_270;
        } else {
                TRACE_TDE(DBG_ERR, "Unsupported task mode for rotation.\n");
                tde_cancel_job(core, handle);
                return ERR_TDE_ILLEGAL_PARAM;
        }

        ret = tde_rotate(core, handle, &src, &dst, angle);
        if (ret != 0) {
                TRACE_TDE(DBG_ERR, "TDE rotate failed, ret=%d.\n", ret);
                tde_cancel_job(core, handle);
                return ret;
        }

        ret = tde_end_job(core, handle, sync_io, block, 1000);
        if (ret != 0) {
                TRACE_TDE(DBG_ERR, "TDE end job failed, ret=%d.\n", ret);
                tde_cancel_job(core, handle);
                return ret;
        }

        return 0;
}