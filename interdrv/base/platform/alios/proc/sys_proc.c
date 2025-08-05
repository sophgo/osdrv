#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "base_common.h"
#include "base_ctx.h"


extern bind_node_s bind_nodes[BIND_NODE_MAXNUM];

/*************************************************************************
 *	sys proc functions
 *************************************************************************/
static bool _is_fisrt_level_bind_node(bind_node_s *node)
{
	int i, j;
	bind_node_s *bindNodes;

	bindNodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindNodes[i].used) && (bindNodes[i].dsts.num != 0)
			&& !CHN_MATCH(&bindNodes[i].src, &node->src)) {
			for (j = 0; j < bindNodes[i].dsts.num; ++j) {
				if (CHN_MATCH(&bindNodes[i].dsts.mmf_chn[j], &node->src))
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
	bind_node_s *bindNodes;

	bindNodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindNodes[i].used) && CHN_MATCH(pstSrcChn, &bindNodes[i].src)
			&& (bindNodes[i].dsts.num != 0)) {
			return &bindNodes[i];
		}
	}

	return NULL; // didn't find next bind node
}

static void _show_sys_status()
{
	int i, j, k;
	bind_node_s *bindNodes, *nextBindNode;
	mmf_chn_s *first, *second, *third;

	bindNodes = bind_nodes;

	printf("-----BIND RELATION TABLE-----------------------------------------------------------------------------------------------------------\n");
	printf("%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
		"1stMod", "1stDev", "1stChn", "2ndMod", "2ndDev", "2ndChn", "3rdMod", "3rdDev", "3rdChn");

	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		//Check if the bind node is used / has destination / first level of bind chain
		if ((bindNodes[i].used) && (bindNodes[i].dsts.num != 0)
			&& (_is_fisrt_level_bind_node(&bindNodes[i]))) {

			first = &bindNodes[i].src; //bind chain first level

			for (j = 0; j < bindNodes[i].dsts.num; ++j) {
				second = &bindNodes[i].dsts.mmf_chn[j]; //bind chain second level

				nextBindNode = _find_next_bind_node(second);
				if (nextBindNode != NULL) {
					for (k = 0; k < nextBindNode->dsts.num; ++k) {
					third = &nextBindNode->dsts.mmf_chn[k]; //bind chain third level
					printf("%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						sys_get_modname(first->mod_id), first->dev_id, first->chn_id,
						sys_get_modname(second->mod_id), second->dev_id, second->chn_id,
						sys_get_modname(third->mod_id), third->dev_id, third->chn_id);
					}
				} else { //level 3 node not found
					printf("%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						sys_get_modname(first->mod_id), first->dev_id, first->chn_id,
						sys_get_modname(second->mod_id), second->dev_id, second->chn_id,
						"null", 0, 0);
				}
			}
		}
	}
	printf("\n-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static void sys_proc_show(int32_t argc, char **argv)
{
	_show_sys_status();
}

ALIOS_CLI_CMD_REGISTER(sys_proc_show, proc_sys, sys info);

