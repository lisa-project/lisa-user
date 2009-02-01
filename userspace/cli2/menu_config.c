#include "cli.h"
#include "swcli_common.h"
#include "menu_interface.h"

int cmd_cdp_version(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_holdtime(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_timer(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_setenpw(struct cli_context *, int, char **, struct menu_node **);
int cmd_setenpwlev(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);
int cmd_hostname(struct cli_context *, int, char **, struct menu_node **);
int cmd_int_any(struct cli_context *, int, char **, struct menu_node **);
int cmd_linevty(struct cli_context *, int, char **, struct menu_node **);
int cmd_set_aging(struct cli_context *, int, char **, struct menu_node **);
int cmd_macstatic(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_cdp_v2(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_cdp_run(struct cli_context *, int, char **, struct menu_node **);
int cmd_noensecret(struct cli_context *, int, char **, struct menu_node **);
int cmd_noensecret_lev(struct cli_context *, int, char **, struct menu_node **);
int cmd_nohostname(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_int_eth(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_int_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_int_any(struct cli_context *, int, char **, struct menu_node **);
int cmd_set_noaging(struct cli_context *, int, char **, struct menu_node **);
int cmd_novlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_vlan(struct cli_context *, int, char **, struct menu_node **);
int dump_args(struct cli_context *, int, char **, struct menu_node **);

static struct menu_node *if_subtree[] = {
	IF_ETHER(NULL, cmd_int_any, NULL),
	IF_VLAN(NULL, cmd_int_any, NULL),
	IF_NETDEV(NULL, cmd_int_any, NULL),
	NULL
};

struct menu_node config_interface = IF_MENU_NODE(if_subtree, "Select an interface to configure");

struct menu_node config_main = {
	/* Root node, .name is used as prompt */
	.name			= "config",
	.subtree	= (struct menu_node *[]) {
		/* #cdp */
		& (struct menu_node){
			.name			= "cdp",
			.help			= "Global CDP configuration subcommands",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #cdp advertise-v2 */
				& (struct menu_node){
					.name			= "advertise-v2",
					.help			= "CDP sends version-2 advertisements",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_cdp_version,
					.subtree	= NULL
				},

				/* #cdp holdtime */
				& (struct menu_node){
					.name			= "holdtime",
					.help			= "Specify the holdtime (in sec) to be sent in packets",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #cdp holdtime <10-255> */
						& (struct menu_node){
							.name			= "<10-255>",
							.help			= "Length  of time  (in sec) that receiver must keep this packet",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_cdp_holdtime,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #cdp timer */
				& (struct menu_node){
					.name			= "timer",
					.help			= "Specify the rate at which CDP packets are sent (in sec)",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #cdp timer <5-254> */
						& (struct menu_node){
							.name			= "<5-254>",
							.help			= "Rate at which CDP packets are sent (in  sec)",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_cdp_timer,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #cdp run */
				& (struct menu_node){
					.name			= "run",
					.help			= "",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_cdp_run,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #enable */
		& (struct menu_node){
			.name			= "enable",
			.help			= "Modify enable password parameters",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #enable secret */
				& (struct menu_node){
					.name			= "secret",
					.help			= "Assign the privileged level secret",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= swcli_tokenize_line_mixed,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #enable secret 0 */
						& (struct menu_node){
							.name			= "0",
							.help			= "Specifies an UNENCRYPTED password will follow",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= swcli_tokenize_line,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #enable secret 0 LINE */
								& (struct menu_node){
									.name			= "LINE",
									.help			= "The UNENCRYPTED (cleartext) 'enable' secret",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= dump_args, //cmd_setenpw,
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						/* #enable secret 5 */
						& (struct menu_node){
							.name			= "5",
							.help			= "Specifies an ENCRYPTED secret will follow",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #enable secret 5 LINE */
								& (struct menu_node){
									.name			= "LINE",
									.help			= "The ENCRYPTED 'enable' secret string",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= cmd_setenpw,
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						/* #enable secret LINE */
						& (struct menu_node){
							.name			= "LINE",
							.help			= "The UNENCRYPTED (cleartext) 'enable' secret",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= dump_args, //cmd_setenpw,
							.subtree	= NULL
						},

						/* #enable secret level */
						& (struct menu_node){
							.name			= "level",
							.help			= "Set exec level password",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #enable secret level <1-15> */
								& (struct menu_node){
									.name			= "<1-15>",
									.help			= "Level number",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #enable secret level <1-15> 0 */
										& (struct menu_node){
											.name			= "0",
											.help			= "Specifies an UNENCRYPTED password will follow",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #enable secret level <1-15> 0 LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "The UNENCRYPTED (cleartext) 'enable' secret",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= cmd_setenpwlev,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #enable secret level <1-15> 5 */
										& (struct menu_node){
											.name			= "5",
											.help			= "Specifies an ENCRYPTED secret will follow",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #enable secret level <1-15> 5 LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "The ENCRYPTED 'enable' secret string",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= cmd_setenpwlev,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #enable secret level <1-15> LINE */
										& (struct menu_node){
											.name			= "LINE",
											.help			= "The UNENCRYPTED (cleartext) 'enable' secret",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= cmd_setenpwlev,
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

		/* #end */
		& (struct menu_node){
			.name			= "end",
			.help			= "Exit from configure mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_end,
			.subtree	= NULL
		},

		/* #exit */
		& (struct menu_node){
			.name			= "exit",
			.help			= "Exit from configure mode",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_end,
			.subtree	= NULL
		},

		/* #hostname */
		& (struct menu_node){
			.name			= "hostname",
			.help			= "Set system's network name",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #hostname WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "This system's network name",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_hostname,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #interface */
		& config_interface,

		/* #line */
		& (struct menu_node){
			.name			= "line",
			.help			= "Configure a terminal line",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #line vty */
				& (struct menu_node){
					.name			= "vty",
					.help			= "Virtual terminal",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #line vty <0-15> */
						& (struct menu_node){
							.name			= "<0-15>",
							.help			= "First Line number",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #line vty <0-15>  */
								& (struct menu_node){
									.name			= "",
									.help			= "Last Line number",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= cmd_linevty,
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

		/* #mac-address-table */
		& (struct menu_node){
			.name			= "mac-address-table",
			.help			= "Configure the MAC address table",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #mac-address-table aging-time */
				& (struct menu_node){
					.name			= "aging-time",
					.help			= "Set MAC address table entry maximum age",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #mac-address-table aging-time <10-1000000> */
						& (struct menu_node){
							.name			= "<10-1000000>",
							.help			= "Maximum age in seconds",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_set_aging,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #mac-address-table static */
				& (struct menu_node){
					.name			= "static",
					.help			= "static keyword",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #mac-address-table static H.H.H */
						& (struct menu_node){
							.name			= "H.H.H",
							.help			= "48 bit mac address",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #mac-address-table static H.H.H vlan */
								& (struct menu_node){
									.name			= "vlan",
									.help			= "VLAN keyword",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #mac-address-table static H.H.H vlan <1-1094> */
										& (struct menu_node){
											.name			= "<1-1094>",
											.help			= "VLAN id of mac address table",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #mac-address-table static H.H.H vlan <1-1094> interface */
												& (struct menu_node){
													.name			= "interface",
													.help			= "interface",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #mac-address-table static H.H.H vlan <1-1094> interface ethernet */
														& (struct menu_node){
															.name			= "ethernet",
															.help			= "Ethernet IEEE 802.3",
															.mask			= CLI_MASK(PRIV(15)),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #mac-address-table static H.H.H vlan <1-1094> interface ethernet  */
																& (struct menu_node){
																	.name			= "",
																	.help			= "Ethernet interface number",
																	.mask			= CLI_MASK(PRIV(15)),
																	.tokenize	= NULL,
																	.run			= cmd_macstatic,
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

						NULL
					} /*}}}*/
				},

				NULL
			} /*}}}*/
		},

		/* #no */
		& (struct menu_node){
			.name			= "no",
			.help			= "Negate a command or set its defaults",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #no cdp */
				& (struct menu_node){
					.name			= "cdp",
					.help			= "Global CDP configuration subcommands",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no cdp advertise-v2 */
						& (struct menu_node){
							.name			= "advertise-v2",
							.help			= "CDP sends version-2 advertisements",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_no_cdp_v2,
							.subtree	= NULL
						},

						/* #no cdp run */
						& (struct menu_node){
							.name			= "run",
							.help			= "",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_no_cdp_run,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #no enable */
				& (struct menu_node){
					.name			= "enable",
					.help			= "Modify enable password parameters",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no enable secret */
						& (struct menu_node){
							.name			= "secret",
							.help			= "Assign the privileged level secret",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_noensecret,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no enable secret level */
								& (struct menu_node){
									.name			= "level",
									.help			= "Set exec level password",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no enable secret level <1-15> */
										& (struct menu_node){
											.name			= "<1-15>",
											.help			= "Level number",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= cmd_noensecret_lev,
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

				/* #no hostname */
				& (struct menu_node){
					.name			= "hostname",
					.help			= "Set system's network name",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_nohostname,
					.subtree	= NULL
				},

				/* #no interface */
				&config_interface,

				/* #no mac-address-table */
				& (struct menu_node){
					.name			= "mac-address-table",
					.help			= "Configure the MAC address table",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no mac-address-table aging-time */
						& (struct menu_node){
							.name			= "aging-time",
							.help			= "Set MAC address table entry maximum age",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_set_noaging,
							.subtree	= NULL
						},

						/* #no mac-address-table static */
						& (struct menu_node){
							.name			= "static",
							.help			= "static keyword",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no mac-address-table static H.H.H */
								& (struct menu_node){
									.name			= "H.H.H",
									.help			= "48 bit mac address",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no mac-address-table static H.H.H vlan */
										& (struct menu_node){
											.name			= "vlan",
											.help			= "VLAN keyword",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #no mac-address-table static H.H.H vlan <1-1094> */
												& (struct menu_node){
													.name			= "<1-1094>",
													.help			= "VLAN id of mac address table",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #no mac-address-table static H.H.H vlan <1-1094> interface */
														& (struct menu_node){
															.name			= "interface",
															.help			= "interface",
															.mask			= CLI_MASK(PRIV(15)),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #no mac-address-table static H.H.H vlan <1-1094> interface ethernet */
																& (struct menu_node){
																	.name			= "ethernet",
																	.help			= "Ethernet IEEE 802.3",
																	.mask			= CLI_MASK(PRIV(15)),
																	.tokenize	= NULL,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #no mac-address-table static H.H.H vlan <1-1094> interface ethernet  */
																		& (struct menu_node){
																			.name			= "",
																			.help			= "Ethernet interface number",
																			.mask			= CLI_MASK(PRIV(15)),
																			.tokenize	= NULL,
																			.run			= cmd_macstatic,
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

								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				/* #no vlan */
				& (struct menu_node){
					.name			= "vlan",
					.help			= "Vlan commands",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no vlan WORD */
						& (struct menu_node){
							.name			= "WORD",
							.help			= "ISL VLAN IDs 1-4094",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_novlan,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				NULL
			} /*}}}*/
		},

		/* #vlan */
		& (struct menu_node){
			.name			= "vlan",
			.help			= "Vlan commands",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #vlan WORD */
				& (struct menu_node){
					.name			= "WORD",
					.help			= "ISL VLAN IDs 1-4094",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= NULL,
					.run			= cmd_vlan,
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
