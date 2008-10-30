#include "cli.h"
#include "swcli_common.h"

int cmd_clr_mac(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_adr(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_clr_mac_vl(struct cli_context *, int, char **, struct menu_node **);
int cmd_conf_t(struct cli_context *, int, char **, struct menu_node **);
int cmd_disable(struct cli_context *, int, char **, struct menu_node **);
int cmd_enable(struct cli_context *, int, char **, struct menu_node **);
int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
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
int cmd_show_mac(struct cli_context *, int, char **, struct menu_node **);
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

struct menu_node menu_main = {
	/* Root node, .name is used as prompt */
	.name			= NULL,
	.subtree	= (struct menu_node[]) {
		{ /* #clear */
			.name			= "clear",
			.help			= "Reset functions",
			.mask			= PRIV(2),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #clear mac */
					.name			= "mac",
					.help			= "MAC forwarding table",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #clear mac address-table */
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= PRIV(2),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #clear mac address-table dynamic */
									.name			= "dynamic",
									.help			= "dynamic entry type",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= cmd_clr_mac,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #clear mac address-table dynamic address */
											.name			= "address",
											.help			= "address keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #clear mac address-table dynamic address H.H.H */
													.name			= "H.H.H",
													.help			= "48 bit mac address",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_adr,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #clear mac address-table dynamic interface */
											.name			= "interface",
											.help			= "interface keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #clear mac address-table dynamic interface ethernet */
													.name			= "ethernet",
													.help			= "Ethernet IEEE 802.3",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #clear mac address-table dynamic interface ethernet  */
															.name			= "",
															.help			= "Ethernet interface number",
															.mask			= PRIV(2),
															.tokenize	= NULL,
															.run			= cmd_clr_mac_eth,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #clear mac address-table dynamic vlan */
											.name			= "vlan",
											.help			= "vlan keyword",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #clear mac address-table dynamic vlan <1-1094> */
													.name			= "<1-1094>",
													.help			= "Vlan interface number",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_vl,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #clear mac-address-table */
					.name			= "mac-address-table",
					.help			= "MAC forwarding table",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #clear mac-address-table dynamic */
							.name			= "dynamic",
							.help			= "dynamic entry type",
							.mask			= PRIV(2),
							.tokenize	= NULL,
							.run			= cmd_clr_mac,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #clear mac-address-table dynamic address */
									.name			= "address",
									.help			= "address keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #clear mac-address-table dynamic address H.H.H */
											.name			= "H.H.H",
											.help			= "48 bit mac address",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_clr_mac_adr,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #clear mac-address-table dynamic interface */
									.name			= "interface",
									.help			= "interface keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #clear mac-address-table dynamic interface ethernet */
											.name			= "ethernet",
											.help			= "Ethernet IEEE 802.3",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #clear mac-address-table dynamic interface ethernet  */
													.name			= "",
													.help			= "Ethernet interface number",
													.mask			= PRIV(2),
													.tokenize	= NULL,
													.run			= cmd_clr_mac_eth,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #clear mac-address-table dynamic vlan */
									.name			= "vlan",
									.help			= "vlan keyword",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #clear mac-address-table dynamic vlan <1-1094> */
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_clr_mac_vl,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #configure */
			.name			= "configure",
			.help			= "Enter configuration mode",
			.mask			= PRIV(15),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #configure terminal */
					.name			= "terminal",
					.help			= "Configure from the terminal",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= cmd_conf_t,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #disable */
			.name			= "disable",
			.help			= "Turn off privileged commands",
			.mask			= PRIV(2),
			.tokenize	= NULL,
			.run			= cmd_disable,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #disable <1-15> */
					.name			= "<1-15>",
					.help			= "Privilege level to go to",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_disable,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #enable */
			.name			= "enable",
			.help			= "Turn on privileged commands",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_enable,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #enable <1-15> */
					.name			= "<1-15>",
					.help			= "Enable level",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_enable,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #exit */
			.name			= "exit",
			.help			= "Exit from the EXEC",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		{ /* #help */
			.name			= "help",
			.help			= "Description of the interactive help system",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_help,
			.subtree	= NULL
		},

		{ /* #logout */
			.name			= "logout",
			.help			= "Exit from the EXEC",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		{ /* #ping */
			.name			= "ping",
			.help			= "Send echo messages",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #ping WORD */
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_ping,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #reload */
			.name			= "reload",
			.help			= "Halt and perform a cold restart",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_reload,
			.subtree	= NULL
		},

		{ /* #quit */
			.name			= "quit",
			.help			= "Exit from the EXEC",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		{ /* #show */
			.name			= "show",
			.help			= "Show running system information",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #show arp */
					.name			= "arp",
					.help			= "ARP table",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				{ /* #show clock */
					.name			= "clock",
					.help			= "Display the system clock",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				{ /* #show cdp */
					.name			= "cdp",
					.help			= "CDP Information",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_cdp,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show cdp entry */
							.name			= "entry",
							.help			= "Information for specific neighbor entry",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp entry * */
									.name			= "*",
									.help			= "all CDP neighbor entries",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp entry * protocol */
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry * protocol | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * protocol | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * protocol | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * protocol | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * protocol | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * protocol | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * protocol | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * protocol | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * protocol | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp entry * version */
											.name			= "version",
											.help			= "Version information",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry * version protocol */
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_entry,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * version protocol | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * version protocol | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry * version protocol | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry * version protocol | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry * version protocol | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry * version protocol | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry * version protocol | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry * version protocol | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry * version protocol | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry * version | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * version | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * version | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * version | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * version | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * version | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * version | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry * version | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry * version | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp entry * | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry * | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry * | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry * | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry * | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry * | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp entry WORD */
									.name			= "WORD",
									.help			= "Name of CDP neighbor entry",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_entry,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp entry WORD protocol */
											.name			= "protocol",
											.help			= "Protocol information",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry WORD protocol | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD protocol | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD protocol | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD protocol | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD protocol | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD protocol | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD protocol | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD protocol | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD protocol | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp entry WORD version */
											.name			= "version",
											.help			= "Version information",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_entry,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry WORD version protocol */
													.name			= "protocol",
													.help			= "Protocol information",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_entry,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD version protocol | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD version protocol | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry WORD version protocol | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry WORD version protocol | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry WORD version protocol | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry WORD version protocol | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry WORD version protocol | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp entry WORD version protocol | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp entry WORD version protocol | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry WORD version | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD version | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD version | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD version | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD version | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD version | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD version | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show cdp entry WORD version | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp entry WORD version | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp entry WORD | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp entry WORD | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry WORD | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry WORD | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp entry WORD | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp entry WORD | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp holdtime */
							.name			= "holdtime",
							.help			= "Time CDP info kept by neighbors",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_holdtime,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp holdtime | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp holdtime | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp holdtime | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp holdtime | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp holdtime | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp holdtime | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp holdtime | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp holdtime | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp holdtime | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp interface */
							.name			= "interface",
							.help			= "CDP interface status and configuration",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_int,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp interface ethernet */
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp interface ethernet  */
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_int,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp interface | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp interface | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp interface | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp interface | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp interface | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp interface | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp interface | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp interface | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp interface | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp neighbors */
							.name			= "neighbors",
							.help			= "CDP neighbor entries",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_ne,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp neighbors ethernet */
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp neighbors ethernet  */
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_cdp_ne_int,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors ethernet  detail */
													.name			= "detail",
													.help			= "Show detailed information",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_cdp_ne_detail,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp neighbors ethernet  detail | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show cdp neighbors ethernet  detail | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp neighbors ethernet  detail | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp neighbors ethernet  detail | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp neighbors ethernet  detail | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp neighbors ethernet  detail | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp neighbors ethernet  detail | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show cdp neighbors ethernet  detail | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show cdp neighbors ethernet  detail | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp neighbors detail */
									.name			= "detail",
									.help			= "Show detailed information",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_cdp_ne_detail,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp neighbors detail | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors detail | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp neighbors detail | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp neighbors detail | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp neighbors detail | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp neighbors detail | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp neighbors detail | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show cdp neighbors detail | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show cdp neighbors detail | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp neighbors | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp neighbors | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp neighbors | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp neighbors | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp neighbors | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp neighbors | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp run */
							.name			= "run",
							.help			= "CDP process running",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_run,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp run | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp run | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp run | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp run | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp run | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp run | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp run | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp run | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp run | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp timer */
							.name			= "timer",
							.help			= "Time CDP info is resent to neighbors",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_timer,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp timer | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp timer | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp timer | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp timer | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp timer | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp timer | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp timer | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp timer | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp timer | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp traffic */
							.name			= "traffic",
							.help			= "CDP statistics",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_cdp_traffic,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp traffic | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp traffic | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp traffic | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp traffic | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp traffic | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp traffic | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp traffic | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show cdp traffic | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show cdp traffic | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show cdp | */
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show cdp | begin */
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp | begin LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp | exclude */
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp | exclude LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp | include */
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp | include LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show cdp | grep */
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show cdp | grep LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show history */
					.name			= "history",
					.help			= "Display the session command history",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_history,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show history | */
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show history | begin */
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show history | begin LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show history | exclude */
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show history | exclude LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show history | include */
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show history | include LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show history | grep */
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show history | grep LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show interfaces */
					.name			= "interfaces",
					.help			= "Interface status and configuration",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_int,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show interfaces ethernet */
							.name			= "ethernet",
							.help			= "Ethernet IEEE 802.3",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show interfaces ethernet  */
									.name			= "",
									.help			= "Ethernet interface number",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_int_eth,
									.subtree	= NULL
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show interfaces vlan */
							.name			= "vlan",
							.help			= "LMS Vlans",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show interfaces vlan <1-1094> */
									.name			= "<1-1094>",
									.help			= "Vlan interface number",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_int_vlan,
									.subtree	= NULL
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show ip */
					.name			= "ip",
					.help			= "IP information",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_sh_ip,
					.subtree	= NULL
				},

				{ /* #show mac */
					.name			= "mac",
					.help			= "MAC configuration",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show mac address-table */
							.name			= "address-table",
							.help			= "MAC forwarding table",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_show_mac,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac address-table address */
									.name			= "address",
									.help			= "address keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table address H.H.H */
											.name			= "H.H.H",
											.help			= "48 bit mac address",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_addr,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table aging-time */
									.name			= "aging-time",
									.help			= "aging-time keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_mac_age,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table aging-time | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table aging-time | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table aging-time | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table aging-time | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table aging-time | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table aging-time | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table aging-time | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table aging-time | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table aging-time | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table dynamic */
									.name			= "dynamic",
									.help			= "dynamic entry type",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_show_mac,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table dynamic address */
											.name			= "address",
											.help			= "address keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table dynamic address H.H.H */
													.name			= "H.H.H",
													.help			= "48 bit mac address",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_addr,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table dynamic interface */
											.name			= "interface",
											.help			= "interface keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table dynamic interface ethernet */
													.name			= "ethernet",
													.help			= "Ethernet IEEE 802.3",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic interface ethernet  */
															.name			= "",
															.help			= "Ethernet interface number",
															.mask			= PRIV(0),
															.tokenize	= NULL,
															.run			= cmd_sh_mac_eth,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table dynamic interface ethernet  vlan */
																	.name			= "vlan",
																	.help			= "VLAN keyword",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> */
																			.name			= "<1-1094>",
																			.help			= "Vlan interface number",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= cmd_sh_mac_vlan,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | */
																					.name			= "|",
																					.help			= "Output modifiers",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | begin */
																							.name			= "begin",
																							.help			= "Begin with the line that matches",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | begin LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | exclude */
																							.name			= "exclude",
																							.help			= "Exclude lines that match",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | exclude LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | include */
																							.name			= "include",
																							.help			= "Include lines that match",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | include LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | grep */
																							.name			= "grep",
																							.help			= "Linux grep functionality",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table dynamic interface ethernet  vlan <1-1094> | grep LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table dynamic interface ethernet  | */
																	.name			= "|",
																	.help			= "Output modifiers",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic interface ethernet  | begin */
																			.name			= "begin",
																			.help			= "Begin with the line that matches",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table dynamic interface ethernet  | begin LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table dynamic interface ethernet  | exclude */
																			.name			= "exclude",
																			.help			= "Exclude lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table dynamic interface ethernet  | exclude LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table dynamic interface ethernet  | include */
																			.name			= "include",
																			.help			= "Include lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table dynamic interface ethernet  | include LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table dynamic interface ethernet  | grep */
																			.name			= "grep",
																			.help			= "Linux grep functionality",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table dynamic interface ethernet  | grep LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table dynamic vlan */
											.name			= "vlan",
											.help			= "VLAN keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table dynamic vlan <1-1094> */
													.name			= "<1-1094>",
													.help			= "Vlan interface number",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_mac_vlan,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic vlan <1-1094> | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table dynamic vlan <1-1094> | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic vlan <1-1094> | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table dynamic vlan <1-1094> | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic vlan <1-1094> | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table dynamic vlan <1-1094> | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic vlan <1-1094> | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table dynamic vlan <1-1094> | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table dynamic vlan <1-1094> | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table dynamic | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table dynamic | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table dynamic | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table dynamic | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table dynamic | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table dynamic | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table interface */
									.name			= "interface",
									.help			= "interface keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table interface ethernet */
											.name			= "ethernet",
											.help			= "Ethernet IEEE 802.3",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table interface ethernet  */
													.name			= "",
													.help			= "Ethernet interface number",
													.mask			= PRIV(0),
													.tokenize	= NULL,
													.run			= cmd_sh_mac_eth,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table interface ethernet  vlan */
															.name			= "vlan",
															.help			= "VLAN keyword",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table interface ethernet  vlan <1-1094> */
																	.name			= "<1-1094>",
																	.help			= "Vlan interface number",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= cmd_sh_mac_vlan,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table interface ethernet  vlan <1-1094> | */
																			.name			= "|",
																			.help			= "Output modifiers",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table interface ethernet  vlan <1-1094> | begin */
																					.name			= "begin",
																					.help			= "Begin with the line that matches",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table interface ethernet  vlan <1-1094> | begin LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac address-table interface ethernet  vlan <1-1094> | exclude */
																					.name			= "exclude",
																					.help			= "Exclude lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table interface ethernet  vlan <1-1094> | exclude LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac address-table interface ethernet  vlan <1-1094> | include */
																					.name			= "include",
																					.help			= "Include lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table interface ethernet  vlan <1-1094> | include LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac address-table interface ethernet  vlan <1-1094> | grep */
																					.name			= "grep",
																					.help			= "Linux grep functionality",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table interface ethernet  vlan <1-1094> | grep LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac address-table interface ethernet  | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table interface ethernet  | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table interface ethernet  | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table interface ethernet  | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table interface ethernet  | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table interface ethernet  | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table interface ethernet  | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table interface ethernet  | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table interface ethernet  | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table static */
									.name			= "static",
									.help			= "static entry type",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_show_mac,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table static address */
											.name			= "address",
											.help			= "address keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table static address H.H.H */
													.name			= "H.H.H",
													.help			= "48 bit mac address",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_addr,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table static interface */
											.name			= "interface",
											.help			= "interface keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table static interface ethernet */
													.name			= "ethernet",
													.help			= "Ethernet IEEE 802.3",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static interface ethernet  */
															.name			= "",
															.help			= "Ethernet interface number",
															.mask			= PRIV(0),
															.tokenize	= NULL,
															.run			= cmd_sh_mac_eth,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table static interface ethernet  vlan */
																	.name			= "vlan",
																	.help			= "VLAN keyword",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static interface ethernet  vlan <1-1094> */
																			.name			= "<1-1094>",
																			.help			= "Vlan interface number",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= cmd_sh_mac_vlan,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table static interface ethernet  vlan <1-1094> | */
																					.name			= "|",
																					.help			= "Output modifiers",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac address-table static interface ethernet  vlan <1-1094> | begin */
																							.name			= "begin",
																							.help			= "Begin with the line that matches",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table static interface ethernet  vlan <1-1094> | begin LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table static interface ethernet  vlan <1-1094> | exclude */
																							.name			= "exclude",
																							.help			= "Exclude lines that match",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table static interface ethernet  vlan <1-1094> | exclude LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table static interface ethernet  vlan <1-1094> | include */
																							.name			= "include",
																							.help			= "Include lines that match",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table static interface ethernet  vlan <1-1094> | include LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						{ /* #show mac address-table static interface ethernet  vlan <1-1094> | grep */
																							.name			= "grep",
																							.help			= "Linux grep functionality",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= (struct menu_node[]) { /*{{{*/
																								{ /* #show mac address-table static interface ethernet  vlan <1-1094> | grep LINE */
																									.name			= "LINE",
																									.help			= "Regular Expression",
																									.mask			= PRIV(1),
																									.tokenize	= NULL,
																									.run			= NULL,
																									.subtree	= NULL
																								},

																								NULL_MENU_NODE
																							} /*}}}*/
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table static interface ethernet  | */
																	.name			= "|",
																	.help			= "Output modifiers",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static interface ethernet  | begin */
																			.name			= "begin",
																			.help			= "Begin with the line that matches",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table static interface ethernet  | begin LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table static interface ethernet  | exclude */
																			.name			= "exclude",
																			.help			= "Exclude lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table static interface ethernet  | exclude LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table static interface ethernet  | include */
																			.name			= "include",
																			.help			= "Include lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table static interface ethernet  | include LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac address-table static interface ethernet  | grep */
																			.name			= "grep",
																			.help			= "Linux grep functionality",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac address-table static interface ethernet  | grep LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table static vlan */
											.name			= "vlan",
											.help			= "VLAN keyword",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table static vlan <1-1094> */
													.name			= "<1-1094>",
													.help			= "Vlan interface number",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= cmd_sh_mac_vlan,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static vlan <1-1094> | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table static vlan <1-1094> | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static vlan <1-1094> | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table static vlan <1-1094> | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static vlan <1-1094> | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table static vlan <1-1094> | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static vlan <1-1094> | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac address-table static vlan <1-1094> | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac address-table static vlan <1-1094> | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table static | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table static | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table static | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table static | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac address-table static | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table static | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table vlan */
									.name			= "vlan",
									.help			= "VLAN keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table vlan <1-1094> */
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_mac_vlan,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table vlan <1-1094> | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac address-table vlan <1-1094> | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table vlan <1-1094> | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac address-table vlan <1-1094> | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table vlan <1-1094> | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac address-table vlan <1-1094> | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table vlan <1-1094> | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac address-table vlan <1-1094> | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac address-table vlan <1-1094> | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac address-table | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac address-table | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac address-table | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac address-table | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show mac-address-table */
					.name			= "mac-address-table",
					.help			= "MAC forwarding table",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_mac,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show mac-address-table address */
							.name			= "address",
							.help			= "address keyword",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table address H.H.H */
									.name			= "H.H.H",
									.help			= "48 bit mac address",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_addr,
									.subtree	= NULL
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table aging-time */
							.name			= "aging-time",
							.help			= "aging-time keyword",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_sh_mac_age,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table aging-time | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table aging-time | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table aging-time | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table aging-time | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table aging-time | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table aging-time | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table aging-time | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table aging-time | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table aging-time | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table dynamic */
							.name			= "dynamic",
							.help			= "dynamic entry type",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_show_mac,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table dynamic address */
									.name			= "address",
									.help			= "address keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table dynamic address H.H.H */
											.name			= "H.H.H",
											.help			= "48 bit mac address",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_addr,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table dynamic interface */
									.name			= "interface",
									.help			= "interface keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table dynamic interface ethernet */
											.name			= "ethernet",
											.help			= "Ethernet IEEE 802.3",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic interface ethernet  */
													.name			= "",
													.help			= "Ethernet interface number",
													.mask			= PRIV(0),
													.tokenize	= NULL,
													.run			= cmd_sh_mac_eth,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table dynamic interface ethernet  vlan */
															.name			= "vlan",
															.help			= "VLAN keyword",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> */
																	.name			= "<1-1094>",
																	.help			= "Vlan interface number",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= cmd_sh_mac_vlan,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | */
																			.name			= "|",
																			.help			= "Output modifiers",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | begin */
																					.name			= "begin",
																					.help			= "Begin with the line that matches",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | begin LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | exclude */
																					.name			= "exclude",
																					.help			= "Exclude lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | exclude LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | include */
																					.name			= "include",
																					.help			= "Include lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | include LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | grep */
																					.name			= "grep",
																					.help			= "Linux grep functionality",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table dynamic interface ethernet  vlan <1-1094> | grep LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table dynamic interface ethernet  | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic interface ethernet  | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table dynamic interface ethernet  | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table dynamic interface ethernet  | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table dynamic interface ethernet  | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table dynamic interface ethernet  | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table dynamic interface ethernet  | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table dynamic interface ethernet  | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table dynamic interface ethernet  | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table dynamic vlan */
									.name			= "vlan",
									.help			= "VLAN keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table dynamic vlan <1-1094> */
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_mac_vlan,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic vlan <1-1094> | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table dynamic vlan <1-1094> | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic vlan <1-1094> | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table dynamic vlan <1-1094> | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic vlan <1-1094> | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table dynamic vlan <1-1094> | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic vlan <1-1094> | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table dynamic vlan <1-1094> | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table dynamic vlan <1-1094> | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table dynamic | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table dynamic | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table dynamic | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table dynamic | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table dynamic | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table dynamic | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table interface */
							.name			= "interface",
							.help			= "interface keyword",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table interface ethernet */
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table interface ethernet  */
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(0),
											.tokenize	= NULL,
											.run			= cmd_sh_mac_eth,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table interface ethernet  vlan */
													.name			= "vlan",
													.help			= "VLAN keyword",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table interface ethernet  vlan <1-1094> */
															.name			= "<1-1094>",
															.help			= "Vlan interface number",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= cmd_sh_mac_vlan,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table interface ethernet  vlan <1-1094> | */
																	.name			= "|",
																	.help			= "Output modifiers",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table interface ethernet  vlan <1-1094> | begin */
																			.name			= "begin",
																			.help			= "Begin with the line that matches",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table interface ethernet  vlan <1-1094> | begin LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac-address-table interface ethernet  vlan <1-1094> | exclude */
																			.name			= "exclude",
																			.help			= "Exclude lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table interface ethernet  vlan <1-1094> | exclude LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac-address-table interface ethernet  vlan <1-1094> | include */
																			.name			= "include",
																			.help			= "Include lines that match",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table interface ethernet  vlan <1-1094> | include LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		{ /* #show mac-address-table interface ethernet  vlan <1-1094> | grep */
																			.name			= "grep",
																			.help			= "Linux grep functionality",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table interface ethernet  vlan <1-1094> | grep LINE */
																					.name			= "LINE",
																					.help			= "Regular Expression",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= NULL
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac-address-table interface ethernet  | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table interface ethernet  | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table interface ethernet  | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table interface ethernet  | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table interface ethernet  | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table interface ethernet  | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table interface ethernet  | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table interface ethernet  | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table interface ethernet  | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table static */
							.name			= "static",
							.help			= "static entry type",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_show_mac,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table static address */
									.name			= "address",
									.help			= "address keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table static address H.H.H */
											.name			= "H.H.H",
											.help			= "48 bit mac address",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_addr,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table static interface */
									.name			= "interface",
									.help			= "interface keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table static interface ethernet */
											.name			= "ethernet",
											.help			= "Ethernet IEEE 802.3",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static interface ethernet  */
													.name			= "",
													.help			= "Ethernet interface number",
													.mask			= PRIV(0),
													.tokenize	= NULL,
													.run			= cmd_sh_mac_eth,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table static interface ethernet  vlan */
															.name			= "vlan",
															.help			= "VLAN keyword",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static interface ethernet  vlan <1-1094> */
																	.name			= "<1-1094>",
																	.help			= "Vlan interface number",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= cmd_sh_mac_vlan,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | */
																			.name			= "|",
																			.help			= "Output modifiers",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= (struct menu_node[]) { /*{{{*/
																				{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | begin */
																					.name			= "begin",
																					.help			= "Begin with the line that matches",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | begin LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | exclude */
																					.name			= "exclude",
																					.help			= "Exclude lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | exclude LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | include */
																					.name			= "include",
																					.help			= "Include lines that match",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | include LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | grep */
																					.name			= "grep",
																					.help			= "Linux grep functionality",
																					.mask			= PRIV(1),
																					.tokenize	= NULL,
																					.run			= NULL,
																					.subtree	= (struct menu_node[]) { /*{{{*/
																						{ /* #show mac-address-table static interface ethernet  vlan <1-1094> | grep LINE */
																							.name			= "LINE",
																							.help			= "Regular Expression",
																							.mask			= PRIV(1),
																							.tokenize	= NULL,
																							.run			= NULL,
																							.subtree	= NULL
																						},

																						NULL_MENU_NODE
																					} /*}}}*/
																				},

																				NULL_MENU_NODE
																			} /*}}}*/
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table static interface ethernet  | */
															.name			= "|",
															.help			= "Output modifiers",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static interface ethernet  | begin */
																	.name			= "begin",
																	.help			= "Begin with the line that matches",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table static interface ethernet  | begin LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table static interface ethernet  | exclude */
																	.name			= "exclude",
																	.help			= "Exclude lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table static interface ethernet  | exclude LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table static interface ethernet  | include */
																	.name			= "include",
																	.help			= "Include lines that match",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table static interface ethernet  | include LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																{ /* #show mac-address-table static interface ethernet  | grep */
																	.name			= "grep",
																	.help			= "Linux grep functionality",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node[]) { /*{{{*/
																		{ /* #show mac-address-table static interface ethernet  | grep LINE */
																			.name			= "LINE",
																			.help			= "Regular Expression",
																			.mask			= PRIV(1),
																			.tokenize	= NULL,
																			.run			= NULL,
																			.subtree	= NULL
																		},

																		NULL_MENU_NODE
																	} /*}}}*/
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table static vlan */
									.name			= "vlan",
									.help			= "VLAN keyword",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table static vlan <1-1094> */
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= cmd_sh_mac_vlan,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static vlan <1-1094> | */
													.name			= "|",
													.help			= "Output modifiers",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table static vlan <1-1094> | begin */
															.name			= "begin",
															.help			= "Begin with the line that matches",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static vlan <1-1094> | begin LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table static vlan <1-1094> | exclude */
															.name			= "exclude",
															.help			= "Exclude lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static vlan <1-1094> | exclude LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table static vlan <1-1094> | include */
															.name			= "include",
															.help			= "Include lines that match",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static vlan <1-1094> | include LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														{ /* #show mac-address-table static vlan <1-1094> | grep */
															.name			= "grep",
															.help			= "Linux grep functionality",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node[]) { /*{{{*/
																{ /* #show mac-address-table static vlan <1-1094> | grep LINE */
																	.name			= "LINE",
																	.help			= "Regular Expression",
																	.mask			= PRIV(1),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= NULL
																},

																NULL_MENU_NODE
															} /*}}}*/
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table static | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table static | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table static | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table static | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show mac-address-table static | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table static | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table vlan */
							.name			= "vlan",
							.help			= "VLAN keyword",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table vlan <1-1094> */
									.name			= "<1-1094>",
									.help			= "Vlan interface number",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= cmd_sh_mac_vlan,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table vlan <1-1094> | */
											.name			= "|",
											.help			= "Output modifiers",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show mac-address-table vlan <1-1094> | begin */
													.name			= "begin",
													.help			= "Begin with the line that matches",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table vlan <1-1094> | begin LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac-address-table vlan <1-1094> | exclude */
													.name			= "exclude",
													.help			= "Exclude lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table vlan <1-1094> | exclude LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac-address-table vlan <1-1094> | include */
													.name			= "include",
													.help			= "Include lines that match",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table vlan <1-1094> | include LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												{ /* #show mac-address-table vlan <1-1094> | grep */
													.name			= "grep",
													.help			= "Linux grep functionality",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node[]) { /*{{{*/
														{ /* #show mac-address-table vlan <1-1094> | grep LINE */
															.name			= "LINE",
															.help			= "Regular Expression",
															.mask			= PRIV(1),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= NULL
														},

														NULL_MENU_NODE
													} /*}}}*/
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show mac-address-table | */
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show mac-address-table | begin */
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table | begin LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table | exclude */
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table | exclude LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table | include */
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table | include LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show mac-address-table | grep */
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show mac-address-table | grep LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show privilege */
					.name			= "privilege",
					.help			= "Show current privilege level",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= cmd_show_priv,
					.subtree	= NULL
				},

				{ /* #show running-config */
					.name			= "running-config",
					.help			= "Current operating configuration",
					.mask			= PRIV(2),
					.tokenize	= NULL,
					.run			= cmd_show_run,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show running-config interface */
							.name			= "interface",
							.help			= "Show interface configuration",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show running-config interface ethernet */
									.name			= "ethernet",
									.help			= "Ethernet IEEE 802.3",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config interface ethernet  */
											.name			= "",
											.help			= "Ethernet interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_run_eth,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show running-config interface vlan */
									.name			= "vlan",
									.help			= "LMS Vlans",
									.mask			= PRIV(2),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config interface vlan <1-1094> */
											.name			= "<1-1094>",
											.help			= "Vlan interface number",
											.mask			= PRIV(2),
											.tokenize	= NULL,
											.run			= cmd_run_vlan,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show running-config | */
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show running-config | begin */
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config | begin LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show running-config | exclude */
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config | exclude LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show running-config | include */
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config | include LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show running-config | grep */
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show running-config | grep LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				{ /* #show sessions */
					.name			= "sessions",
					.help			= "Information about Telnet connections",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				{ /* #show startup-config */
					.name			= "startup-config",
					.help			= "Contents of startup configuration",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_start,
					.subtree	= NULL
				},

				{ /* #show users */
					.name			= "users",
					.help			= "Display information about terminal lines",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= NULL
				},

				{ /* #show version */
					.name			= "version",
					.help			= "System hardware and software status",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_ver,
					.subtree	= NULL
				},

				{ /* #show vlan */
					.name			= "vlan",
					.help			= "VTP VLAN status",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_show_vlan,
					.subtree	= (struct menu_node[]) { /*{{{*/
						{ /* #show vlan brief */
							.name			= "brief",
							.help			= "VTP all VLAN status in brief",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= cmd_show_vlan,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show vlan brief | */
									.name			= "|",
									.help			= "Output modifiers",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show vlan brief | begin */
											.name			= "begin",
											.help			= "Begin with the line that matches",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show vlan brief | begin LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show vlan brief | exclude */
											.name			= "exclude",
											.help			= "Exclude lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show vlan brief | exclude LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show vlan brief | include */
											.name			= "include",
											.help			= "Include lines that match",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show vlan brief | include LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										{ /* #show vlan brief | grep */
											.name			= "grep",
											.help			= "Linux grep functionality",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node[]) { /*{{{*/
												{ /* #show vlan brief | grep LINE */
													.name			= "LINE",
													.help			= "Regular Expression",
													.mask			= PRIV(1),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= NULL
												},

												NULL_MENU_NODE
											} /*}}}*/
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						{ /* #show vlan | */
							.name			= "|",
							.help			= "Output modifiers",
							.mask			= PRIV(1),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node[]) { /*{{{*/
								{ /* #show vlan | begin */
									.name			= "begin",
									.help			= "Begin with the line that matches",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show vlan | begin LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show vlan | exclude */
									.name			= "exclude",
									.help			= "Exclude lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show vlan | exclude LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show vlan | include */
									.name			= "include",
									.help			= "Include lines that match",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show vlan | include LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								{ /* #show vlan | grep */
									.name			= "grep",
									.help			= "Linux grep functionality",
									.mask			= PRIV(1),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node[]) { /*{{{*/
										{ /* #show vlan | grep LINE */
											.name			= "LINE",
											.help			= "Regular Expression",
											.mask			= PRIV(1),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= NULL
										},

										NULL_MENU_NODE
									} /*}}}*/
								},

								NULL_MENU_NODE
							} /*}}}*/
						},

						NULL_MENU_NODE
					} /*}}}*/
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #traceroute */
			.name			= "traceroute",
			.help			= "Trace route to destination",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #traceroute WORD */
					.name			= "WORD",
					.help			= "Ping destination address or hostname",
					.mask			= PRIV(1),
					.tokenize	= NULL,
					.run			= cmd_trace,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		{ /* #where */
			.name			= "where",
			.help			= "List active connections",
			.mask			= PRIV(1),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= NULL
		},

		{ /* #write */
			.name			= "write",
			.help			= "Write running configuration to memory",
			.mask			= PRIV(15),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node[]) { /*{{{*/
				{ /* #write memory */
					.name			= "memory",
					.help			= "Write to NV memory",
					.mask			= PRIV(15),
					.tokenize	= NULL,
					.run			= cmd_wrme,
					.subtree	= NULL
				},

				NULL_MENU_NODE
			} /*}}}*/
		},

		NULL_MENU_NODE
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/
