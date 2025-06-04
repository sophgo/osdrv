
#include "ipcm_plat_adapter.h"
#include "ipcm_system.h"
#include "ipcm_test_common.h"

u32 g_test_size_buf[] = {32, 60, 64, 80, 120, 128, 200, 256, 300, 500, 512, 600, 800, 1000, 1024};
int g_test_size_cnt = sizeof(g_test_size_buf) / sizeof(u32);
