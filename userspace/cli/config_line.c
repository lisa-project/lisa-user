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
	strncpy(cfg->vty[0].passwd, pw, CLI_PASS_LEN);
	cfg->vty[0].passwd[CLI_PASS_LEN] = '\0';
}

static sw_command_t sh_line_pattern[] = {
	{"LINE",				15, valid_regex, cmd_setpw,	RUN,		"The UNENCRYPTED (cleartext) line password",		NULL},		
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_pass[] = {
	{"0",					15, valid_0,	NULL,			PTCNT,		"Specifies an UNENCRYPTED password will follow",	sh_line_pattern},
	{"LINE",				15, valid_regex, cmd_setpw,	RUN,		"The UNENCRYPTED (cleartext) line password",		NULL},		
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

static sw_command_t sh_line[] = {
	{"password",			15,	NULL,		NULL,			0,			"Set a password",									sh_pass},
	{"exit",				15,	NULL,		cmd_exit,		RUN,		"Exit from line configuration mode",				NULL},
	{"end",					15,	NULL,		cmd_end,		RUN,		"End line configuration mode",				NULL},
	{NULL,					0,	NULL,		NULL,			0,			NULL,												NULL}
};

sw_command_root_t command_root_config_line =		{"%s(config-line)%c",			sh_line};
