#ifndef _CIF_L_H_
#define _CIF_L_H_

#define VIVO_INT_CSI_MAC0	      17
#define VIVO_INT_CSI_MAC1	      18
#define VIVO_INT_CSI_MAC2	      19

#define DPHY_TOP_BASE		(0x0A0A6000)
#define SENSOR_MAC0_BASE	(0x0A0A0000)
#define SENSOR_MAC1_BASE	(0x0A0A2000)


int driver_cif_init(void);
int driver_cif_exit(void);
long driver_cif_ioctl(unsigned int cmd, unsigned long arg);
#ifdef CONFIG_PROC_FS

extern int proc_cif_show(struct seq_file *m, void *v);

#endif

#endif
