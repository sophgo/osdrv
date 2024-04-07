#include "drv_file.h"


driver_file_t drv_fopen(const char *osal_file_tname, const char *mode)
{

    drv_file_t *cvi_fp = (drv_file_t *)
                       vmalloc(sizeof(drv_file_t));

    if (!cvi_fp)
        return NULL;

    if (!strncmp(mode, "rb", 2)) {
        cvi_fp->filep = filp_open(osal_file_tname, O_RDONLY/*|O_NONBLOCK*/, 0644);
    } else if (!strncmp(mode, "wb", 2)) {
        cvi_fp->filep = filp_open(osal_file_tname, O_RDWR | O_CREAT, 0644);
    }

    if (IS_ERR(cvi_fp->filep)) {
        vfree(cvi_fp);
        return NULL;
    }

    cvi_fp->old_fs = get_fs();

    return cvi_fp;
}

size_t drv_fwrite(const void *p, int size, int count, driver_file_t fp)
{
    drv_file_t *cvi_fp = (drv_file_t *)fp;
    struct file *filep = cvi_fp->filep;

    return kernel_write(filep, p, size * count, &filep->f_pos);
}

size_t drv_fread(void *p, int size, int count, driver_file_t fp)
{
    drv_file_t *cvi_fp = (drv_file_t *)fp;
    struct file *filep = cvi_fp->filep;

    return kernel_read(filep, p, size * count, &filep->f_pos);
}

long drv_ftell(driver_file_t fp)
{
    drv_file_t *cvi_fp = (drv_file_t *)fp;
    struct file *filep = cvi_fp->filep;

    return filep->f_pos;
}

int drv_fseek(driver_file_t fp, long offset, int origin)
{
    drv_file_t *cvi_fp = (drv_file_t *)fp;
    struct file *filep = cvi_fp->filep;

    return default_llseek(filep, offset, origin);
}

int drv_fclose(driver_file_t fp)
{
    drv_file_t *cvi_fp = (drv_file_t *)fp;
    struct file *filep;

    if (!fp)
        return -1;

    filep = cvi_fp->filep;
    filp_close(filep, 0);
    set_fs(cvi_fp->old_fs);
    vfree(cvi_fp);

    return 0;
}

