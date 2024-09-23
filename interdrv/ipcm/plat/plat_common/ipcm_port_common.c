
#include "ipcm_port_common.h"

MSGPROC_FN port_get_msg_fn(u32 msg_id, msg_proc_info *proc_info)
{
    u32 i = 0;
    if (proc_info == NULL) {
        ipcm_err("proc_info is null\n");
        return NULL;
    }
    for (i=0; i < proc_info->func_amount; i++) {
        if (msg_id == proc_info->table[i].msg_id) {
            return proc_info->table[i].func;
        }
    }
    ipcm_warning("msg_id:%u not register func, port id:%u\n", msg_id, proc_info->port_id);
    return NULL;
}
