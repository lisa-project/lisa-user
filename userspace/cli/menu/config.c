#include "swcli.h"
#include "config.h"

static struct menu_node *if_subtree[] = {
	IF_ETHER(NULL, cmd_int_any, NULL),
	IF_VLAN(NULL, cmd_int_any, NULL),
	IF_NETDEV(NULL, cmd_int_any, NULL),
	NULL
};

struct menu_node config_interface = IF_MENU_NODE(if_subtree, "Select an interface to configure");

static struct menu_node *eth_tree[] = {
		IF_ETHER(NULL, cmd_macstatic, NULL, 15),
		IF_NETDEV(NULL, cmd_macstatic, NULL, 15),
		NULL
};

static struct menu_node *if_tree[] = {
	 & (struct menu_node) IF_MENU_NODE(eth_tree, "interface", 15),
	 NULL
};

static struct menu_node *vlan_tree[] = {
		& (struct menu_node) VLAN_MENU_NODE(if_tree, NULL, NULL, 15),
		NULL
};

static struct menu_node mac_addr_table_static = {
		.name			= "static",
		.help			= "static keyword",
		.mask			= CLI_MASK(PRIV(15)),
		.tokenize	= swcli_tokenize_mac,
		.run			= NULL,
		.subtree  = (struct menu_node *[]) {
				&(struct menu_node) {
						.name			= "H.H.H",
						.help			=	"48 bit mac address",
						.mask			= CLI_MASK(PRIV(15)),
						.tokenize = NULL,
						.run			= NULL,
						.subtree	= vlan_tree
				},
				NULL
		}
};


// FIXME FIXME FIXME
// #1 - Cisco accepts multiple vlans at the same time (vlan list, similar
//      to the one at "switchport trunk allowed vlans")
// #2 - If other nodes must also be included in "vlan" subtree, there are
//      two issues:
//      a. we need a mixed vlan id / regular node tokenizer; one suitable
//         is swcli_tokenize_word_mixed(), and this also leads back to #1
//      b. we cannot have a common "vlan" node for all places
//         (config#/vlan, config#/no vlan, config-vlan#/vlan etc), so the
//         parent "vlan" node must be replicated for each place where it
//         is needed
struct menu_node config_vlan = {
	.name			= "vlan",
	.help			= "Vlan commands",
	.mask			= CLI_MASK(PRIV(15)),
	.tokenize	= swcli_tokenize_number,
	.run			= NULL,
	.subtree	= (struct menu_node *[]) { /*{{{*/
		/* #vlan WORD */
		& (struct menu_node){
			.name			= "WORD",
			.help			= "ISL VLAN IDs 1-4094",
			.mask			= CLI_MASK(PRIV(15)),
			.tokenize	= NULL,
			.run			= cmd_vlan,
			.subtree	= NULL,
			.priv		= (int []) {VALID_LIMITS, 1, 4094}
		},

		NULL
	} /*}}}*/
};

struct menu_node config_main = {
	/* Root node, .name is used as prompt */
	.name			= "config",
	.subtree	= (struct menu_node *[]) {
		/* #ip */
		& (struct menu_node){
			.name			= "ip",
			.help			= "Global IP configuration subcommands",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #ip igmp */
				& (struct menu_node){
					.name			= "igmp",
					.help			= "IGMP global configuration",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #ip igmp snooping */
						& (struct menu_node){
							.name			= "snooping",
							.help			= "Global IGMP Snooping enable for LiSA Vlans",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_ip_igmp_snooping,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #ip igmp snooping vlan*/
								& (struct menu_node){
									.name			= "vlan",
									.help			= "IGMP Snooping enable for LiSA VLAN",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= swcli_tokenize_number,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #ip igmp snooping vlan <number>*/
										& (struct menu_node){
											.name			= "<1-4094>",
											.help			= "Vlan number",
											.mask			= CLI_MASK(PRIV(1)),
											.tokenize	= NULL,
											.run			= cmd_ip_igmp_snooping,
											.priv			= (int []) {VALID_LIMITS, 1, 4094}, 
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #ip igmp snooping vlan <number> mrouter*/
												& (struct menu_node){
													.name			= "mrouter",
													.help			= "Configure an L2 port as a multicast router port",
													.mask			= CLI_MASK(PRIV(1)),
													.tokenize	= NULL,
													.run			= NULL,
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #ip igmp snooping vlan <number> mrouter interface*/
														& (struct menu_node){
															.name			= "interface",
															.help			= "next-hop interface to mrouter",
															.mask			= CLI_MASK(PRIV(1)),
															.tokenize	= if_tok_if,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #ip igmp snooping vlan <number> mrouter interface Ethernet <number>*/
																IF_ETHER(NULL, cmd_add_mrouter, NULL),
																IF_NETDEV(NULL, cmd_add_mrouter, NULL),

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
					.run			= cmd_cdp_v2,
					.subtree	= NULL
				},

				/* #cdp holdtime */
				& (struct menu_node){
					.name			= "holdtime",
					.help			= "Specify the holdtime (in sec) to be sent in packets",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= swcli_tokenize_number,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #cdp holdtime <10-255> */
						& (struct menu_node){
							.name			= "<10-255>",
							.help			= "Length  of time  (in sec) that receiver must keep this packet",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_cdp_holdtime,
							.subtree	= NULL,
							.priv			= (int []) {VALID_LIMITS, 10, 255}
						},

						NULL
					} /*}}}*/
				},

				/* #cdp timer */
				& (struct menu_node){
					.name			= "timer",
					.help			= "Specify the rate at which CDP packets are sent (in sec)",
					.mask			= CLI_MASK(PRIV(15)),
					.tokenize	= swcli_tokenize_number,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #cdp timer <5-254> */
						& (struct menu_node){
							.name			= "<5-254>",
							.help			= "Rate at which CDP packets are sent (in  sec)",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_cdp_timer,
							.subtree	= NULL,
							.priv			= (int []) {VALID_LIMITS, 5, 254}
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
									.run			= cmd_setenpw,
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
							.tokenize	= swcli_tokenize_line,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #enable secret 5 LINE */
								& (struct menu_node){
									.name			= "LINE",
									.help			= "The ENCRYPTED 'enable' secret string",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= cmd_setenpw_encrypted,
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
							.run			= cmd_setenpw,
							.subtree	= NULL
						},

						/* #enable secret level */
						& (struct menu_node){
							.name			= "level",
							.help			= "Set exec level password",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= swcli_tokenize_number,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #enable secret level <1-15> */
								& (struct menu_node){
									.name			= "<1-15>",
									.help			= "Level number",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= swcli_tokenize_line_mixed,
									.run			= NULL,
									.priv			= (int []) {VALID_LIMITS, 1, 15},
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #enable secret level <1-15> 0 */
										& (struct menu_node){
											.name			= "0",
											.help			= "Specifies an UNENCRYPTED password will follow",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= swcli_tokenize_line,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #enable secret level <1-15> 0 LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "The UNENCRYPTED (cleartext) 'enable' secret",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= cmd_setenpw,
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
											.tokenize	= swcli_tokenize_line,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #enable secret level <1-15> 5 LINE */
												& (struct menu_node){
													.name			= "LINE",
													.help			= "The ENCRYPTED 'enable' secret string",
													.mask			= CLI_MASK(PRIV(15)),
													.tokenize	= NULL,
													.run			= cmd_setenpw_encrypted,
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
											.run			= cmd_setenpw,
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
			.tokenize	= swcli_tokenize_line,
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
					.tokenize	= swcli_tokenize_number,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #line vty <0-15> */
						& (struct menu_node){
							.name			= "<0-15>",
							.help			= "First Line number",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= swcli_tokenize_number,
							.run			= NULL,
							.priv			= (int []) {VALID_LIMITS, 0, 15},
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #line vty <0-15>  */
								& (struct menu_node){
									.name			= "",
									.help			= "Last Line number",
									.mask			= CLI_MASK(PRIV(15)),
									.tokenize	= NULL,
									.run			= cmd_linevty,
									.subtree	= NULL,
									.priv			= (int []) {VALID_LIMITS, 0, 15}
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
					.tokenize	= swcli_tokenize_number,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #mac-address-table aging-time <10-1000000> */
						& (struct menu_node){
							.name			= "<10-1000000>",
							.help			= "Maximum age in seconds",
							.mask			= CLI_MASK(PRIV(15)),
							.tokenize	= NULL,
							.run			= cmd_set_aging,
							.subtree	= NULL,
							.priv			= (int []) {VALID_LIMITS, 10, 1000000}
						},

						NULL
					} /*}}}*/
				},

				/* #mac-address-table static */
				&mac_addr_table_static,

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
							.run			= cmd_cdp_v2,
							.subtree	= NULL
						},

						/* #no cdp run */
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
									.tokenize	= swcli_tokenize_number,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no enable secret level <1-15> */
										& (struct menu_node){
											.name			= "<1-15>",
											.help			= "Level number",
											.mask			= CLI_MASK(PRIV(15)),
											.tokenize	= NULL,
											.run			= cmd_noensecret,
											.subtree	= NULL,
											.priv			= (int []) {VALID_LIMITS, 1, 15}
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
					.run			= cmd_hostname,
					.subtree	= NULL
				},

				/* #no interface */
				&config_interface,

				/* #no ip */
				& (struct menu_node){
					.name			= "ip",
					.help			= "Global IP configuration subcommands",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no ip igmp */
						& (struct menu_node){
							.name			= "igmp",
							.help			= "IGMP global configuration",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no ip igmp snooping */
								& (struct menu_node){
									.name			= "snooping",
									.help			= "Global IGMP Snooping enable for LiSA Vlans",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= NULL,
									.run			= cmd_ip_igmp_snooping,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no ip igmp snooping vlan*/
										& (struct menu_node){
											.name			= "vlan",
											.help			= "IGMP Snooping enable for LiSA VLAN",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= swcli_tokenize_number,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #no ip igmp snooping vlan <number>*/
												& (struct menu_node){
													.name			= "<1-4094>",
													.help			= "Vlan number",
													.mask			= CLI_MASK(PRIV(1)),
													.tokenize	= NULL,
													.run			= cmd_ip_igmp_snooping,
													.priv			= (int []) {VALID_LIMITS, 1, 4094}, 
													.subtree	= (struct menu_node *[]) { /*{{{*/
														/* #no ip igmp snooping vlan <number> mrouter*/
														& (struct menu_node){
															.name			= "mrouter",
															.help			= "Configure an L2 port as a multicast router port",
															.mask			= CLI_MASK(PRIV(1)),
															.tokenize	= NULL,
															.run			= NULL,
															.subtree	= (struct menu_node *[]) { /*{{{*/
																/* #no ip igmp snooping vlan <number> mrouter interface*/
																& (struct menu_node){
																	.name			= "interface",
																	.help			= "next-hop interface to mrouter",
																	.mask			= CLI_MASK(PRIV(1)),
																	.tokenize	= if_tok_if,
																	.run			= NULL,
																	.subtree	= (struct menu_node *[]) { /*{{{*/
																		/* #no ip igmp snooping vlan <number> mrouter interface Ethernet <number>*/
																		IF_ETHER(NULL, cmd_add_mrouter, NULL),
																		IF_NETDEV(NULL, cmd_add_mrouter, NULL),

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
							.run			= cmd_set_aging,
							.subtree	= NULL
						},

						/* #no mac-address-table static */
						&mac_addr_table_static,

						NULL
					} /*}}}*/
				},

				/* #no vlan */
				& config_vlan,

				NULL
			} /*}}}*/
		},

		/* #vlan */
		& config_vlan,

		NULL
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/
