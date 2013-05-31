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

#ifndef _SHARED_H
#define _SHARED_H


#include "cdp_client.h"
#include "rstp_client.h"
#include "sw_api.h"
#include "mm.h"
#include "swsock.h"

#define SW_CONFIG_ROOT	"/etc/lisa"
#define SW_CONFIG_FILE	SW_CONFIG_ROOT"/config.text"
#define SW_TAGS_FILE	SW_CONFIG_ROOT"/tags"

#define SW_MAX_VTY		15
#define SW_MAX_ENABLE	15

#define SW_PASS_LEN 	30
#define SW_SECRET_LEN 	30

#define SW_MAX_TAG		40
#define SW_MAX_VLAN_NAME	31
#define SW_MAX_PORT_DESC	31

extern struct switch_operations *sw_ops;
extern struct mm_private *mm;
#define SHM ((struct switch_mem *)MM_STATIC(mm))

/*
 * Switch shared memory structure
 */
struct switch_mem {
	/* Enable secrets (crypted) */
	struct {
		char secret[SW_SECRET_LEN + 1];
	} enable[SW_MAX_ENABLE+1];
	/* Line vty passwords (clear text) */
	struct {
		char passwd[SW_PASS_LEN + 1];
	} vty[SW_MAX_VTY + 1];
	/* CDP configuration */
	struct cdp_configuration cdp;
	/* RSTP configuration */
	struct rstp_configuration rstp;
	/* List of interface tags */
	struct mm_list_head if_tags;
	/* List of vlan descriptions */
	struct mm_list_head vlan_descs;
	/* List of interface descriptions */
	struct mm_list_head if_descs;

	/* List of VLAN specific data */
	struct mm_list_head vlan_data;
	/* List of interface specific data */
	struct mm_list_head if_data;
	/* Bridge + 8021q age time */
	int age_time;
};

struct if_tag {
	int if_index;
	char tag[SW_MAX_TAG + 1];
	struct mm_list_head lh;
};

struct vlan_desc {
	int vlan_id;
	char desc[SW_MAX_VLAN_NAME + 1];
	struct mm_list_head lh;
};

struct if_desc {
	int if_index;
	char desc[SW_MAX_PORT_DESC + 1];
	struct mm_list_head lh;
};


/* Used only by the bridge + 8021q middleware by now */
struct vlan_data {
	int vlan_id;
	struct mm_list_head vif_list;
	struct mm_list_head lh;
};

struct if_data {
	struct net_switch_device device;
	int mode;
	unsigned char *bitmap;
	unsigned short access_vlan;
	struct mm_list_head lh;
};

/* Temporarily copied from include/linux/net_switch.h */
#define sw_bitmap_reset(bitmap, offset) ((bitmap)[(offset) / 8] &= ~(1 << ((offset) % 8)))
#define sw_bitmap_set(bitmap, offset)	((bitmap)[(offset) / 8] |= (1 << ((offset) % 8)))
#define sw_bitmap_test(bitmap, offset)	((bitmap)[(offset) / 8] & (1 << ((offset) % 8)))

#define sw_allow_vlan(bitmap, vlan)	(sw_bitmap_reset(bitmap, vlan))
#define sw_forbid_vlan(bitmap, vlan)	(sw_bitmap_set(bitmap, vlan))
#define sw_forbidden_vlan(bitmap, vlan) (sw_bitmap_test(bitmap, vlan))
#define sw_allowed_vlan(bitmap, vlan)	(!sw_bitmap_test(bitmap, vlan))

#define sw_set_mrouter(bitmap, vlan)	(sw_bitmap_set(bitmap, vlan))
#define sw_reset_mrouter(bitmap, vlan)	(sw_bitmap_reset(bitmap, vlan))
#define sw_is_mrouter(bitmap, vlan)	(sw_bitmap_test(bitmap, vlan))


#define __default_vlan_name(__buf, __vlan) snprintf(__buf, 9, "VLAN%04d", (__vlan))
#define default_vlan_name(__lvalue, __vlan) do {\
		int status; \
		__lvalue = alloca(9); \
		status = __default_vlan_name(__lvalue, __vlan); \
		assert(status < 9); \
} while (0)

#define __default_iface_name(__buf) snprintf(__buf, 2,"--")
#define default_iface_name(__lvalue) do {\
		int status; \
		__lvalue = alloca(2); \
		status = __default_iface_name(__lvalue); \
		assert(status < 2); \
} while (0)

/* Identifiers for the types of passwords stored in the
 * shared memory area
 */
enum {
	SHARED_AUTH_VTY,
	SHARED_AUTH_ENABLE,
};


/* Initialize the switch's shared memory area */
int switch_init(void) __attribute__((constructor));

/* The caller invokes this function requesting the type of
 * authentication and level. In turn, the function will call
 * the user's auth() callback function with the appropriate
 * password from the shared memory area, passing it back the
 * private data pointer.
 */
int switch_auth(int type, int level,
		int (*auth)(char *pw, void *priv), void *priv);

/* Stores the requested password in the shared memory area */
int switch_set_passwd(int type, int level, char *passwd);

/* lookup interface arg0 and put tag into arg1; return 0 if
 * interface has a tag, 1 otherwise
 */
int switch_get_if_tag(int if_index, char *tag);

/* 1. if arg1 is not null, then assign tag arg1 to interface
 * arg0; if arg2 is not NULL and tag is already assigned to
 * another interface, put the other interface's name into arg2.
 * return 0 if operation successful, 1 if tag is already assigned
 * to another interface
 * 2. if arg1 is null, delete tag for interface arg0. return 0 if
 * successfull, 1 if interface had no tag assigned.
 */
int switch_set_if_tag(int if_index, char *tag, int *other_if);



/**
 * lookup interface arg0 and put description into arg1
 * return 0 if operation was succesful or a negative value if
 * the interface has no description
 */
int switch_get_if_desc(int if_index, char *desc);

/**
 * if interface description arg1 is null then the default
 * value will be set, else arg1 will be set as description for
 * interface arg0
 * return 0 if succesfull or a negative if setting the description
 * failed
 */
int switch_set_if_desc(int if_index, char *desc);

/* Forgets about interface identified by arg0; return 0 if  interface has been
 * stored in shared memory, negative value otherwise and set errno.
 */
int switch_del_if(int if_index);



/* lookup vlan arg0 and put description into arg1; return 0 if
 * vlan has a description, negative value otherwise and set errno
 */
int switch_get_vlan_desc(int vlan_id, char *desc);

/* If arg1 is null, reset description for vlan arg0 to default,
 * otherwise set arg1 as description for vlan arg0. To delete
 * description call switch_del_vlan instead.
 *
 * return 0 if successful, negative value if setting description failed
 * and also set errno
 */
int switch_set_vlan_desc(int vlan_id, const char *desc);

/* Forgets about vlan identified by arg0; return 0 if vlan has been
 * stored in shared memory, negative value otherwise and set errno.
 */
int switch_del_vlan_desc(int vlan_id);



/* Lookup vlan data arg0 and put it into arg1; return 0 if
 * vlan has data assigned, negative value otherwise and set errno
 */
int get_vlan_data(int vlan_id, struct vlan_data **data);

/* Buld new vlan data if there is no data assigned to vlan_id
 * or replace the net_devices list with the new one
 */
int add_vlan_data(int vlan_id);

/* Remove VLAN data for a given vlan_id
 */
int del_vlan_data(int vlan_id);



/* Lookup interface data for arg0 and put it into arg1; return 0 if
 * interface has data assigned, negative value otherwise and set errno
 */
int get_if_data(int if_index, struct if_data *data);

/* Buld new interface data if there is no data assigned to if_index
 * or replace the structure with a new one
 */
int set_if_data(int if_index, struct if_data new_data);

/* Remove interface data for a given if_index
 */
int del_if_data(int if_index);



/* Add virtual interface specific information to VLAN data */
int add_vif_data(int vlan_id, struct net_switch_device device);

/* Remove virtual interface information from VLAN data */
void del_vif_data(int vlan_id, char *if_name);



/* Sets the cdp global configuration */
void switch_set_cdp(struct cdp_configuration *cdp);

/* Gets the cdp global configuration */
void switch_get_cdp(struct cdp_configuration *cdp);

/* Sets the RSTP global configuration */
void switch_set_rstp(struct rstp_configuration *rstp);

/* Gets the RSTP global configuration */
void switch_get_rstp(struct rstp_configuration *rstp);

#endif
