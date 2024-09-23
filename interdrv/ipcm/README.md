
# linux driver 编译
# 按照正常project流程
#   1.source build/envsetup_soc.sh
#   2.defconfig cv1801c_wevb_0009a_spinor
#   3.build_kernel & build_osdrv
#   生成cvi_ipcm.ko


# linux userspace 编译
#   1.cd osdrv/interdrv/v2/ipcm/
#   2.mkdir cbuild;cd cbuild
#   3.cmake ..;make
#   生成libipcmuser.a和test

# alios 编译
#   1.build_alios
#

# sdk 说明
#   接口头文件
#       plat/include/ipcm_message.h
#       plat/include/ipcm_system.h
#       plat/include/ipcm_anonymous.h
#

# sample 运行
# alios端：(必须先运行alios端)
#   ipcm_test start
# linux端(kernel space for ipc msg)
#   (mount -t debugfs none /sys/kernel/debug/)
#   cd /sys/kernel/debug/ipcm  -- golden_v1.0.0分支
#   cd /proc/cvitek/ipcm  -- 主线分支
#   echo 1 > test
# linux端(user space for anon and system)
#   ./test
# 
# ----------------------------------------------------
# anonymous 内核态api测试
# insmod cvi_ipcm_test.ko
# 参考代码：osdrv/interdrv/v2/ipcm/test/linux/anon_kernel_test.c
#
# 说明：
# 1. alios端发送port id>=PORT_ANON_KER_ST的客制化msg会先送到kernel 处理，根据kernel handler返回值判断是否进一步给用户层处理；
# 2. alios端发送port id范围为PORT_ANON_ST到PORT_ANON_KER_ST之间的处理逻辑不变；
# 3. linux kernel可以支持发送port id>=PORT_ANON_KER_ST的msg
#
# 测试步骤：
# 1. alios端：ipcm_test start
# 2. linux端：insmod cvi_ipcm_test.ko   // 测试linux往alios发msg
# // 测试alios往linux发msg，其中5357633017835721621的16进制为0x4A5A22D801009395，打开ipcm log可以查看是否收到
# 3. alios端：ipcm_test sndmsg 5357633017835721621
#
#