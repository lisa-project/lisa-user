/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#include "config_line.h"

static void cmd_end(FILE *out, char **argv) {
	cmd_root = &command_root_main;
}

static void cmd_exit(FILE *out, char **argv) {
	cmd_root = &command_root_config;
}

static void cmd_setpw(FILE *out, char **argv) {
	char *pw= NULL;

	while (*argv) {
		pw = *argv;
		argv++;
	}
	assert(pw);
	assert(strlen(pw) <= CLI_PASS_LEN);
	cfg_lock();
	strncpy(CFG->vty[0].passwd, pw, CLI_PASS_LEN);
	CFG->vty[0].passwd[CLI_PASS_LEN] = '\0';
	cfg_unlock();
}

static sw_command_t sh_line_pattern[] = {
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_regex,
		.func   = cmd_setpw,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) line password",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_pass[] = {
	{
		.name   = "0",
		.priv   = 15,
		.valid  = valid_0,
		.func   = NULL,
		.state  = PTCNT,
		.doc    = "Specifies an UNENCRYPTED password will follow",
		.subcmd = sh_line_pattern
	},
	{
		.name   = "LINE",
		.priv   = 15,
		.valid  = valid_regex,
		.func   = cmd_setpw,
		.state  = RUN,
		.doc    = "The UNENCRYPTED (cleartext) line password",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

static sw_command_t sh_line[] = {
	{
		.name   = "password",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Set a password",
		.subcmd = sh_pass
	},
	{
		.name   = "exit",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Exit from line configuration mode",
		.subcmd = NULL
	},
	{
		.name   = "end",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_end,
		.state  = RUN,
		.doc    = "End line configuration mode",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

sw_command_root_t command_root_config_line =		{"%s(config-line)%c",			sh_line};
