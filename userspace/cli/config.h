#ifndef _CONFIG_H
#define _CONFIG_H

#include "command.h"

extern sw_command_t sh_conf_int[];
extern sw_command_root_t command_root_config;
extern sw_command_root_t command_root_config_vlan;

extern int valid_0(char *, char);
#endif
