#include "swcli.h"
#include "cdp.h"
#include "exec.h"

extern struct menu_node *show_mac_addr_t[];
extern struct menu_node *clear_mac_addr_t[];

static struct menu_node output_modifiers_line = {
	.name			= "LINE",
	.help			= "Regular Expression",
	.mask			= CLI_MASK(PRIV(1)),
	.tokenize	= swcli_tokenize_line,
	.run			= swcli_output_modifiers_run,
	.subtree	= NULL
};

struct menu_node output_modifiers = {
	.name			= "|",
	.help			= "Output modifiers",
	.mask			= CLI_MASK(PRIV(1)),
	.tokenize	= NULL,
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		& (struct menu_node){
			.name			= "begin",
			.help			= "Begin with the line that matches",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize		= swcli_tokenize_line,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) {
				&output_modifiers_line,
				NULL
			}
		},

		& (struct menu_node){
			.name			= "exclude",
			.help			= "Exclude lines that match",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize		= swcli_tokenize_line,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) {
				&output_modifiers_line,
				NULL
			}
		},

		& (struct menu_node){
			.name			= "include",
			.help			= "Include lines that match",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize		= swcli_tokenize_line,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) {
				&output_modifiers_line,
				NULL
			}
		},

		/* #show cdp entry * protocol | grep */
		& (struct menu_node){
			.name			= "grep",
			.help			= "Linux grep functionality",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize		= swcli_tokenize_line,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) {
				&output_modifiers_line,
				NULL
			}
		},

		NULL
	} /*}}}*/
};

struct menu_node *output_modifiers_subtree[] = {
	&output_modifiers,
	NULL
};

static struct menu_node cdp_ne_det = {
	.name			= "detail",
	.help			= "Show detailed information",
	.mask			= CLI_MASK(PRIV(1)),
	.tokenize	= NULL,
	.run			= cmd_sh_cdp_ne,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		/* #show cdp neighbors ethernet  detail | */
		&output_modifiers,
		NULL
	} /*}}}*/
};

static struct menu_node *cdp_ne_if_subtree[] = {
	&cdp_ne_det,
	&output_modifiers,
	NULL
};

static struct menu_node *sh_int_subtree[] = {
	// TODO don't use output_modifiers_subtree because we'll have to
	// extend this anyway when we implement more subnodes
	&output_modifiers,
	NULL
};

struct menu_node menu_main = {
	/* Root node, .name is used as prompt */
	.name			= NULL,
	.subtree	= (struct menu_node *[]) {
		/* #clear */
		& (struct menu_node){
			.name			= "clear",
			.help			= "Reset functions",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #clear mac */
				& (struct menu_node){
					.name			= "mac",
					.help			= "MAC forwarding table",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #clear mac address-table */
						& (struct menu_node){
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= clear_mac_addr_t,
						},

						NULL
					} /*}}}*/
				},

				/* #clear mac-address-table */
				& (struct menu_node){
					.name			= "mac-address-table",
					.help			= "MAC forwarding table",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= clear_mac_addr_t,
				},

				NULL
			} /*}}}*/
		},

		/* #configure */
		& (struct menu_node){
			.name			= "configure",
			.help			= "Enter configuration mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #configure terminal */
				& (struct menu_node){
					.name			= "terminal",
					.help			= "Configure from the terminal",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_conf_t,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #disable */
		& (struct menu_node){
			.name			= "disable",
			.help			= "Turn off privileged commands",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_disable,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #disable <1-15> */
				& (struct menu_node){
					.name			= "<1-15>",
					.help			= "Privilege level to go to",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_disable,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #enable */
		& (struct menu_node){
			.name			= "enable",
			.help			= "Turn on privileged commands",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_enable,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #enable <1-15> */
				& (struct menu_node){
					.name			= "<1-15>",
					.help			= "Enable level",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_enable,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #exit */
		& (struct menu_node){
			.name			= "exit",
			.help			= "Exit from the EXEC",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #help */
		& (struct menu_node){
			.name			= "help",
			.help			= "Description of the interactive help system",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_help,
			.subtree	= NULL
		},

		/* #logout */
		& (struct menu_node){
			.name			= "logout",
			.help			= "Exit from the EXEC",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #ping */
		& (struct menu_node){
			.name			= "ping",
			.help			= "Send echo messages",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #ping WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_ping,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #reload */
		& (struct menu_node){
			.name			= "reload",
			.help			= "Halt and perform a cold restart",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_reload,
			.subtree	= NULL
		},

		/* #quit */
		& (struct menu_node){
			.name			= "quit",
			.help			= "Exit from the EXEC",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #show */
		& (struct menu_node){
			.name			= "show",
			.help			= "Show running system information",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #show arp */
				& (struct menu_node){
					.name			= "arp",
					.help			= "ARP table",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show clock */
				& (struct menu_node){
					.name			= "clock",
					.help			= "Display the system clock",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show cdp */
				& (struct menu_node){
					.name			= "cdp",
					.help			= "CDP Information",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_sh_cdp,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show cdp entry */
						& (struct menu_node){
							.name			= "entry",
							.help			= "Information for specific neighbor entry",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp entry * */
								& (struct menu_node){
									.name			= "*",
									.help			= "all CDP neighbor entries",
									.mask			= CLI_MASK(PRIV(1)),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp entry * protocol */
										& (struct menu_node){
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= CLI_MASK(PRIV(1)),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) {
												&output_modifiers,
												NULL
											}
										},

										/* #show cdp entry * version */
										& (struct menu_node){
											.name			= "version",
											.help			= "Version information",
											.mask			= CLI_MASK(PRIV(1)),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry * version protocol */
												& (struct menu_node){
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= CLI_MASK(PRIV(1)),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_entry,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * version protocol | */
														&output_modifiers,
														NULL
													} /*}}}*/
												},

												/* #show cdp entry * version | */
												&output_modifiers,
												NULL
											} /*}}}*/
										},

										/* #show cdp entry * | */
										&output_modifiers,
										NULL
									} /*}}}*/
								},

								/* #show cdp entry WORD */
								& (struct menu_node){
									.name			= "WORD",
									.help			= "Name of CDP neighbor entry",
									.mask			= CLI_MASK(PRIV(1)),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp entry WORD protocol */
										& (struct menu_node){
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= CLI_MASK(PRIV(1)),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry WORD protocol | */
												&output_modifiers,
												NULL
											} /*}}}*/
										},

										/* #show cdp entry WORD version */
										& (struct menu_node){
											.name			= "version",
											.help			= "Version information",
											.mask			= CLI_MASK(PRIV(1)),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry WORD version protocol */
												& (struct menu_node){
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= CLI_MASK(PRIV(1)),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_entry,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD version protocol | */
														&output_modifiers,
														NULL
													} /*}}}*/
												},

												/* #show cdp entry WORD version | */
												&output_modifiers,
												NULL
											} /*}}}*/
										},

										/* #show cdp entry WORD | */
										&output_modifiers,
										NULL
									} /*}}}*/
								},

								NULL
							} /*}}}*/
						},

						/* #show cdp holdtime */
						& (struct menu_node){
							.name			= "holdtime",
							.help			= "Time CDP info kept by neighbors",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_holdtime,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp holdtime | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp interface */
						& (struct menu_node){
							.name			= "interface",
							.help			= "CDP interface status and configuration",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= if_tok_if,
							.run			= cmd_sh_cdp_int,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								IF_ETHER(output_modifiers_subtree, cmd_sh_cdp_int, NULL),
								IF_NETDEV(output_modifiers_subtree, cmd_sh_cdp_int, NULL),

								/* #show cdp interface | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp neighbors */
						& (struct menu_node){
							.name			= "neighbors",
							.help			= "CDP neighbor entries",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= if_tok_if,
							.run			= cmd_sh_cdp_ne,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp neighbors ethernet */
								IF_ETHER(cdp_ne_if_subtree, cmd_sh_cdp_ne, NULL),
								IF_NETDEV(cdp_ne_if_subtree, cmd_sh_cdp_ne, NULL),

								/* #show cdp neighbors detail */
								& cdp_ne_det,

								/* #show cdp neighbors | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp run */
						& (struct menu_node){
							.name			= "run",
							.help			= "CDP process running",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_run,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp run | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp timer */
						& (struct menu_node){
							.name			= "timer",
							.help			= "Time CDP info is resent to neighbors",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_timer,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp timer | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp traffic */
						& (struct menu_node){
							.name			= "traffic",
							.help			= "CDP statistics",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_traffic,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp traffic | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp | */
						&output_modifiers,
						NULL
					} /*}}}*/
				},

				/* #show history */
				& (struct menu_node){
					.name			= "history",
					.help			= "Display the session command history",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_history,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show history | */
						&output_modifiers,
						NULL
					} /*}}}*/
				},

				/* #show interfaces */
				& (struct menu_node){
					.name			= "interfaces",
					.help			= "Interface status and configuration",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= if_tok_if,
					.run			= cmd_sh_int,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show interfaces ethernet */
						IF_ETHER(sh_int_subtree, cmd_sh_int, NULL),

						/* #show interfaces vlan */
						IF_VLAN(sh_int_subtree, cmd_sh_int, NULL),

						/* #show interfaces netdev */
						IF_NETDEV(sh_int_subtree, cmd_sh_int, NULL),

						/* #show interfaces | */
						&output_modifiers,

						NULL
					} /*}}}*/
				},

				/* #show ip */
				& (struct menu_node){
					.name			= "ip",
					.help			= "IP information",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_sh_ip,
					.subtree	= NULL
				},

				/* #show mac */
				& (struct menu_node){
					.name			= "mac",
					.help			= "MAC configuration",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show mac address-table */
						& (struct menu_node){
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_sh_mac_addr_t,
							.subtree	= show_mac_addr_t
						},

						NULL
					} /*}}}*/
				},

				/* #show mac-address-table */
				& (struct menu_node){
					.name			= "mac-address-table",
					.help			= "MAC forwarding table",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_sh_mac_addr_t,
					.subtree	= show_mac_addr_t
				},

				/* #show privilege */
				& (struct menu_node){
					.name			= "privilege",
					.help			= "Show current privilege level",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_show_priv,
					.subtree	= NULL
				},

				/* #show running-config */
				& (struct menu_node){
					.name			= "running-config",
					.help			= "Current operating configuration",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_show_run,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show running-config interface */
						& (struct menu_node){
							.name			= "interface",
							.help			= "Show interface configuration",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= if_tok_if,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show running-config interface ethernet */
								IF_ETHER(output_modifiers_subtree, cmd_show_run, NULL),

								/* #show running-config interface netdev */
								IF_NETDEV(output_modifiers_subtree, cmd_show_run, NULL),

								/* #show running-config interface vlan */
								IF_VLAN(output_modifiers_subtree, cmd_show_run, NULL),

								NULL
							} /*}}}*/
						},

						/* #show running-config | */
						&output_modifiers,
						NULL
					} /*}}}*/
				},

				/* #show sessions */
				& (struct menu_node){
					.name			= "sessions",
					.help			= "Information about Telnet connections",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show startup-config */
				& (struct menu_node){
					.name			= "startup-config",
					.help			= "Contents of startup configuration",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_show_start,
					.subtree	= NULL
				},

				/* #show users */
				& (struct menu_node){
					.name			= "users",
					.help			= "Display information about terminal lines",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show version */
				& (struct menu_node){
					.name			= "version",
					.help			= "System hardware and software status",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_show_ver,
					.subtree	= NULL
				},

				/* #show vlan */
				& (struct menu_node){
					.name			= "vlan",
					.help			= "VTP VLAN status",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_show_vlan,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show vlan brief */
						& (struct menu_node){
							.name			= "brief",
							.help			= "VTP all VLAN status in brief",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= NULL,
							.run			= cmd_show_vlan,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show vlan brief | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show vlan | */
						&output_modifiers,
						NULL
					} /*}}}*/
				},

				NULL
			} /*}}}*/
		},

		/* #traceroute */
		& (struct menu_node){
			.name			= "traceroute",
			.help			= "Trace route to destination",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #traceroute WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= CLI_MASK(PRIV(1)),
					.tokenize	= NULL,
					.run			= cmd_trace,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #where */
		& (struct menu_node){
			.name			= "where",
			.help			= "List active connections",
			.mask			= CLI_MASK(PRIV(1)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= NULL
		},

		/* #write */
		& (struct menu_node){
			.name			= "write",
			.help			= "Write running configuration to memory",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #write memory */
				& (struct menu_node){
					.name			= "memory",
					.help			= "Write to NV memory",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_wrme,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		NULL
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/
