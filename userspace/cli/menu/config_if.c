#include "swcli.h"
#include "config_if.h"

extern struct menu_node config_interface;

int swcli_tokenize_ip(struct cli_context *ctx, const char *buf,
		struct menu_node **tree, struct tokenize_out *out);

#define VLAN_WORD_MENU_NODE(__priv) {\
	.name			= "WORD",\
	.help			= "VLAN IDs of the allowed VLANs when this port is in trunking mode",\
	.mask			= CLI_MASK(PRIV(2)),\
	.tokenize	= NULL,\
	.run			= cmd_trunk_vlan,\
	.subtree	= NULL,\
	.priv			= (void *)(__priv)\
}

#define VLAN_WORD(__priv) & (struct menu_node) VLAN_WORD_MENU_NODE(__priv)

struct menu_node config_if_main = {
	/* Root node, .name is used as prompt */
	.name			= "config-if",
	.subtree	= (struct menu_node *[]) {
		/* #cdp */
		& (struct menu_node){
			.name			= "cdp",
			.help			= "CDP interface subcommands",
			.mask			= CLI_MASK(PRIV(2), IFF_SWITCHED),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #cdp enable */
				& (struct menu_node){
					.name			= "enable",
					.help			= "Enable CDP on interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_cdp_if_set,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #description */
		& (struct menu_node){
			.name			= "description",
			.help			= "Interface specific description",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #description LINE */
				& (struct menu_node){
					.name			= "LINE",
					.help			= "A character string describing this interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_if_desc,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #duplex */
		& (struct menu_node){
			.name			= "duplex",
			.help			= "Configure duplex operation.",
			.mask			= CLI_MASK(PRIV(2), IFF_SWITCHED),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #duplex auto */
				& (struct menu_node){
					.name			= "auto",
					.help			= "Enable AUTO duplex configuration",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #duplex full */
				& (struct menu_node){
					.name			= "full",
					.help			= "Force full duplex operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #duplex half */
				& (struct menu_node){
					.name			= "half",
					.help			= "Force half-duplex operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #end */
		& (struct menu_node){
			.name			= "end",
			.help			= "End from configure mode",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_end,
			.subtree	= NULL
		},

		/* #exit */
		& (struct menu_node){
			.name			= "exit",
			.help			= "Exit from interface configuration mode",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_exit,
			.subtree	= NULL
		},

		/* #help */
		& (struct menu_node){
			.name			= "help",
			.help			= "Descrption of the interactive help system",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_help,
			.subtree	= NULL
		},

		/* #interface */
		& config_interface,

		/* #ip */
		& (struct menu_node){
			.name			= "ip",
			.help			= "Interface Internet Protocol config commands",
			.mask			= CLI_MASK(PRIV(2), IFF_ROUTED | IFF_VIF),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #ip address */
				& (struct menu_node){
					.name			= "address",
					.help			= "Set the IP address of an interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= swcli_tokenize_ip,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #ip address A.B.C.D */
						& (struct menu_node){
							.name			= "A.B.C.D",
							.help			= "IP address",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= swcli_tokenize_ip,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #ip address A.B.C.D A.B.C.D */
								& (struct menu_node){
									.name			= "A.B.C.D",
									.help			= "IP subnet mask",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= NULL,
									.run			= cmd_ip,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #ip address A.B.C.D A.B.C.D secondary */
										& (struct menu_node){
											.name			= "secondary",
											.help			= "Make this IP address a secondary address",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_ip,
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

		/* #no */
		& (struct menu_node){
			.name			= "no",
			.help			= "Negate a command or set its defaults",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #no cdp */
				& (struct menu_node){
					.name			= "cdp",
					.help			= "CDP interface subcommands",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no cdp enable */
						& (struct menu_node){
							.name			= "enable",
							.help			= "Enable CDP on interface",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_cdp_if_set,
							.subtree	= NULL
						},

						NULL
					} /*}}}*/
				},

				/* #no description */
				& (struct menu_node){
					.name			= "description",
					.help			= "Interface specific description",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_if_desc,
					.subtree	= NULL
				},

				/* #no duplex */
				& (struct menu_node){
					.name			= "duplex",
					.help			= "Configure duplex operation.",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #no ip */
				& (struct menu_node){
					.name			= "ip",
					.help			= "Interface Internet Protocol config commands",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no ip address */
						& (struct menu_node){
							.name			= "address",
							.help			= "Set the IP address of an interface",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= swcli_tokenize_ip,
							.run			= cmd_ip,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no ip address A.B.C.D */
								& (struct menu_node){
									.name			= "A.B.C.D",
									.help			= "IP address",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= swcli_tokenize_ip,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no ip address A.B.C.D A.B.C.D */
										& (struct menu_node){
											.name			= "A.B.C.D",
											.help			= "IP subnet mask",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_ip,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #no ip address A.B.C.D A.B.C.D secondary */
												& (struct menu_node){
													.name			= "secondary",
													.help			= "Make this IP address a secondary address",
													.mask			= CLI_MASK(PRIV(2)),
													.tokenize	= NULL,
													.run			= cmd_ip,
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

				/* #no shutdown */
				& (struct menu_node){
					.name			= "shutdown",
					.help			= "Shutdown the selected interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_ioctl_simple,
					.subtree	= NULL,
					.priv			= (struct swcfgreq *[]) { /*{{{*/
						& (struct swcfgreq){
							.cmd				= SWCFG_ENABLE_IF
						},
						NULL
					} /*}}}*/
				},

				/* #no speed */
				& (struct menu_node){
					.name			= "speed",
					.help			= "Configure speed operation.",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #no switchport */
				& (struct menu_node){
					.name			= "switchport",
					.help			= "Set switching mode characteristics",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_swport_off,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #no switchport access */
						& (struct menu_node){
							.name			= "access",
							.help			= "Set access mode characteristics of the interface",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no switchport access vlan */
								& (struct menu_node){
									.name			= "vlan",
									.help			= "Set VLAN when interface is in access mode",
									.mask			= CLI_MASK(PRIV(1)),
									.tokenize	= NULL,
									.run			= cmd_acc_vlan,
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						/* #no switchport mode */
						& (struct menu_node){
							.name			= "mode",
							.help			= "Set trunking mode of the interface",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_nomode,
							.subtree	= NULL
						},

						/* #no switchport trunk */
						& (struct menu_node){
							.name			= "trunk",
							.help			= "Set trunking characteristics of the interface",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no switchport trunk allowed */
								& (struct menu_node){
									.name			= "allowed",
									.help			= "Set allowed VLAN characteristics when interface is in trunking mode",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #no switchport trunk allowed vlan */
										& (struct menu_node){
											.name			= "vlan",
											.help			= "Set allowed VLANs when interface is in trunking mode",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_trunk_vlan,
											.subtree	= NULL,
											.priv			= (void *)CMD_VLAN_NO
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

		/* #shutdown */
		& (struct menu_node){
			.name			= "shutdown",
			.help			= "Shutdown the selected interface",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_ioctl_simple,
			.subtree	= NULL,
			.priv			= (struct swcfgreq *[]) { /*{{{*/
				& (struct swcfgreq){
					.cmd				= SWCFG_DISABLE_IF
				},
				NULL
			} /*}}}*/
		},

		/* #speed */
		& (struct menu_node){
			.name			= "speed",
			.help			= "Configure speed operation.",
			.mask			= CLI_MASK(PRIV(2), IFF_SWITCHED),
			.tokenize	= NULL,
			.run			= NULL,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #speed 10 */
				& (struct menu_node){
					.name			= "10",
					.help			= "Force 10 Mbps operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #speed 100 */
				& (struct menu_node){
					.name			= "100",
					.help			= "Force 100 Mbps operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				/* #speed auto */
				& (struct menu_node){
					.name			= "auto",
					.help			= "Enable AUTO speed configuration",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_speed_duplex,
					.subtree	= NULL
				},

				NULL
			} /*}}}*/
		},

		/* #switchport */
		& (struct menu_node){
			.name			= "switchport",
			.help			= "Set switching mode characteristics",
			.mask			= CLI_MASK(PRIV(2), IFF_SWITCHED | IFF_ROUTED),
			.tokenize	= NULL,
			.run			= cmd_swport_on,
			.subtree	= (struct menu_node *[]) { /*{{{*/
				/* #switchport access */
				& (struct menu_node){
					.name			= "access",
					.help			= "Set access mode characteristics of the interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #switchport access vlan */
						& (struct menu_node){
							.name			= "vlan",
							.help			= "Set VLAN when interface is in access mode",
							.mask			= CLI_MASK(PRIV(1)),
							.tokenize	= swcli_tokenize_number,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #switchport access vlan <1-1094> */
								& (struct menu_node){
									.name			= "<1-1094>",
									.help			= "VLAN ID of the VLAN when this port is in access mode",
									.mask			= CLI_MASK(PRIV(1)),
									.tokenize	= NULL,
									.run			= cmd_acc_vlan,
									.priv			= (int []) {VALID_LIMITS, 1, 4094},
									.subtree	= NULL
								},

								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				/* #switchport mode */
				& (struct menu_node){
					.name			= "mode",
					.help			= "Set trunking mode of the interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #switchport mode access */
						& (struct menu_node){
							.name			= "access",
							.help			= "Set trunking mode to ACCESS unconditionally",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_ioctl_simple,
							.subtree	= NULL,
							.priv			= (struct swcfgreq *[]) { /*{{{*/
								& (struct swcfgreq){
									.cmd					= SWCFG_SETACCESS,
									.ext.access		= 1
								},
								NULL
							} /*}}}*/
						},

						/* #switchport mode trunk */
						& (struct menu_node){
							.name			= "trunk",
							.help			= "Set trunking mode to TRUNK unconditionally",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_ioctl_simple,
							.subtree	= NULL,
							.priv			= (struct swcfgreq *[]) { /*{{{*/
								& (struct swcfgreq){
									.cmd					= SWCFG_SETTRUNK,
									.ext.trunk		= 1
								},
								NULL
							} /*}}}*/
						},

						NULL
					} /*}}}*/
				},

				/* #switchport trunk */
				& (struct menu_node){
					.name			= "trunk",
					.help			= "Set trunking characteristics of the interface",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #switchport trunk allowed */
						& (struct menu_node){
							.name			= "allowed",
							.help			= "Set allowed VLAN characteristics when interface is in trunking mode",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #switchport trunk allowed vlan */
								& (struct menu_node){
									.name			= "vlan",
									.help			= "Set allowed VLANs when interface is in trunking mode",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= swcli_tokenize_word_mixed,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #switchport trunk allowed vlan WORD */
										VLAN_WORD(CMD_VLAN_SET),

										/* #switchport trunk allowed vlan add */
										& (struct menu_node){
											.name			= "add",
											.help			= "add VLANs to the current list",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= swcli_tokenize_word,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan add WORD */
												VLAN_WORD(CMD_VLAN_ADD),

												NULL
											} /*}}}*/
										},

										/* #switchport trunk allowed vlan all */
										& (struct menu_node){
											.name			= "all",
											.help			= "all VLANs",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_trunk_vlan,
											.subtree	= NULL,
											.priv			= (void *)CMD_VLAN_ALL
										},

										/* #switchport trunk allowed vlan except */
										& (struct menu_node){
											.name			= "except",
											.help			= "all VLANs except the following",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= swcli_tokenize_word,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan except WORD */
												VLAN_WORD(CMD_VLAN_EXCEPT),

												NULL
											} /*}}}*/
										},

										/* #switchport trunk allowed vlan none */
										& (struct menu_node){
											.name			= "none",
											.help			= "no VLANs",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_trunk_vlan,
											.subtree	= NULL,
											.priv			= (void *)CMD_VLAN_NONE
										},

										/* #switchport trunk allowed vlan remove */
										& (struct menu_node){
											.name			= "remove",
											.help			= "remove VLANs from the current list",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= swcli_tokenize_word,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan remove WORD */
												VLAN_WORD(CMD_VLAN_REMOVE),

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
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/
