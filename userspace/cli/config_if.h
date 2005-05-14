#ifndef _CONFIG_IF_H
#define _CONFIG_IF_H

#include "command.h"

extern sw_command_root_t command_root_config_if_eth;
extern sw_command_root_t command_root_config_if_vlan;

/* Selected eth interface when entering in interface configuration mode */
extern char sel_eth[];
/* Selected vlan interface when entering in interface configuration mode */
extern char sel_vlan[];

#endif
