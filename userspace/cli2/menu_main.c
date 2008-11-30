#include "cli.h"
#include "swcli_common.h"

int swcli_tokenize_line(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_output_modifiers_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev);

int cmd_clr_mac(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_adr(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_vl(struct cli_context *, int, char **, struct menu_node **);
int cmd_conf_t(struct cli_context *, int, char **, struct menu_node **);
int cmd_disable(struct cli_context *, int, char **, struct menu_node **);
int cmd_enable(struct cli_context *, int, char **, struct menu_node **);
int cmd_quit(struct cli_context *, int, char **, struct menu_node **);
int cmd_help(struct cli_context *, int, char **, struct menu_node **);
int cmd_ping(struct cli_context *, int, char **, struct menu_node **);
int cmd_reload(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_entry(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_holdtime(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_ne_detail(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_timer(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_cdp_traffic(struct cli_context *, int, char **, struct menu_node **);
int cmd_history(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_int(struct cli_context *, int, char **, struct menu_node **);
int cmd_int_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_int_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_ip(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_addr(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_mac_age(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_mac_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_sh_mac_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_priv(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_run_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_run_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_start(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_ver(struct cli_context *, int, char **, struct menu_node **);
int cmd_show_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_trace(struct cli_context *, int, char **, struct menu_node **);
int cmd_wrme(struct cli_context *, int, char **, struct menu_node **);

int cmd_sh_mac_addr_t(struct cli_context *, int, char **, struct menu_node **);
extern struct menu_node *show_mac_addr_t[];

static struct menu_node output_modifiers_line = {
	.name			= "LINE",
	.help			= "Regular Expression",
	.mask			= PRIV(1),
	.tokenize		= swcli_tokenize_line,
	.run			= swcli_output_modifiers_run,
	/* this recurrent node is actually a clever trick that
	 * helps tokenizing the line. at every step the tokenize()
	 * function will extract exactly one token and add it into
	 * tokv[argc] */
	.subtree		= (struct menu_node *[]) {
		&output_modifiers_line,
		NULL
	}
};

struct menu_node output_modifiers = {
	.name			= "|",
	.help			= "Output modifiers",
	.mask			= PRIV(1),
	.tokenize	= NULL,
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		& (struct menu_node){
			.name			= "begin",
			.help			= "Begin with the line that matches",
			.mask			= PRIV(1),
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
			.mask			= PRIV(1),
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
			.mask			= PRIV(1),
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
			.mask			= PRIV(1),
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

struct menu_node menu_main = {
	/* Root node, .name is used as prompt */
	.name			= NULL,
	.subtree	= (struct menu_node *[]) {
		/* #clear */
		& (struct menu_node){
			.name			= "clear",
			.help			= "Reset functions",
			.mask			= PRIV(2),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #clear mac */
				& (struct menu_node){
					.name			= "mac",
					.help			= "MAC forwarding table",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #clear mac address-table */
						& (struct menu_node){
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= PRIV(2),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #clear mac address-table dynamic */
								& (struct menu_node){
									.name			= "dynamic",
									.help			= "dynamic entry type",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= cmd_clr_mac,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #clear mac address-table dynamic address */
										& (struct menu_node){
											.name			= "address",
											.help			= "address keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #clear mac address-table dynamic address H.H.H */
												& (struct menu_node){
													.name			= "H.H.H",
													.help			= "48 bit mac address",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_adr,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #clear mac address-table dynamic interface */
										& (struct menu_node){
											.name			= "interface",
											.help			= "interface keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #clear mac address-table dynamic interface ethernet */
												& (struct menu_node){
													.name			= "ethernet",
													.help			= "Ethernet IEEE 802.3",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #clear mac address-table dynamic interface ethernet  */
														& (struct menu_node){
															.name			= "",
															.help			= "Ethernet interface number",
															.mask			= PRIV(2),
															.tokenize	= NULL,
															.run			= cmd_clr_mac_eth,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												NULL
											} /*}}}*/
										},

										/* #clear mac address-table dynamic vlan */
										& (struct menu_node){
											.name			= "vlan",
											.help			= "vlan keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #clear mac address-table dynamic vlan <1-1094> */
												& (struct menu_node){
													.name			= "<1-1094>",
													.help			= "Vlan interface number",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_vl,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										NULL
									} /*}}}*/
								},

								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				/* #clear mac-address-table */
				& (struct menu_node){
					.name			= "mac-address-table",
					.help			= "MAC forwarding table",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #clear mac-address-table dynamic */
						& (struct menu_node){
							.name			= "dynamic",
							.help			= "dynamic entry type",
							.mask			= PRIV(2),
							.tokenize	= NULL,
							.run			= cmd_clr_mac,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #clear mac-address-table dynamic address */
								& (struct menu_node){
									.name			= "address",
									.help			= "address keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #clear mac-address-table dynamic address H.H.H */
										& (struct menu_node){
											.name			= "H.H.H",
											.help			= "48 bit mac address",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_clr_mac_adr,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #clear mac-address-table dynamic interface */
								& (struct menu_node){
									.name			= "interface",
									.help			= "interface keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #clear mac-address-table dynamic interface ethernet */
										& (struct menu_node){
											.name			= "ethernet",
											.help			= "Ethernet IEEE 802.3",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #clear mac-address-table dynamic interface ethernet  */
												& (struct menu_node){
													.name			= "",
													.help			= "Ethernet interface number",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_eth,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										NULL
									} /*}}}*/
								},

								/* #clear mac-address-table dynamic vlan */
								& (struct menu_node){
									.name			= "vlan",
									.help			= "vlan keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #clear mac-address-table dynamic vlan <1-1094> */
										& (struct menu_node){
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_clr_mac_vl,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				NULL
			} /*}}}*/
		},

		/* #configure */
		& (struct menu_node){
			.name			= "configure",
			.help			= "Enter configuration mode",
			.mask			= PRIV(15),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #configure terminal */
				& (struct menu_node){
					.name			= "terminal",
					.help			= "Configure from the terminal",
					.mask			= PRIV(2),
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
			.mask			= PRIV(2),
			.tokenize	= NULL,
			.run			= cmd_disable,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #disable <1-15> */
				& (struct menu_node){
					.name			= "<1-15>",
					.help			= "Privilege level to go to",
					.mask			= PRIV(1),
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
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_enable,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #enable <1-15> */
				& (struct menu_node){
					.name			= "<1-15>",
					.help			= "Enable level",
					.mask			= PRIV(1),
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
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #help */
		& (struct menu_node){
			.name			= "help",
			.help			= "Description of the interactive help system",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_help,
			.subtree	= NULL
		},

		/* #logout */
		& (struct menu_node){
			.name			= "logout",
			.help			= "Exit from the EXEC",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #ping */
		& (struct menu_node){
			.name			= "ping",
			.help			= "Send echo messages",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #ping WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= PRIV(1),
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
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_reload,
			.subtree	= NULL
		},

		/* #quit */
		& (struct menu_node){
			.name			= "quit",
			.help			= "Exit from the EXEC",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_quit,
			.subtree	= NULL
		},

		/* #show */
		& (struct menu_node){
			.name			= "show",
			.help			= "Show running system information",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #show arp */
				& (struct menu_node){
					.name			= "arp",
					.help			= "ARP table",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show clock */
				& (struct menu_node){
					.name			= "clock",
					.help			= "Display the system clock",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show cdp */
				& (struct menu_node){
					.name			= "cdp",
					.help			= "CDP Information",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_cdp,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show cdp entry */
						& (struct menu_node){
							.name			= "entry",
							.help			= "Information for specific neighbor entry",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp entry * */
								& (struct menu_node){
									.name			= "*",
									.help			= "all CDP neighbor entries",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp entry * protocol */
										& (struct menu_node){
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= PRIV(1),
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
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry * version protocol */
												& (struct menu_node){
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= PRIV(1),
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
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp entry WORD protocol */
										& (struct menu_node){
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= PRIV(1),
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
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry WORD version protocol */
												& (struct menu_node){
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= PRIV(1),
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
							.mask			= PRIV(1),
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
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_int,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp interface ethernet */
								& (struct menu_node){
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp interface ethernet  */
										& (struct menu_node){
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_int,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show cdp interface | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp neighbors */
						& (struct menu_node){
							.name			= "neighbors",
							.help			= "CDP neighbor entries",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_ne,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp neighbors ethernet */
								& (struct menu_node){
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp neighbors ethernet  */
										& (struct menu_node){
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_ne_int,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors ethernet  detail */
												& (struct menu_node){
													.name			= "detail",
													.help			= "Show detailed information",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_ne_detail,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp neighbors ethernet  detail | */
														&output_modifiers,
														NULL
													} /*}}}*/
												},

												NULL
											} /*}}}*/
										},

										NULL
									} /*}}}*/
								},

								/* #show cdp neighbors detail */
								& (struct menu_node){
									.name			= "detail",
									.help			= "Show detailed information",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_ne_detail,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp neighbors detail | */
										&output_modifiers,
										NULL
									} /*}}}*/
								},

								/* #show cdp neighbors | */
								&output_modifiers,
								NULL
							} /*}}}*/
						},

						/* #show cdp run */
						& (struct menu_node){
							.name			= "run",
							.help			= "CDP process running",
							.mask			= PRIV(1),
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
							.mask			= PRIV(1),
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
							.mask			= PRIV(1),
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
					.mask			= PRIV(1),
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
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_int,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show interfaces ethernet */
						& (struct menu_node){
							.name			= "ethernet",
							.help			= "Ethernet IEEE 802.3",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show interfaces ethernet  */
								& (struct menu_node){
									.name			= "",
									.help			= "Ethernet interface number",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_int_eth,
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						/* #show interfaces vlan */
						& (struct menu_node){
							.name			= "vlan",
							.help			= "LMS Vlans",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show interfaces vlan <1-1094> */
								& (struct menu_node){
									.name			= "<1-1094>",
									.help			= "Vlan interface number",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_int_vlan,
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				/* #show ip */
				& (struct menu_node){
					.name			= "ip",
					.help			= "IP information",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_ip,
					.subtree	= NULL
				},

				/* #show mac */
				& (struct menu_node){
					.name			= "mac",
					.help			= "MAC configuration",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show mac address-table */
						& (struct menu_node){
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= PRIV(1),
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
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_mac_addr_t,
					.subtree	= show_mac_addr_t
				},

				/* #show privilege */
				& (struct menu_node){
					.name			= "privilege",
					.help			= "Show current privilege level",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= cmd_show_priv,
					.subtree	= NULL
				},

				/* #show running-config */
				& (struct menu_node){
					.name			= "running-config",
					.help			= "Current operating configuration",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= cmd_show_run,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show running-config interface */
						& (struct menu_node){
							.name			= "interface",
							.help			= "Show interface configuration",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show running-config interface ethernet */
								& (struct menu_node){
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config interface ethernet  */
										& (struct menu_node){
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_run_eth,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show running-config interface vlan */
								& (struct menu_node){
									.name			= "vlan",
									.help			= "LMS Vlans",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config interface vlan <1-1094> */
										& (struct menu_node){
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_run_vlan,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

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
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show startup-config */
				& (struct menu_node){
					.name			= "startup-config",
					.help			= "Contents of startup configuration",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_start,
					.subtree	= NULL
				},

				/* #show users */
				& (struct menu_node){
					.name			= "users",
					.help			= "Display information about terminal lines",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				/* #show version */
				& (struct menu_node){
					.name			= "version",
					.help			= "System hardware and software status",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_ver,
					.subtree	= NULL
				},

				/* #show vlan */
				& (struct menu_node){
					.name			= "vlan",
					.help			= "VTP VLAN status",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_vlan,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show vlan brief */
						& (struct menu_node){
							.name			= "brief",
							.help			= "VTP all VLAN status in brief",
							.mask			= PRIV(1),
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
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #traceroute WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= PRIV(1),
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
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= NULL
		},

		/* #write */
		& (struct menu_node){
			.name			= "write",
			.help			= "Write running configuration to memory",
			.mask			= PRIV(15),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #write memory */
				& (struct menu_node){
					.name			= "memory",
					.help			= "Write to NV memory",
					.mask			= PRIV(15),
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
