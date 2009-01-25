#include "cli.h"
#include "swcli_common.h"
#include "menu_interface.h"

int cmd_cdp_if_enable(struct cli_context *, int, char **, struct menu_node **);
int cmd_setethdesc(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_auto(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_full(struct cli_context *, int, char **, struct menu_node **);
int cmd_du_half(struct cli_context *, int, char **, struct menu_node **);
int cmd_end(struct cli_context *, int, char **, struct menu_node **);
int cmd_exit(struct cli_context *, int, char **, struct menu_node **);
int cmd_help(struct cli_context *, int, char **, struct menu_node **);
int cmd_cdp_if_disable(struct cli_context *, int, char **, struct menu_node **);
int cmd_noethdesc(struct cli_context *, int, char **, struct menu_node **);
int cmd_noshutd(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_auto(struct cli_context *, int, char **, struct menu_node **);
int cmd_swport_off(struct cli_context *, int, char **, struct menu_node **);
int cmd_noacc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_nomode(struct cli_context *, int, char **, struct menu_node **);
int cmd_allvlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_shutd(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_10(struct cli_context *, int, char **, struct menu_node **);
int cmd_sp_100(struct cli_context *, int, char **, struct menu_node **);
int cmd_swport_on(struct cli_context *, int, char **, struct menu_node **);
int cmd_acc_vlan(struct cli_context *, int, char **, struct menu_node **);
int cmd_access(struct cli_context *, int, char **, struct menu_node **);
int cmd_trunk(struct cli_context *, int, char **, struct menu_node **);
int cmd_setvlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_addvlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_excvlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_novlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_remvlans(struct cli_context *, int, char **, struct menu_node **);
int cmd_ip(struct cli_context *, int, char **, struct menu_node **);
int cmd_no_ip(struct cli_context *, int, char **, struct menu_node **);

extern struct menu_node config_interface;

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
					.run			= cmd_cdp_if_enable,
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
					.run			= cmd_setethdesc,
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
					.run			= cmd_du_auto,
					.subtree	= NULL
				},

				/* #duplex full */
				& (struct menu_node){
					.name			= "full",
					.help			= "Force full duplex operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_du_full,
					.subtree	= NULL
				},

				/* #duplex half */
				& (struct menu_node){
					.name			= "half",
					.help			= "Force half-duplex operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_du_half,
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
					.tokenize	= NULL,
					.run			= NULL,
					.subtree	= (struct menu_node *[]) { /*{{{*/
						/* #ip address A.B.C.D */
						& (struct menu_node){
							.name			= "A.B.C.D",
							.help			= "IP address",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
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
							.run			= cmd_cdp_if_disable,
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
					.run			= cmd_noethdesc,
					.subtree	= NULL
				},

				/* #no duplex */
				& (struct menu_node){
					.name			= "duplex",
					.help			= "Configure duplex operation.",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_du_auto,
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
							.tokenize	= NULL,
							.run			= cmd_no_ip,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #no ip address A.B.C.D */
								& (struct menu_node){
									.name			= "A.B.C.D",
									.help			= "IP address",
									.mask			= CLI_MASK(PRIV(2)),
									.tokenize	= NULL,
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
					.run			= cmd_noshutd,
					.subtree	= NULL
				},

				/* #no speed */
				& (struct menu_node){
					.name			= "speed",
					.help			= "Configure speed operation.",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_sp_auto,
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
									.run			= cmd_noacc_vlan,
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
											.run			= cmd_allvlans,
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

		/* #shutdown */
		& (struct menu_node){
			.name			= "shutdown",
			.help			= "Shutdown the selected interface",
			.mask			= CLI_MASK(PRIV(2)),
			.tokenize	= NULL,
			.run			= cmd_shutd,
			.subtree	= NULL
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
					.run			= cmd_sp_10,
					.subtree	= NULL
				},

				/* #speed 100 */
				& (struct menu_node){
					.name			= "100",
					.help			= "Force 100 Mbps operation",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_sp_100,
					.subtree	= NULL
				},

				/* #speed auto */
				& (struct menu_node){
					.name			= "auto",
					.help			= "Enable AUTO speed configuration",
					.mask			= CLI_MASK(PRIV(2)),
					.tokenize	= NULL,
					.run			= cmd_sp_auto,
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
							.tokenize	= NULL,
							.run			= NULL,
							.subtree	= (struct menu_node *[]) { /*{{{*/
								/* #switchport access vlan <1-1094> */
								& (struct menu_node){
									.name			= "<1-1094>",
									.help			= "VLAN ID of the VLAN when this port is in access mode",
									.mask			= CLI_MASK(PRIV(1)),
									.tokenize	= NULL,
									.run			= cmd_acc_vlan,
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
							.run			= cmd_access,
							.subtree	= NULL
						},

						/* #switchport mode trunk */
						& (struct menu_node){
							.name			= "trunk",
							.help			= "Set trunking mode to TRUNK unconditionally",
							.mask			= CLI_MASK(PRIV(2)),
							.tokenize	= NULL,
							.run			= cmd_trunk,
							.subtree	= NULL
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
									.tokenize	= NULL,
									.run			= NULL,
									.subtree	= (struct menu_node *[]) { /*{{{*/
										/* #switchport trunk allowed vlan WORD */
										& (struct menu_node){
											.name			= "WORD",
											.help			= "VLAN IDs of the allowed VLANs when this port is in trunking mode",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_setvlans,
											.subtree	= NULL
										},

										/* #switchport trunk allowed vlan add */
										& (struct menu_node){
											.name			= "add",
											.help			= "add VLANs to the current list",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan add WORD */
												& (struct menu_node){
													.name			= "WORD",
													.help			= "VLAN IDs of the allowed VLANs when this port is in trunking mode",
													.mask			= CLI_MASK(PRIV(2)),
													.tokenize	= NULL,
													.run			= cmd_addvlans,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #switchport trunk allowed vlan all */
										& (struct menu_node){
											.name			= "all",
											.help			= "all VLANs",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_allvlans,
											.subtree	= NULL
										},

										/* #switchport trunk allowed vlan except */
										& (struct menu_node){
											.name			= "except",
											.help			= "all VLANs except the following",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan except WORD */
												& (struct menu_node){
													.name			= "WORD",
													.help			= "VLAN IDs of the allowed VLANs when this port is in trunking mode",
													.mask			= CLI_MASK(PRIV(2)),
													.tokenize	= NULL,
													.run			= cmd_excvlans,
													.subtree	= NULL
												},

												NULL
											} /*}}}*/
										},

										/* #switchport trunk allowed vlan none */
										& (struct menu_node){
											.name			= "none",
											.help			= "no VLANs",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= cmd_novlans,
											.subtree	= NULL
										},

										/* #switchport trunk allowed vlan remove */
										& (struct menu_node){
											.name			= "remove",
											.help			= "remove VLANs from the current list",
											.mask			= CLI_MASK(PRIV(2)),
											.tokenize	= NULL,
											.run			= NULL,
											.subtree	= (struct menu_node *[]) { /*{{{*/
												/* #switchport trunk allowed vlan remove WORD */
												& (struct menu_node){
													.name			= "WORD",
													.help			= "VLAN IDs of the allowed VLANs when this port is in trunking mode",
													.mask			= CLI_MASK(PRIV(2)),
													.tokenize	= NULL,
													.run			= cmd_remvlans,
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
	}
};

/* vim: ts=2 shiftwidth=2 foldmethod=marker
*/
