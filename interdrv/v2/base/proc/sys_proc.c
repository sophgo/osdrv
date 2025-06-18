#include <linux/common.h>
#include <linux/comm_sys.h>
#include <linux/base_uapi.h>
#include "base_ctx.h"
#include "sys_proc.h"
#include "base_common.h"
#include "base_debug.h"

#define SYS_PROC_NAME			"sys"
#define SYS_PROC_PERMS			(0644)

static void *shared_mem;

extern bind_node_s bind_nodes[BIND_NODE_MAXNUM];

/*************************************************************************
 *	sys proc functions
 *************************************************************************/
static bool _is_fisrt_level_bind_node(bind_node_s *node)
{
	int i, j;
	bind_node_s *bindnodes;

	bindnodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindnodes[i].used) && (bindnodes[i].dsts.num != 0)
			&& !CHN_MATCH(&bindnodes[i].src, &node->src)) {
			for (j = 0; j < bindnodes[i].dsts.num; ++j) {
				if (CHN_MATCH(&bindnodes[i].dsts.mmf_chn[j], &node->src))
					// find other source in front of this node
					return false;
			}
		}
	}

	return true;
}

static bind_node_s *_find_next_bind_node(const mmf_chn_s *pstSrcChn)
{
	int i;
	bind_node_s *bindnodes;

	bindnodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindnodes[i].used) && CHN_MATCH(pstSrcChn, &bindnodes[i].src)
			&& (bindnodes[i].dsts.num != 0)) {
			return &bindnodes[i];
		}
	}

	return NULL; // didn't find next bind node
}

static void _show_sys_status(struct seq_file *m)
{
	int i, j, k;
	mmf_version_s *mmfversion;
	bind_node_s *bindnodes;
	mmf_chn_s *bind_chain[6];
	bind_node_s *binding_nodes[6];
	int next_dest_idex[6];
	int binding_level[6];
	int current_depth;
	bind_node_s *current_node;
	int current_dest_idex;
	int level;
	bind_node_s *next_node;

	for (j = 0; j < 6; j++) {
		bind_chain[j] = NULL;
		binding_nodes[j] = NULL;
		next_dest_idex[j] = 0;
		binding_level[j] = 0;
	}

	mmfversion = (mmf_version_s *)(shared_mem + BASE_VERSION_INFO_OFFSET);
	bindnodes = bind_nodes;

	seq_printf(m, "\nSys commit version:%s\n", GIT_OSDRV_COMMIT_HASH);
	seq_printf(m, "\nModule: [SYS], Version[%s], Build Time[%s]\n",
				mmfversion->version, UTS_VERSION);
	seq_puts(m, "-----BIND RELATION TABLE-----------------------------------------------------------------------------------------------------------\n");

	seq_printf(m, "%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
		"1stMod", "1stDev", "1stChn",
		"2ndMod", "2ndDev", "2ndChn",
		"3rdMod", "3rdDev", "3rdChn",
		"4thMod", "4thDev", "4thChn",
		"5thMod", "5thDev", "5thChn",
		"6thMod", "6thDev", "6thChn");

	for (i = 0; i < BIND_NODE_MAXNUM; i++) {
		//Check if the bind node is used / has destination / first level of bind chain
		if (!bindnodes[i].used || bindnodes[i].dsts.num == 0
			|| !_is_fisrt_level_bind_node(&bindnodes[i])) {
			continue;
		}

		current_depth = 0;
		for (j = 0; j < 6; j++) {
			bind_chain[j] = NULL;
			next_dest_idex[j] = 0;
			binding_level[j] = 0;
		}

		// init first node
		bind_chain[0] = &bindnodes[i].src;
		binding_nodes[0] = &bindnodes[i];
		next_dest_idex[0] = 0;
		binding_level[0] = 1;
		current_depth = 1;

		while (current_depth > 0) {
			current_node = binding_nodes[current_depth - 1];
			current_dest_idex = next_dest_idex[current_depth - 1];
			level = binding_level[current_depth - 1];

			if (current_dest_idex >= current_node->dsts.num) {
				current_depth--;
				if (current_depth > 0) {
					bind_chain[binding_level[current_depth - 1]] = NULL;
				}
				continue;
			}

			bind_chain[level] = &current_node->dsts.mmf_chn[current_dest_idex];
			next_dest_idex[current_depth - 1]++;

			// find next bind node
			next_node = _find_next_bind_node(bind_chain[level]);

			if (next_node != NULL && level < 5) {
				binding_nodes[current_depth] = next_node;
				next_dest_idex[current_depth] = 0;
				binding_level[current_depth] = level + 1;
				current_depth++;
			} else {
				for (k = 0; k < 6; k++) {
					if (bind_chain[k] != NULL) {
						seq_printf(m, "%-10s%-10d%-10d",
							sys_get_modname(bind_chain[k]->mod_id),
							bind_chain[k]->dev_id,
							bind_chain[k]->chn_id);
					} else {
						seq_printf(m, "%-10s%-10d%-10d", "null", 0, 0);
					}
				}
				seq_puts(m, "\n");
			}
		}
	}
	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static int _sys_proc_show(struct seq_file *m, void *v)
{
	_show_sys_status(m);
	return 0;
}

static int _sys_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _sys_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _sys_proc_fops = {
	.proc_open = _sys_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _sys_proc_fops = {
	.owner = THIS_MODULE,
	.open = _sys_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int sys_proc_init(struct proc_dir_entry *_proc_dir, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(SYS_PROC_NAME, SYS_PROC_PERMS, _proc_dir, &_sys_proc_fops, NULL) == NULL) {
		TRACE_BASE(DBG_ERR, "sys proc creation failed\n");
		rc = -1;
	}

	shared_mem = shm;
	return rc;
}

int sys_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(SYS_PROC_NAME, _proc_dir);
	shared_mem = NULL;

	return 0;
}
