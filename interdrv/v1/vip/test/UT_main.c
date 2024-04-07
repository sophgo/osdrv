#include <ut_common.h>
#include "ut_instances.h"

/********************************************
 *	Global	Variables
 ********************************************/

#define MAX_MOD 64

static struct module_op *op[MAX_MOD];
static struct inparam input;
static uint8_t hooks_num;

/********************************************
 *	Private	Functions
 ********************************************/

static inline void usage(void)
{
	ut_pr(UT_INFO, "Run repeat numbers:");
	scanf("%d", (int *)&input.loop_num);

	ut_pr(UT_INFO, "Manual/auto command mode(1/0):");
	scanf("%d", (int *)&input.manual_mode);
}

static inline void reset_hooks(void)
{
	uint8_t i = 0;

	for (; i < MAX_MOD; i++)
		op[i] = NULL;

	ut_pr(UT_INFO, "UT reset hooks\n");
}

static inline void create_hooks(void)
{
#define GET_INSTANCE(x)\
	do {\
		op[hooks_num] = x##_get_instance();\
		op[hooks_num]->state = INIT;\
		memcpy(&(op[hooks_num]->incfg), &input, sizeof(input));\
		hooks_num++;\
	} while (0)

	//GET_INSTANCE(wrap_i2c);
	//GET_INSTANCE(cif);
	//GET_INSTANCE(snsr);
	GET_INSTANCE(isp);
	//GET_INSTANCE(sclr);
	//GET_INSTANCE(disp);

	ut_pr(UT_INFO, "UT create hooks(%d)\n", hooks_num);
}

/********************************************
 *	Public	Functions
 ********************************************/

enum cvi_errno ut_moduleCB(void *msg)
{
	enum cvi_errno ret = ERR_NONE;
	struct msgpack *msgpak = (struct msgpack *)msg;
	uint8_t i = 0;

	ut_pr(UT_INFO, "UT module CB send to dst(%s)\n", msgpak->dst_name);

	while ((op[i] != NULL) && strcmp(msgpak->dst_name, op[i]->name)) {
		i++;
	};

	if (i == hooks_num) {
		ut_pr(UT_ERR, "Err dst module name(%s)\n", msgpak->dst_name);
		ret = ERR_MSG_NO_DST;
	} else
		ret = op[i]->msgrcv(msg);

	return ret;
}

int main(int argc, char **argv)
{
	enum cvi_errno err = ERR_NONE;
	uint16_t i = 0, test_num = 0;

	usage();

	reset_hooks();
	create_hooks();

	do {
		switch (op[i]->state) {
		case INIT:
			err = op[i]->init((void *)&(op[i]->incfg));
			if (!err)
				op[i]->state = CONFIG;
			break;

		case CONFIG:
			err = op[i]->config((void *)&(op[i]->incfg));
			if (!err)
				op[i]->state = START;
			break;

		case START:
			err = op[i]->start((void *)&(op[i]->incfg));
			if (!err)
				op[i]->state = STOP;
			break;

		case STOP:
			err = op[i]->stop((void *)&(op[i]->incfg));

			if (!err) {
				op[i]->state = INIT;

				if (++i == hooks_num) {
					i = 0;
					test_num++;
				}
			}
			break;

		default:
			break;
		}
	} while (!err && (test_num < input.loop_num));

	if (err) {
		op[i]->stop((void *)&(op[i]->incfg));

		ut_pr(UT_ERR, "%s ERR Stage(%d), errno(0x%x)\n",
		      op[i]->name, op[i]->state, err);
	} else {
		for (i = 0; i < hooks_num; i++)
			ut_pr(UT_NOTICE, "%s UT PASS\n", op[i]->name);
	}
}

