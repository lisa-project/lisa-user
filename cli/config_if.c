#include "config_if.h"

int eth_no;
int vlan_no;
int int_type;

static sw_command_t sh_eth[] = {
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

static sw_command_t sh_vlan[] = {
	{NULL,					0,	NULL,		NULL,				NA,			NULL,											NULL}
};

sw_command_root_t command_root_config_if_eth =			{"%s(config-if)%c",			sh_eth};
sw_command_root_t command_root_config_if_vlan =			{"%s(config-if)%c",			sh_vlan};
