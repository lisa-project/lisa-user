#include "cli.h"
#include "swcli_common.h"

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
	.tokenize	= NULL,
	.run			= NULL,
	.subtree	= NULL
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
			.tokenize	= NULL,
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
			.tokenize	= NULL,
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
			.tokenize	= NULL,
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
			.tokenize	= NULL,
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
														& (struct menu_node){
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry * version protocol | begin */
																& (struct menu_node){
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry * version protocol | begin LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry * version protocol | exclude */
																& (struct menu_node){
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry * version protocol | exclude LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry * version protocol | include */
																& (struct menu_node){
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry * version protocol | include LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry * version protocol | grep */
																& (struct menu_node){
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry * version protocol | grep LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
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

												/* #show cdp entry * version | */
												& (struct menu_node){
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * version | begin */
														& (struct menu_node){
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry * version | begin LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry * version | exclude */
														& (struct menu_node){
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry * version | exclude LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry * version | include */
														& (struct menu_node){
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry * version | include LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry * version | grep */
														& (struct menu_node){
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry * version | grep LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
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

										/* #show cdp entry * | */
										& (struct menu_node){
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry * | begin */
												& (struct menu_node){
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * | begin LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry * | exclude */
												& (struct menu_node){
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * | exclude LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry * | include */
												& (struct menu_node){
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * | include LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry * | grep */
												& (struct menu_node){
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry * | grep LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
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
												& (struct menu_node){
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD protocol | begin */
														& (struct menu_node){
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD protocol | begin LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD protocol | exclude */
														& (struct menu_node){
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD protocol | exclude LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD protocol | include */
														& (struct menu_node){
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD protocol | include LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD protocol | grep */
														& (struct menu_node){
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD protocol | grep LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
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
														& (struct menu_node){
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD version protocol | begin */
																& (struct menu_node){
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry WORD version protocol | begin LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry WORD version protocol | exclude */
																& (struct menu_node){
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry WORD version protocol | exclude LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry WORD version protocol | include */
																& (struct menu_node){
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry WORD version protocol | include LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp entry WORD version protocol | grep */
																& (struct menu_node){
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp entry WORD version protocol | grep LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
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

												/* #show cdp entry WORD version | */
												& (struct menu_node){
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD version | begin */
														& (struct menu_node){
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD version | begin LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD version | exclude */
														& (struct menu_node){
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD version | exclude LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD version | include */
														& (struct menu_node){
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD version | include LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL
															} /*}}}*/
														},

														/* #show cdp entry WORD version | grep */
														& (struct menu_node){
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp entry WORD version | grep LINE */
																& (struct menu_node){
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
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

										/* #show cdp entry WORD | */
										& (struct menu_node){
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp entry WORD | begin */
												& (struct menu_node){
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD | begin LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry WORD | exclude */
												& (struct menu_node){
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD | exclude LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry WORD | include */
												& (struct menu_node){
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD | include LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp entry WORD | grep */
												& (struct menu_node){
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp entry WORD | grep LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
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

						/* #show cdp holdtime */
						& (struct menu_node){
							.name			= "holdtime",
							.help			= "Time CDP info kept by neighbors",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_holdtime,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp holdtime | */
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp holdtime | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp holdtime | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp holdtime | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp holdtime | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp holdtime | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp holdtime | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp holdtime | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp holdtime | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp interface | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp interface | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp interface | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp interface | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp interface | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp interface | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp interface | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp interface | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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
														& (struct menu_node){
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #show cdp neighbors ethernet  detail | begin */
																& (struct menu_node){
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp neighbors ethernet  detail | begin LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp neighbors ethernet  detail | exclude */
																& (struct menu_node){
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp neighbors ethernet  detail | exclude LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp neighbors ethernet  detail | include */
																& (struct menu_node){
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp neighbors ethernet  detail | include LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL
																	} /*}}}*/
																},

																/* #show cdp neighbors ethernet  detail | grep */
																& (struct menu_node){
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #show cdp neighbors ethernet  detail | grep LINE */
																		& (struct menu_node){
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
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
										& (struct menu_node){
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors detail | begin */
												& (struct menu_node){
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp neighbors detail | begin LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp neighbors detail | exclude */
												& (struct menu_node){
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp neighbors detail | exclude LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp neighbors detail | include */
												& (struct menu_node){
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp neighbors detail | include LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL
													} /*}}}*/
												},

												/* #show cdp neighbors detail | grep */
												& (struct menu_node){
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #show cdp neighbors detail | grep LINE */
														& (struct menu_node){
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
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

								/* #show cdp neighbors | */
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp neighbors | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp neighbors | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp neighbors | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp neighbors | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp neighbors | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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

						/* #show cdp run */
						& (struct menu_node){
							.name			= "run",
							.help			= "CDP process running",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_run,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp run | */
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp run | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp run | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp run | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp run | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp run | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp run | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp run | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp run | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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

						/* #show cdp timer */
						& (struct menu_node){
							.name			= "timer",
							.help			= "Time CDP info is resent to neighbors",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_timer,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp timer | */
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp timer | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp timer | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp timer | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp timer | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp timer | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp timer | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp timer | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp timer | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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

						/* #show cdp traffic */
						& (struct menu_node){
							.name			= "traffic",
							.help			= "CDP statistics",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_traffic,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp traffic | */
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp traffic | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp traffic | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp traffic | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp traffic | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp traffic | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp traffic | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show cdp traffic | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show cdp traffic | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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

						/* #show cdp | */
						& (struct menu_node){
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show cdp | begin */
								& (struct menu_node){
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp | begin LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show cdp | exclude */
								& (struct menu_node){
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp | exclude LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show cdp | include */
								& (struct menu_node){
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp | include LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show cdp | grep */
								& (struct menu_node){
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show cdp | grep LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
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

				/* #show history */
				& (struct menu_node){
					.name			= "history",
					.help			= "Display the session command history",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_history,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #show history | */
						& (struct menu_node){
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show history | begin */
								& (struct menu_node){
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show history | begin LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show history | exclude */
								& (struct menu_node){
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show history | exclude LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show history | include */
								& (struct menu_node){
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show history | include LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show history | grep */
								& (struct menu_node){
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show history | grep LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
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
						& (struct menu_node){
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show running-config | begin */
								& (struct menu_node){
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config | begin LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show running-config | exclude */
								& (struct menu_node){
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config | exclude LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show running-config | include */
								& (struct menu_node){
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config | include LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show running-config | grep */
								& (struct menu_node){
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show running-config | grep LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
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
								& (struct menu_node){
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show vlan brief | begin */
										& (struct menu_node){
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show vlan brief | begin LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show vlan brief | exclude */
										& (struct menu_node){
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show vlan brief | exclude LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show vlan brief | include */
										& (struct menu_node){
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show vlan brief | include LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #show vlan brief | grep */
										& (struct menu_node){
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #show vlan brief | grep LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
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

						/* #show vlan | */
						& (struct menu_node){
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #show vlan | begin */
								& (struct menu_node){
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show vlan | begin LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show vlan | exclude */
								& (struct menu_node){
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show vlan | exclude LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show vlan | include */
								& (struct menu_node){
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show vlan | include LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL
									} /*}}}*/
								},

								/* #show vlan | grep */
								& (struct menu_node){
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #show vlan | grep LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
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
