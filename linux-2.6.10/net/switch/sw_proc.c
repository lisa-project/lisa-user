#include <linux/proc_fs.h>

#include "sw_debug.h"
#include "sw_proc.h"

static struct proc_dir_entry *switch_dir, *iface_file,
							 *mac_file;


static int proc_read_ifaces(char *page, char **start,
		off_t off, int count,
		int *eof, void *data) {
	int len;

	len = sprintf(page, "Hello, this is the ifaces file\n");
		
	return len;
}

static int proc_read_mac(char *page, char **start,
		off_t off, int count,
		int *eof, void *data) {
	int len;

	len = sprintf(page, "Hello, this is the mac file\n");
		
	return len;
}

int init_switch_proc(void) {

	/* Create our own directory under /proc/net */
	switch_dir = proc_mkdir(SW_PROCFS_DIR, proc_net);
	if (!switch_dir)
		return -ENOMEM;

	switch_dir->owner = THIS_MODULE;

	/*
	   Create the ifaces file, which lists information
	   about the interfaces added to switch
	 */
	iface_file = create_proc_read_entry(SW_PROCFS_IFACES, 0600,
			switch_dir, proc_read_ifaces, NULL);
	if (!iface_file) {
		remove_proc_entry(SW_PROCFS_DIR, proc_net);
		return -ENOMEM;
	}
	iface_file->owner = THIS_MODULE;

	mac_file = create_proc_read_entry(SW_PROCFS_MAC, 0600,
			switch_dir, proc_read_mac, NULL);
	if (!mac_file) {
		remove_proc_entry(SW_PROCFS_DIR, proc_net);
		remove_proc_entry(SW_PROCFS_IFACES, switch_dir);
		return -ENOMEM;
	}
	mac_file->owner = THIS_MODULE;

	dbg("sw_proc initialized\n");
	return 0;
}

void cleanup_switch_proc(void) {
	remove_proc_entry(SW_PROCFS_IFACES, switch_dir);
	remove_proc_entry(SW_PROCFS_MAC, switch_dir);
	remove_proc_entry(SW_PROCFS_DIR, proc_net);
}
