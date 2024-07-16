#include <ut_common.h>

/*************************************************************
 *	Public	functions
 *************************************************************/
enum cvi_errno sclr_init(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno sclr_config(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno sclr_start(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno sclr_stop(void *priv)
{
	enum cvi_errno ret = ERR_NONE;

	return ret;
}

enum cvi_errno sclr_msgrcv(void *priv)
{
	enum cvi_errno ret = ERR_NONE;
	struct msgpack *msg = (struct msgpack *)priv;

	ut_pr(UT_INFO, "SCLR rev frm src(%s) val(%d)\n", msg->src_name, *(uint8_t *)msg->msg_ctx);

	return ret;
}

/*************************************************************
 *	Instance	Creation
 *************************************************************/
MODULE_DECL("sclr", sclr);

struct module_op *sclr_get_instance(void)
{
	return &sclr_op;
}

