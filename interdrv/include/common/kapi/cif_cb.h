#ifndef __CIF_CB_H__
#define __CIF_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

enum CIF_CB_CMD {
	CIF_CB_RESET_LVDS,
	CIF_CB_GET_CIF_ATTR,
	CIF_CB_MAX
};


struct cif_img_size_s {
	unsigned int	width;
	unsigned int	height;
	unsigned int	start_x;
	unsigned int	start_y;
	unsigned int	active_w;
	unsigned int	active_h;
	unsigned int	max_width;
	unsigned int	max_height;
};

struct cif_attr_s {
	unsigned int		devno;
	unsigned int		stagger_vsync;
	struct cif_img_size_s	img_size;
};

#ifdef __cplusplus
}
#endif

#endif /* __CIF_CB_H__ */
