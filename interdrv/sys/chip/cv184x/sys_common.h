#ifndef __SYS_COMMON_H_
#define __SYS_COMMON_H_

union top_reset {
	struct {
		u32 main_ap : 1;
		u32 second_ap : 1;
		u32 ddr : 1;
		u32 h264 : 1;
		u32 jpeg : 1;
		u32 h265 : 1;
		u32 vipsys : 1;
		u32 tdam : 1;
		u32 tpu : 1;
		u32 tpusys : 1;
		u32 tsm : 1;
		u32 usb : 1;
		u32 eth0 : 1;
		u32 eth1 : 1;
		u32 nand : 1;
		u32 emmc : 1;
		u32 sd0 : 1;
		u32 sd1 : 1;
		u32 sdma : 1;
		u32 i2s0 : 1;
		u32 i2s1 : 1;
		u32 i2s2 : 1;
		u32 i2s3 : 1;
		u32 uart0 : 1;
		u32 uart1 : 1;
		u32 uart2 : 1;
		u32 uart3 : 1;
		u32 i2c0 : 1;
		u32 i2c1 : 1;
		u32 i2c2 : 1;
		u32 i2c3 : 1;
		u32 i2c4 : 1;
	} b;
	u32 raw;
};


int sys_comm_set_base_addr(void *top, void *rtc);

#endif
