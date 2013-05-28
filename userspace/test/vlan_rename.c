/**
 * Test vlan rename.
 *
 * Use cases:
 * 1. Try to rename invalid vlan.
 * 2. Try to rename default vlans and check it fails.
 * 3. Rename an existing vlan and test get_vlan_desc returns the desired name.
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "switch.h"
#include "lisa.h"

#define DEFAULT_VLAN		1
#define INVALID_VLAN		-1
#define ARBITRARY_VLAN		100
#define DESC1			"new description"
#define DESC2			"new desc"

static void test_rename_invalid(void) {
	int ret;

	ret = switch_set_vlan_desc(INVALID_VLAN, NULL);
	assert(ret == -1);
	assert(errno == EINVAL);
}

static void test_rename_default(void) {
	int ret;

	ret = switch_set_vlan_desc(DEFAULT_VLAN, NULL);
	assert(ret == -1);
	assert(errno == EPERM);
}

static void test_rename_arbitrary(void) {
	int add_ret, ret;
	char desc[SW_MAX_VLAN_NAME + 1], eq_desc[SW_MAX_VLAN_NAME + 1];
	char bkp_desc[SW_MAX_VLAN_NAME + 1];

	/* Initialize switch context as vlan description resides in mm. */
	switch_init();
	/* Try to add ARBITRARY_VLAN. If it already exists, just rename it. */
	add_ret = sw_ops->vlan_add(sw_ops, ARBITRARY_VLAN);
	if (add_ret == -1 && errno != EEXIST)
		assert(0);
	if (add_ret == 0) {
		__default_vlan_name(desc, ARBITRARY_VLAN);
		ret = switch_set_vlan_desc(ARBITRARY_VLAN, desc);
		if (ret) {
			ret = sw_ops->vlan_del(sw_ops, ARBITRARY_VLAN);
			assert(ret == 0);
			return;
		}
	}

	strcpy(desc, DESC1);

	/* If vlan_add returned EEXIST backup its description. */
	if (add_ret == -1) {
		ret = switch_get_vlan_desc(ARBITRARY_VLAN, bkp_desc);
		assert(ret == 0);
		if (strcmp(desc, bkp_desc) == 0)
			strcpy(desc, DESC2);
	}

	ret = switch_set_vlan_desc(ARBITRARY_VLAN, desc);
	assert(ret == 0);
	ret= switch_get_vlan_desc(ARBITRARY_VLAN, eq_desc);
	assert(ret == 0);
	assert(strcmp(desc, eq_desc) == 0);

	if (add_ret == 0) {
		/* Cleanup. */
		ret = sw_ops->vlan_del(sw_ops, ARBITRARY_VLAN);
		assert(ret == 0);
		ret = switch_del_vlan_desc(ARBITRARY_VLAN);
		assert(ret == 0);
	}
	else {
		/* Restore description. */
		ret = switch_set_vlan_desc(ARBITRARY_VLAN, bkp_desc);
		assert(ret == 0);
	}
}

int main(int argc, char **argv) {

	test_rename_invalid();
	printf("Rename Invalid test susccesful\n");
	test_rename_default();
	printf("Rename Default test susccesful\n");
	test_rename_arbitrary();
	printf("Rename vlan tests susccesful\n");

	return 0;
}
