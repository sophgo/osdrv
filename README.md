# OSDRV编译指南

OSDRV（操作系统底层驱动）包含与硬件交互的底层驱动，将底层的操作系统驱动代码编译成目标平台可以使用的二进制文件，并确保这些驱动可以与操作系统、硬件设备以及其他系统组件正常协同工作。

```
// 环境搭建;
git clone -b sg200x-evb git@github.com:sophgo/sophpi.git
```

```
// 拉cv184x-v6.x 分支release代码;
./sophpi/scripts/repo_clone.sh --gitclone sophpi/scripts/subtree_cv184x-v6.x.xml
```

```
// 编译SDK;
source build/envsetup_soc.sh
defconfig cv184x
defconfig cv1842cp_wevb_0015a_spinor
clean_all && build_all
```

烧录整个固件包即可



## 许可证

本项目基于 [GPL-2.0](LICENSE) 许可证开源.

第三方库的文件中，包含如下许可证：
the BSD License：See extdrv/wireless/icommsemi/sv6115/tools/hostap_v2.10/README
Apache License 2.0：See http://www.apache.org/licenses/LICENSE-2.0
