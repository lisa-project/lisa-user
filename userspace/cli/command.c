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

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/param.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>

#include "command.h"
#include "climain.h"
#include "config.h"
#include "build_config.h"
#include "filter.h"
#include "if.h"
#include "shared.h"
#include "ip.h"
#include "list.h"
#include "cdp.h"

int console_session = 0;

static char if_name[IFNAMSIZ];

static void show_ip(FILE *, char *);

static void swcli_sig_term(int sig) {
	char hostname[MAX_HOSTNAME];
	cmd_root = &command_root_main;
	signal(SIGTSTP, SIG_IGN);
	printf("^Z\n");
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	sprintf(prompt, cmd_root->prompt, hostname, priv ? '#' : '>');
	rl_set_prompt(prompt);
	rl_forced_update_display();
}

/* Command Handlers implementation */
static void cmd_disable(FILE *out, char **argv) {
	int req;
	
	req = argv[0] == NULL ? 1 : atoi(argv[0]);
	if (req > priv) {
		fprintf(out, 
				"New privilege level must be less than current"
				"privilege level\n");
		return;
	}
	priv = req;
}

static int secret_validator(char *pass, void *arg) {
	char *secret = arg;

	return !strcmp(secret, crypt(pass, secret));
}

static void cmd_enable(FILE *out, char **argv) {
	int req;
	int fail = 0;
	char secret[CLI_SECRET_LEN + 1];

	req = argv[0] == NULL ? 15 : atoi(argv[0]);
	if(req > priv) {
		fail = 1;
		cfg_lock();
		strcpy(secret, CFG->enable_secret[req]);
		cfg_unlock();
		if(secret[0] == '\0') {
			if(console_session) {
				priv = req;
				return;
			}
			fprintf(out, "%% No password set\n");
			return;
		}
		fail = !cfg_checkpass(3, secret_validator, secret);
	}
	if(fail) {
		fprintf(out, "%% Bad secrets\n\n");
		return;
	}
	priv = req;
}

static void cmd_conf_t(FILE *out, char **argv) {
	cmd_root = &command_root_config;
	printf("Enter configuration commands, one per line.  End with CNTL/Z.\n");
	signal(SIGTSTP, swcli_sig_term);
}

void cmd_help(FILE *out, char **argv) {
	fprintf(out,
		"Help may be requested at any point in a command by entering\n"
		"a question mark '?'.  If nothing matches, the help list will\n"
		"be empty and you must backup until entering a '?' shows the\n"
		"available options.\n"
		"Two styles of help are provided:\n"
		"1. Full help is available when you are ready to enter a\n"
		"   command argument (e.g. 'show ?') and describes each possible\n"
		"   argument.\n"
		"2. Partial help is provided when an abbreviated argument is entered\n"
		"   and you want to know what arguments match the input\n"
		"   (e.g. 'show pr?'.)\n\n"
		);
}

void cmd_show_ver(FILE *out, char **argv) {
	FILE *fp;
	char buf[255];
	
	fprintf(out,
		"LiSA Command Line Interface, ver. 1.0, compiled 2005.06.01 \n"
		"For more info go to http://lisa.ines.ro\n\n"
		"This tool is part of the Linux Switch Appliance Project and can\n"
		"be used (succesfully) only with a kernel patched with the LMS\n"
		"(i.e. Linux Multilayer Switch) kernel patch.\n\n"
		"Operating system info:\n\n"
			);
	fp = fopen(VERSION_FILE_PATH, "r");
	if (!fp) {
		fprintf(out, "No available information.\n\n");
		return;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		fprintf(out, "%s\n", buf);
	}
	fclose(fp);
	fprintf(out, "\n");
}

static void cmd_history(FILE *out, char **argv) {
	HIST_ENTRY **history;
	HIST_ENTRY *entry;
	int i;

	history = history_list();
	if (history) {
		for (i=0; (entry = history[i]); i++) {
			fprintf(out, "   %s\n", entry->line);
		}
	}
}

static void cmd_exit(FILE *out, char **argv) {
	cdp_destroy_ipc(&cdp_s);
	pclose(out);
	exit(0);
}

static void cmd_run_eth(FILE *out, char **argv) {
	char *arg = *argv;
	int status;
	FILE *tmp = NULL;
	char tmp_name[MAXPATHLEN];

	fprintf(out, "Building configuration...\n");
	fflush(out);
	do {
		tmp = mk_tmp_stream(tmp_name, "w+");
		if(tmp == NULL)
			break;
		status = build_int_eth_config(tmp, arg, 1);
		if(status)
			break;
		fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(tmp));
		rewind(tmp);
		copy_data(out, tmp);
		fclose(tmp);
		unlink(tmp_name);
		fprintf(out, "end\n\n");
		fflush(out);
		return;
	} while(0);
	if(tmp != NULL) {
		fclose(tmp);
		unlink(tmp_name);
	}
	fprintf(out, "Command rejected: device not present\n");
	return;
}

static void cmd_show_priv(FILE *out, char **argv) {
	fprintf(out, "Current privilege level is %d\n", priv);
	fflush(out);
}

static void cmd_show_start(FILE *out, char **argv) {
	char buf[1024];
	FILE *fp;

	if (!(fp = fopen(config_file, "r"))) {
		perror("fopen");
		return;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		buf[sizeof(buf)-1] = '\0';
		fprintf(out, buf);
	}
	fclose(fp);
}

static void cmd_show_run(FILE *out, char **argv) {
	int status;
	FILE *tmp = NULL;
	char tmp_name[MAXPATHLEN];

	fprintf(out, "Building configuration...\n");
	fflush(out);
	do {
		tmp = mk_tmp_stream(tmp_name, "w+");
		if(tmp == NULL)
			break;
		status = build_config(tmp, 1);
		if(status)
			break;
		fprintf(out, "\nCurrent configuration : %ld bytes\n", ftell(tmp));
		rewind(tmp);
		copy_data(out, tmp);
		fclose(tmp);
		unlink(tmp_name);
		fprintf(out, "!\nend\n\n");
		fflush(out);
		return;
	} while(0);
	if(tmp != NULL) {
		fclose(tmp);
		unlink(tmp_name);
	}
	fprintf(out, "Oops! Something went terribly wrong :(\n");
	return;
}

static void cmd_run_vlan(FILE *out, char **argv) {
	char *dev = NULL;
	if (*argv)
		dev = if_name_vlan(*argv);
	show_ip(out, dev);
}

static void init_mac_filter(struct swcfgreq *user_arg) {
	user_arg->ifindex = NULL;
	user_arg->cmd = SWCFG_GETMAC;
	memset(&user_arg->ext.marg.addr, 0, ETH_ALEN);
	user_arg->ext.marg.addr_type = SW_FDB_ANY;
	user_arg->vlan = 0;
}

static void do_mac_filter(FILE *out, struct swcfgreq *user_arg, int size, char *buf) {
	int status;

	do {
		user_arg->ext.marg.buf_size = size;
		user_arg->ext.marg.buf = buf;
		status = ioctl(sock_fd, SIOCSWCFG, user_arg);
		if (status == -1) {
			if (errno == ENOMEM) {
				dbg("Insufficient buffer space. Realloc'ing ...\n");
				buf = realloc(buf, size+INITIAL_BUF_SIZE);
				assert(buf);
				size+=INITIAL_BUF_SIZE;
				continue;
			}
			perror("ioctl");
			return;
		}
	} while(status < 0);
	user_arg->ext.marg.actual_size = status;
	cmd_showmac(out, (char *)user_arg);
}

static void cmd_show_mac(FILE *out, char **argv) {
	int i, size;
	char *arg, *buf;
	struct swcfgreq user_arg;

	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);
	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
		}
	}
	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_mac_eth(FILE *out, char **argv) {
	char *arg, *buf;
	struct swcfgreq user_arg;
	int i, size;


	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);

	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
			continue;
		}
		if (valid_eth(arg, arg[strlen(arg)-1]) && !user_arg.ifindex) {
			user_arg.ifindex = if_name_eth(arg);
		}
	}

	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_mac_vlan(FILE *out, char **argv) {
	char *arg, *buf;
	struct swcfgreq user_arg;
	int i, size, a[2], cnt=0, k;

	
	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);
	
	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
			continue;
		}
		if (sscanf(arg, "%d", &k)) {
			a[cnt++] = k;
		}
	}
	if (cnt > 1) { /* si eth si vlan */
		sprintf(if_name, "eth%d", a[0]);
		user_arg.ifindex = if_name;
		user_arg.vlan = a[1];
	}
	else user_arg.vlan = a[0];

	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_sh_addr(FILE *out, char **argv) {
	char *arg, *buf;
	struct swcfgreq user_arg;
	int i, size;
	
	buf = (char *)malloc(INITIAL_BUF_SIZE);
	size = INITIAL_BUF_SIZE;
	init_mac_filter(&user_arg);
	
	for (i=0; (arg = argv[i]); i++) {
		if (strcmp(arg, "dynamic") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_DYN;
			continue;
		}
		if (strcmp(arg, "static") == 0) {
			user_arg.ext.marg.addr_type = SW_FDB_STATIC;
			continue;
		}
		if (valid_mac(arg, ' ')) {
			parse_mac(arg, user_arg.ext.marg.addr);
		}
	}
	
	do_mac_filter(out, &user_arg, size, buf);
	fprintf(out, "\n");
}

static void cmd_ping(FILE *out, char **argv) {
	FILE *p;
	char cmd_buf[MAX_LINE_WIDTH];
	int nc;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	nc = sprintf(cmd_buf, "%s -i %d -c %d %s", PING_PATH, PING_INTERVAL, PING_COUNT, argv[0]);
	assert(nc < sizeof(cmd_buf));
	assert((p =	popen(cmd_buf, "w")));
	pclose(p);
}

static void cmd_trace(FILE *out, char **argv) {
	FILE *p;
	char cmd_buf[MAX_LINE_WIDTH];
	int nc;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	nc = sprintf(cmd_buf, "%s %s", TRACEROUTE_PATH, argv[0]);
	assert(nc < sizeof(cmd_buf));
	assert((p =	popen(cmd_buf, "w")));
	pclose(p);
}

static void cmd_clr_mac_eth(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	int status;
	unsigned char mac[ETH_ALEN];

	ioctl_arg.cmd = SWCFG_DELMACDYN;
	ioctl_arg.vlan = 0;
	memset(mac, 0, ETH_ALEN);
	ioctl_arg.ext.mac = mac;
	ioctl_arg.ifindex = if_name_eth(argv[0]);
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status == -1) {
		fprintf(out, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(out);
	}
}

static void cmd_clr_mac(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	int status;
	unsigned char mac[ETH_ALEN];

	ioctl_arg.cmd = SWCFG_DELMACDYN;
	memset(mac, 0, ETH_ALEN);
	ioctl_arg.ext.mac = mac;
	ioctl_arg.vlan = 0;
	ioctl_arg.ifindex = NULL;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status == -1) {
		fprintf(out, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(out);
	}
}

static void cmd_clr_mac_vl(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	int status;
	unsigned char mac[ETH_ALEN];

	ioctl_arg.cmd = SWCFG_DELMACDYN;
	ioctl_arg.vlan = parse_vlan(argv[0]);
	memset(mac, 0, ETH_ALEN);
	ioctl_arg.ext.mac = mac;
	ioctl_arg.ifindex = NULL;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status == -1) {
		fprintf(out, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(out);
	}
}

static void cmd_clr_mac_adr(FILE *out, char **argv) {
	struct swcfgreq ioctl_arg;
	int status;
	unsigned char mac[ETH_ALEN];

	ioctl_arg.cmd = SWCFG_DELMACDYN;
	status = parse_mac(argv[0], mac);
	assert(!status);
	ioctl_arg.ext.mac = mac;
	ioctl_arg.vlan = 0;
	ioctl_arg.ifindex = NULL;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status == -1) {
		fprintf(out, "MAC address could not be removed\n"
				"Address not found\n\n");
		fflush(out);
	}
}

static void show_ip_list(FILE *out, struct list_head *ipl) {
	struct ip_addr_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, ipl, lh) {
		fprintf(out, "  inet: %s / %s\n", 
				entry->inet, entry->mask);
		list_del(&entry->lh);
		free(entry);
	}
}

static void show_ip(FILE *out, char *dev) {
	FILE *fh;
	char buf[128];
	struct list_head *ipl;

	fh = fopen(PROCNETSWITCH_PATH, "r");
	if (!fh) {
		perror("fopen");
		return;
	}
	while (fgets(buf, sizeof(buf), fh)) {
		/* make sure buf is null-terminated */
		buf[sizeof(buf)-1] = '\0';
		/* strip the newline at the end of buf */
		if (strlen(buf))
			buf[strlen(buf)-1] = '\0'; 
		/* compare to the interface name we're searching for */
		if (dev && strcmp(dev, buf))
			continue;
		ipl = list_ip_addr(buf, 0);
		if (ipl && !list_empty(ipl)) {
			fprintf(out, "%s:\n", buf);
			show_ip_list(out, ipl);
		}
		if (ipl)
			free(ipl);
		if (dev)
			break;	
	}
	fclose(fh);
}

static void cmd_sh_ip(FILE *out, char **argv) {
	char *dev = NULL;
	if (*argv)
		dev = if_name_vlan(*argv);
	show_ip(out, dev);
}

/* FIXME: quick hack
   ar tb facut ca la show mac (selectori id si name) 
 */
static void cmd_show_vlan(FILE *out, char **argv) {
	FILE *in;
	char buf[512];

	fprintf(out, "\n");
	if ((in = fopen(VLAN_FILE_PATH, "r")) == NULL) {
		perror("fopen");
		return;
	}
	
	while (fgets(buf, sizeof(buf), in)) {
		buf[strlen(buf)-1] = '\0';
		fprintf(out, "%s\n", buf);
	}

	fclose(in);
}

int get_mac_age() {
	struct swcfgreq ioctl_arg;
	int status;

	ioctl_arg.cmd = SWCFG_GETAGETIME;
	status = ioctl(sock_fd, SIOCSWCFG, &ioctl_arg);
	if (status) { 
		perror("Error getting age time");
		return -1;
	}	
	return ioctl_arg.ext.nsec;
}

static void cmd_sh_mac_age(FILE *out, char **argv) {
	long age;

	if ((age = get_mac_age()) >= 0) {
		fprintf(out, "%li\n", age);
	}
}

static void cmd_wrme(FILE *out, char **argv) {
	int status;
	FILE *tmp = NULL;

	fprintf(out, "Building configuration...\n");
	fflush(out);
	cfg_lock();
	do {
		if (!(tmp = fopen(config_file, "w+"))) {
			perror("fopen");
			break;
		}
		status = build_config(tmp, 0);
		if(status) {
			fclose(tmp);
			break;
		}
		fprintf(out, "Current configuration : %ld bytes\n", ftell(tmp));
		fclose(tmp);
		sync();
		fprintf(out, "\n[OK]\n\n");
	} while(0);
	cfg_unlock();
}

static void cmd_reload(FILE *out, char **argv) {
	int key;

	fputs("Proceed with reload? [confirm]", out);
	fflush(out);
	key = read_key();
	if(key != 'y' && key != 'Y' && key != '\n') {
		fputc('\n', out);
		return;
	}
	system("reboot &> /dev/null");
	/* delay returning so that the prompt does'n get displayed again */
	sleep(5);
}


/* Validation Handlers Implementation */
int valid_regex(char *arg, char lookahead) {
	return 1;
}

int valid_eth(char *arg, char lookahead) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 0 || no > 7) /* FIXME max value */
		return 0;
	/* FIXME interfata e in switch */
	return 1;
}

int valid_vlan(char *arg, char lookahead) {
	int no;
	if(sscanf(arg, "%d", &no) != 1)
		return 0;
	if(no < 1 || no > 4094)
		return 0;
	return 1;
}

int valid_mac(char *arg, char lookahead) {
	regex_t regex;
	regmatch_t result;
	int res;

	if (regcomp(&regex, whitespace(lookahead)? 
				"^[0-9a-f]{1,4}(\\.[0-9a-f]{1,4}){2}$":"^[0-9a-f]{1,4}(\\.[0-9a-f]{0,4}){0,2}$", 
				REG_ICASE|REG_EXTENDED)) {
		perror("regcomp");
		return 0;
	}
	res = regexec(&regex, arg, 1, &result, 0);
	return (res == 0);
}

int valid_dyn(char *arg, char lookahead) {
	return (strcmp(arg, "dynamic") == 0);
}

int valid_static(char *arg, char lookahead) {
	return (strcmp(arg, "static") == 0);
}

int valid_star(char *arg, char lookahead) {
	return (strcmp(arg, "*") == 0);
}

int valid_protocol(char *arg, char lookahead) {
	return (arg && strlen(arg) && strncmp(arg, "protocol", strlen(arg)) == 0);
}

int valid_version(char *arg, char lookahead) {
	return (arg && strlen(arg) && strncmp(arg, "version", strlen(arg)) == 0);
}

char vlan_range[] = "<1-1094>\0";

int parse_eth(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);

	return no;
}

char *if_name_eth(char *arg) {
	sprintf(if_name, "eth%d", parse_eth(arg));
	return if_name;
}

char *if_name_vlan(char *arg) {
	sprintf(if_name, "vlan%d", parse_vlan(arg));
	return if_name;
}

int parse_vlan(char *arg) {
	int no, status;

	status = sscanf(arg, "%d", &no);
	assert(status == 1);
	assert(no >= 1 && no <= 4094);
	return no;
}

static char vlan_name[16];
char *default_vlan_name(int vlan) {
	snprintf(vlan_name, sizeof(vlan_name), "VLAN%04d", vlan);
	return vlan_name;
}

int parse_mac(char *arg, unsigned char *mac) {
	unsigned short a0, a1, a2;

	if (sscanf(arg, "%hx.%hx.%hx", &a0, &a1, &a2) != ETH_ALEN/2)
		return EINVAL;

	mac[0] = a0 / 0x100;
	mac[1] = a0 % 0x100;
	mac[2] = a1 / 0x100;
	mac[3] = a1 % 0x100;
	mac[4] = a2 / 0x100;
	mac[5] = a2 % 0x100;

	return 0;
}

int valid_priv(char *arg, char lookahead) {
	int n;

	n = atoi(arg);
	if(n >= 1 && n <= 15)
		return 1;
	return 0;
}

/* Pipe regex (the argument for pipe modifiers) */
static sw_command_t sh_pipe_regex[] = {
	{
		.name   = "LINE",
		.priv   = 1,
		.valid  = valid_regex,
		.func   = NULL,
		.state  = RUN,
		.doc    = "Regular Expression",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* Pipe modifiers (begin, exclude, include, grep) menu node */
static sw_command_t sh_pipe_mod[] = {
	{
		.name   = "begin",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = MODE_BEGIN,
		.doc    = "Begin with the line that matches",
		.subcmd = sh_pipe_regex
	},
	{
		.name   = "exclude",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = MODE_EXCLUDE,
		.doc    = "Exclude lines that match",
		.subcmd = sh_pipe_regex
	},
	{
		.name   = "include",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = MODE_INCLUDE,
		.doc    = "Include lines that match",
		.subcmd = sh_pipe_regex
	},
	{
		.name   = "grep",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = MODE_GREP,
		.doc    = "Linux grep functionality",
		.subcmd = sh_pipe_regex
	},
	SW_COMMAND_LIST_END
};

/* Pipe menu node (can be accessed from many places) */
sw_command_t sh_pipe[] = {
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show interfaces ethernet ...# menu node */
static sw_command_t sh_int_eth[] = {
	/* #show interfaces ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 1,
		.valid  = valid_eth,
		.func   = cmd_int_eth,
		.state  = RUN,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show interfaces vlan ...# menu node */
static sw_command_t sh_int_vlan[] = {
	/* #show interfaces vlan <1-1094># */
	{
		.name   = vlan_range,
		.priv   = 1,
		.valid  = valid_vlan,
		.func   = cmd_int_vlan,
		.state  = RUN,
		.doc    = "Vlan interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show running-config interface ethernet ...# menu node */
static sw_command_t sh_run_eth[] = {
	/* #show running-config interface ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 2,
		.valid  = valid_eth,
		.func   = cmd_run_eth,
		.state  = RUN,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show running-config interface vlan ...# menu node */
static sw_command_t sh_run_vlan[] = {
	/* #show running-config interface vlan <1-1094># */
	{
		.name   = vlan_range,
		.priv   = 2,
		.valid  = valid_vlan,
		.func   = cmd_run_vlan,
		.state  = RUN,
		.doc    = "Vlan interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table [dynamic] vlan ...# menu node */
static sw_command_t sh_mac_vlan[] = {
	/* #show mac-address-table [dynamic] vlan <1-1094># */
	{
		.name   = vlan_range,
		.priv   = 1,
		.valid  = valid_vlan,
		.func   = cmd_sh_mac_vlan,
		.state  = RUN|PTCNT,
		.doc    = "Vlan interface number",
		.subcmd = sh_pipe
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table interface ethernet <0-7> ...# menu node */
static sw_command_t sh_sh_m_eth[] = {
	/* #show mac-address-table interface ethernet <0-7> vlan# */
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "VLAN keyword",
		.subcmd = sh_mac_vlan
	},
	/* #show mac-address-table interface ethernet <0-7> |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table interface ethernet ...# menu node */
static sw_command_t sh_mac_eth[] = {
	/* #show mac-address-table interface ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 0,
		.valid  = valid_eth,
		.func   = cmd_sh_mac_eth,
		.state  = RUN|PTCNT,
		.doc    = "Ethernet interface number",
		.subcmd = sh_sh_m_eth
	},
	SW_COMMAND_LIST_END
};

/* #show interfaces ...# menu node */
static sw_command_t sh_sh_int[] = {
	/* #show interfaces ethernet# */
	{
		.name   = "ethernet",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_int_eth
	},
	/* #show interfaces vlan# */
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "LMS Vlans",
		.subcmd = sh_int_vlan
	},
	SW_COMMAND_LIST_END
};

/* #show cdp interface ethernet ...# menu node */
static sw_command_t sh_cdp_int_eth[] = {
	/* #show cdp interface ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 1,
		.valid  = valid_eth,
		.func   = cmd_sh_cdp_int,
		.state  = RUN,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show cdp neighbors ethernet <0-7> ...# menu node */
static sw_command_t sh_cdp_ne_det[] = {
	/* #show cdp neighbors ethernet <0-7> detail# */
	{
		.name   = "detail",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_ne_detail,
		.state  = RUN,
		.doc    = "Show detailed information",
		.subcmd = sh_pipe
	},
	SW_COMMAND_LIST_END
};

/* #show cdp neighbors ethernet ...# menu node */
static sw_command_t sh_cdp_ne_eth[] = {
	/* #show cdp neigbors ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 1,
		.valid  = valid_eth,
		.func   = cmd_sh_cdp_ne_int,
		.state  = RUN|PTCNT,
		.doc    = "Ethernet interface number",
		.subcmd = sh_cdp_ne_det
	},
	SW_COMMAND_LIST_END
};

/* #show cdp interface ...# menu node */
static sw_command_t sh_cdp_int[] = {
	/* #show cdp interface ethernet# */
	{
		.name   = "ethernet",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_cdp_int_eth
	},
	/* #show cdp interface |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show cdp entry * version ...# menu node */
static sw_command_t sh_cdp_entry_ve[] = {
	/* #show cdp entry * version protocol# */
	{
		.name   = "protocol",
		.priv   = 1,
		.valid  = valid_protocol,
		.func   = cmd_sh_cdp_entry,
		.state  = RUN|PTCNT|CMPL,
		.doc    = "Protocol information",
		.subcmd = sh_pipe
	},
	/* #show cdp entry * version |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show cdp entry * ...# menu node */
static sw_command_t sh_sh_cdp_entry[] = {
	/* #show cdp entry * protocol# */
	{
		.name   = "protocol",
		.priv   = 1,
		.valid  = valid_protocol,
		.func   = cmd_sh_cdp_entry,
		.state  = RUN|PTCNT|CMPL,
		.doc    = "Protocol information",
		.subcmd = sh_pipe
	},
	/* #show cdp entry * version# */
	{
		.name   = "version",
		.priv   = 1,
		.valid  = valid_version,
		.func   = cmd_sh_cdp_entry,
		.state  = RUN|PTCNT|CMPL,
		.doc    = "Version information",
		.subcmd = sh_cdp_entry_ve
	},
	/* #show cdp entry * |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show cdp entry ...# menu node */
static sw_command_t sh_cdp_entry[] = {
	/* #show cdp entry *# */
	{
		.name   = "*",
		.priv   = 1,
		.valid  = valid_star,
		.func   = cmd_sh_cdp_entry,
		.state  = RUN|PTCNT|CMPL,
		.doc    = "all CDP neighbor entries",
		.subcmd = sh_sh_cdp_entry
	},
	/* #show cdp entry WORD# */
	{
		.name   = "WORD",
		.priv   = 1,
		.valid  = valid_regex,
		.func   = cmd_sh_cdp_entry,
		.state  = RUN|PTCNT,
		.doc    = "Name of CDP neighbor entry",
		.subcmd = sh_sh_cdp_entry
	},
	SW_COMMAND_LIST_END
};

/* #show cdp neighbors ...# menu node */
static sw_command_t sh_cdp_ne[] = {
	/* #show cdp neighbors ethernet# */
	{
		.name   = "ethernet",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_cdp_ne_eth
	},
	/* #show cdp neighbors detail# */
	{
		.name   = "detail",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_ne_detail,
		.state  = RUN,
		.doc    = "Show detailed information",
		.subcmd = sh_pipe
	},
	/* #show cdp neighbors |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show cdp ...# menu node*/
static sw_command_t sh_cdp[] = {
	/* #show cdp entry# */
	{
		.name   = "entry",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Information for specific neighbor entry",
		.subcmd = sh_cdp_entry
	},
	/* #show cdp holdtime# */
	{
		.name   = "holdtime",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_holdtime,
		.state  = RUN,
		.doc    = "Time CDP info kept by neighbors",
		.subcmd = sh_pipe
	},
	/* #show cdp interface# */
	{
		.name   = "interface",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_int,
		.state  = RUN,
		.doc    = "CDP interface status and configuration",
		.subcmd = sh_cdp_int
	},
	/* #show cdp neighbors# */
	{
		.name   = "neighbors",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_ne,
		.state  = RUN,
		.doc    = "CDP neighbor entries",
		.subcmd = sh_cdp_ne
	},
	/* #show cdp run# */
	{
		.name   = "run",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_run,
		.state  = RUN,
		.doc    = "CDP process running",
		.subcmd = sh_pipe
	},
	/* #show cdp timer# */
	{
		.name   = "timer",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_timer,
		.state  = RUN,
		.doc    = "Time CDP info is resent to neighbors",
		.subcmd = sh_pipe
	},
	/* #show cdp traffic# */
	{
		.name   = "traffic",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp_traffic,
		.state  = RUN,
		.doc    = "CDP statistics",
		.subcmd = sh_pipe
	},
	/* #show cdp |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show running-config interface ...# menu node */
static sw_command_t sh_sh_run_int[] = {
	/* #show running-config interface ethernet# */
	{
		.name   = "ethernet",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_run_eth
	},
	/* #show running-config interface vlan# */
	{
		.name   = "vlan",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "LMS Vlans",
		.subcmd = sh_run_vlan
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table dynamic interface ...#  or
   #show mac-address-table interface ...# menu node */
static sw_command_t sh_sh_mac_int[] = {
	/* #show mac-address-table [dynamic] interface ethernet# or
	 * #show mac-address-table interface ethernet# */
	{
		.name   = "ethernet",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_mac_eth
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table address ...# menu node */
static sw_command_t sh_sh_mac_addr[] = {
	/* #show mac-address-table address H.H.H# */
	{
		.name   = "H.H.H",
		.priv   = 1,
		.valid  = valid_mac,
		.func   = cmd_sh_addr,
		.state  = RUN|PTCNT,
		.doc    = "48 bit mac address",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table dynamic ...# menu node */
static sw_command_t sh_sh_mac_sel[] = {
	/* #show mac-address-table dynamic address# */
	{
		.name   = "address",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "address keyword",
		.subcmd = sh_sh_mac_addr
	},
	/* #show mac-address-table dynamic interface# */
	{
		.name   = "interface",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "interface keyword",
		.subcmd = sh_sh_mac_int
	},
	/* #show mac-address-table dynamic vlan# */
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "VLAN keyword",
		.subcmd = sh_mac_vlan
	},
	/* #show mac-address-table dynamic |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show mac-address-table ...# menu node */
static sw_command_t sh_mac_addr_table[] = {
	/* #show mac-address-table address# */
	{
		.name   = "address",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "address keyword",
		.subcmd = sh_sh_mac_addr
	},
	/* #show mac-address-table aging-time# */
	{
		.name   = "aging-time",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_mac_age,
		.state  = RUN,
		.doc    = "aging-time keyword",
		.subcmd = sh_pipe
	},
	/* #show mac-address-table dynamic# */
	{
		.name   = "dynamic",
		.priv   = 1,
		.valid  = valid_dyn,
		.func   = cmd_show_mac,
		.state  = RUN|CMPL|PTCNT,
		.doc    = "dynamic entry type",
		.subcmd = sh_sh_mac_sel
	},
	/* #show mac-address-table interface# */
	{
		.name   = "interface",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "interface keyword",
		.subcmd = sh_sh_mac_int
	},
	/* #show mac-address-table static# */
	{
		.name   = "static",
		.priv   = 1,
		.valid  = valid_static,
		.func   = cmd_show_mac,
		.state  = RUN|CMPL|PTCNT,
		.doc    = "static entry type",
		.subcmd = sh_sh_mac_sel
	},
	/* #show mac-address-table vlan# */
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "VLAN keyword",
		.subcmd = sh_mac_vlan
	},
	/* #show mac-address-table |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show running-config ...# menu node */
static sw_command_t sh_show_run[] = {
	/* #show running-config interface# */
	{
		.name   = "interface",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Show interface configuration",
		.subcmd = sh_sh_run_int
	},
	/* #show running-config |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show mac ...# menu node */
static sw_command_t sh_sh_mac[] = {
	/* #show mac address-table# */
	{
		.name   = "address-table",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_mac,
		.state  = RUN,
		.doc    = "MAC forwarding table",
		.subcmd = sh_mac_addr_table
	},
	SW_COMMAND_LIST_END
};

/* #show vlan ...# menu node */
static sw_command_t sh_show_vlan[] = {
	/* #show vlan brief# */
	{
		.name   = "brief",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_vlan,
		.state  = RUN,
		.doc    = "VTP all VLAN status in brief",
		.subcmd = sh_pipe
	},
	/* #show vlan |# */
	{
		.name   = "|",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Output modifiers",
		.subcmd = sh_pipe_mod
	},
	SW_COMMAND_LIST_END
};

/* #show ...# menu node */
static sw_command_t sh_show[] = {
	/* #show arp# */
	{
		.name   = "arp",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = RUN,
		.doc    = "ARP table",
		.subcmd = NULL
	},
	/* #show clock# */
	{
		.name   = "clock",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = RUN,
		.doc    = "Display the system clock",
		.subcmd = NULL
	},
	/* #show cdp# */
	{
		.name   = "cdp",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_cdp,
		.state  = RUN,
		.doc    = "CDP Information",
		.subcmd = sh_cdp
	},
	/* #show history# */
	{
		.name   = "history",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_history,
		.state  = RUN,
		.doc    = "Display the session command history",
		.subcmd = sh_pipe
	},
	/* #show interfaces# */
	{
		.name   = "interfaces",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_int,
		.state  = RUN,
		.doc    = "Interface status and configuration",
		.subcmd = sh_sh_int
	},
	/* #show ip# */
	{
		.name   = "ip",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_sh_ip,
		.state  = RUN,
		.doc    = "IP information",
		.subcmd = NULL
	},
	/* #show mac# */
	{
		.name   = "mac",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "MAC configuration",
		.subcmd = sh_sh_mac
	},
	/* #show mac-address-table# */
	{
		.name   = "mac-address-table",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_mac,
		.state  = RUN,
		.doc    = "MAC forwarding table",
		.subcmd = sh_mac_addr_table
	},
	/* #show privilege# */
	{
		.name   = "privilege",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_show_priv,
		.state  = RUN,
		.doc    = "Show current privilege level",
		.subcmd = NULL
	},
	/* #show running-config# */
	{
		.name   = "running-config",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_show_run,
		.state  = RUN,
		.doc    = "Current operating configuration",
		.subcmd = sh_show_run
	},
	/* #show sessions# */
	{
		.name   = "sessions",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = RUN,
		.doc    = "Information about Telnet connections",
		.subcmd = NULL
	},
	/* #show startup-config# */
	{
		.name   = "startup-config",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_start,
		.state  = RUN,
		.doc    = "Contents of startup configuration",
		.subcmd = NULL
	},
	/* #show users# */
	{
		.name   = "users",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = RUN,
		.doc    = "Display information about terminal lines",
		.subcmd = NULL
	},
	/* #show version# */
	{
		.name   = "version",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_ver,
		.state  = RUN,
		.doc    = "System hardware and software status",
		.subcmd = NULL
	},
	/* #show vlan# */
	{
		.name   = "vlan",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_show_vlan,
		.state  = RUN,
		.doc    = "VTP VLAN status",
		.subcmd = sh_show_vlan
	},
	SW_COMMAND_LIST_END
};

/* #configure ...# menu node */
static sw_command_t sh_conf[] = {
	/* #configure terminal# */
	{
		.name   = "terminal",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_conf_t,
		.state  = RUN,
		.doc    = "Configure from the terminal",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #ping ...# menu node */
static sw_command_t sh_ping[] = {
	/* #ping WORD# */
	{
		.name   = "WORD",
		.priv   = 1,
		.valid  = valid_regex,
		.func   = cmd_ping,
		.state  = RUN|PTCNT,
		.doc    = "Ping destination address or hostname",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #traceroute ...# menu node */
static sw_command_t sh_trace[] = {
	/* #traceroute WORD# */
	{
		.name   = "WORD",
		.priv   = 1,
		.valid  = valid_regex,
		.func   = cmd_trace,
		.state  = RUN|PTCNT,
		.doc    = "Ping destination address or hostname",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

char priv_range[] = "<1-15>\0";

/* #enable ...# menu node */
static sw_command_t sh_enable[] = {
	/* #enable <1-15># */
	{
		.name   = priv_range,
		.priv   = 1,
		.valid  = valid_priv,
		.func   = cmd_enable,
		.state  = RUN|PTCNT|NPG,
		.doc    = "Enable level",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #disable ...# menu node */
static sw_command_t sh_disable[] = {
	/* #disable <1-15># */
	{
		.name   = priv_range,
		.priv   = 1,
		.valid  = valid_priv,
		.func   = cmd_disable,
		.state  = RUN|PTCNT|NPG,
		.doc    = "Privilege level to go to",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #clear mac-address-table dynamic interface ethernet ...# menu node */
static sw_command_t sh_clr_eth[] = {
	/* #clear mac-address-table dynamic interface ethernet <0-7># */
	{
		.name   = eth_range,
		.priv   = 2,
		.valid  = valid_eth,
		.func   = cmd_clr_mac_eth,
		.state  = RUN|PTCNT,
		.doc    = "Ethernet interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #clear mac address-table dynamic address ...# menu node */
static sw_command_t sh_clr_sel_addr[] = {
	/* #clear mac-address-table dynamic address H.H.H# */
	{
		.name   = "H.H.H",
		.priv   = 2,
		.valid  = valid_mac,
		.func   = cmd_clr_mac_adr,
		.state  = RUN|PTCNT,
		.doc    = "48 bit mac address",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #clear mac-address-table dynamic interface ...# */
static sw_command_t sh_clr_sel_int[] = {
	/* #clear mac-address-table dynamic interface ethernet# */
	{
		.name   = "ethernet",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Ethernet IEEE 802.3",
		.subcmd = sh_clr_eth
	},
	SW_COMMAND_LIST_END
};

/* #clear mac-address-table dynamic vlan ...# menu node */
static sw_command_t sh_clr_sel_vlan[] = {
	/* #clear mac-address-table dynamic vlan <1-1094># */
	{
		.name   = vlan_range,
		.priv   = 2,
		.valid  = valid_vlan,
		.func   = cmd_clr_mac_vl,
		.state  = RUN|PTCNT,
		.doc    = "Vlan interface number",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* #clear mac address-table dynamic ...# menu node */
static sw_command_t sh_clr_sel[] = {
	/* #clear mac-address-table dynamic address# */
	{
		.name   = "address",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "address keyword",
		.subcmd = sh_clr_sel_addr
	},
	/* #clear mac-address-table dynamic interface# */
	{
		.name   = "interface",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "interface keyword",
		.subcmd = sh_clr_sel_int
	},
	/* #clear mac-address-table dynamic vlan# */
	{
		.name   = "vlan",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "vlan keyword",
		.subcmd = sh_clr_sel_vlan
	},
	SW_COMMAND_LIST_END
};

/* #clear mac-address-table ...# menu node*/
static sw_command_t sh_clr_mac_addr[] = {
	/* #clear mac-address-table dynamic# or
	 * #clear mac address-table dynamic# */
	{
		.name   = "dynamic",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_clr_mac,
		.state  = RUN,
		.doc    = "dynamic entry type",
		.subcmd = sh_clr_sel
	},
	SW_COMMAND_LIST_END
};

/* #clear mac ...# menu node*/
static sw_command_t sh_clr_mac[] = {
	/* #clear mac address-table# */
	{
		.name   = "address-table",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "MAC forwarding table",
		.subcmd = sh_clr_mac_addr
	},
	SW_COMMAND_LIST_END
};

/* #clear ...#  menu node */
static sw_command_t sh_clear[] = {
	/* #clear mac# */
	{
		.name   = "mac",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "MAC forwarding table",
		.subcmd = sh_clr_mac
	},
	/* #clear mac-address-table# */
	{
		.name   = "mac-address-table",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "MAC forwarding table",
		.subcmd = sh_clr_mac_addr
	},
	SW_COMMAND_LIST_END
};

/* #write ...# menu node */
static sw_command_t sh_write[] = {
	/* #write memory# */
	{
		.name   = "memory",
		.priv   = 15,
		.valid  = NULL,
		.func   = cmd_wrme,
		.state  = RUN,
		.doc    = "Write to NV memory",
		.subcmd = NULL
	},
	SW_COMMAND_LIST_END
};

/* Main menu node */
static sw_command_t sh[] = {
	/* #clear# */
	{
		.name   = "clear",
		.priv   = 2,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Reset functions",
		.subcmd = sh_clear
	},
	/* #configure# */
	{
		.name   = "configure",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Enter configuration mode",
		.subcmd = sh_conf
	},
	/* #disable# */
	{
		.name   = "disable",
		.priv   = 2,
		.valid  = NULL,
		.func   = cmd_disable,
		.state  = RUN,
		.doc    = "Turn off privileged commands",
		.subcmd = sh_disable
	},
	/* #enable# */
	{
		.name   = "enable",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_enable,
		.state  = RUN|NPG,
		.doc    = "Turn on privileged commands",
		.subcmd = sh_enable
	},
	/* #exit# */
	{
		.name   = "exit",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Exit from the EXEC",
		.subcmd = NULL
	},
	/* #help# */
	{
		.name   = "help",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_help,
		.state  = RUN,
		.doc    = "Description of the interactive help system",
		.subcmd = NULL
	},
	/* #logout# */
	{
		.name   = "logout",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Exit from the EXEC",
		.subcmd = NULL
	},
	/* #ping# */
	{
		.name   = "ping",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Send echo messages",
		.subcmd = sh_ping
	},
	/* #reload# */
	{
		.name   = "reload",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_reload,
		.state  = RUN|NPG,
		.doc    = "Halt and perform a cold restart",
		.subcmd = NULL
	},
	/* #quit# */
	{
		.name   = "quit",
		.priv   = 1,
		.valid  = NULL,
		.func   = cmd_exit,
		.state  = RUN,
		.doc    = "Exit from the EXEC",
		.subcmd = NULL
	},
	/* #show# */
	{
		.name   = "show",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Show running system information",
		.subcmd = sh_show
	},
	/* #traceroute# */
	{
		.name   = "traceroute",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Trace route to destination",
		.subcmd = sh_trace
	},
	/* #where# */
	{
		.name   = "where",
		.priv   = 1,
		.valid  = NULL,
		.func   = NULL,
		.state  = RUN,
		.doc    = "List active connections",
		.subcmd = NULL
	},
	/* #write# */
	{
		.name   = "write",
		.priv   = 15,
		.valid  = NULL,
		.func   = NULL,
		.state  = 0,
		.doc    = "Write running configuration to memory",
		.subcmd = sh_write
	},
	SW_COMMAND_LIST_END
};

/* Main command menu */
sw_command_root_t command_root_main = {"%s%c", sh};
