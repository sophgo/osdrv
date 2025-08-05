#ifndef __TDE_IP_CTRL_H__
#define __TDE_IP_CTRL_H__


struct tde_buffer {
	u64 addr;
	u32 stride;
	u32 width;
	u32 height;
};

struct tde_line_attr {
	u32 start_x;
	u32 start_y;
	u32 end_x;
	u32 end_y;
	u32 color;
	u32 thick;
};


void tde_run_rotate90(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer);
void tde_run_rotate270(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer);
void tde_run_draw_line(struct tde_line_attr *attr, struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer);
void tde_run_copy(struct tde_buffer *src_buffer, struct tde_buffer *dst_buffer);

void tde_ip_init(void);
void tde_ip_deinit(void);
u32 tde_ip_clear_intr(void);
void tde_ip_reset(void);


#endif
