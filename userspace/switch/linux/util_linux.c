/*
 *    This file is part of LiSA Switch
 *
 *    LiSA Switch is free software; you can redistribute it
 *    and/or modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2 of the
 *    License, or (at your option) any later version.
 *
 *    LiSA Switch is distributed in the hope that it will be
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Switch; if not, write to the Free
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 */

#include "util_linux.h"

/* Returns 1 if the switch has this VLAN, 0 otherwise */
int has_vlan(int vlan)
{
	mm_ptr_t ptr;

	mm_lock(mm);
	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->vlan_data)) {
		struct vlan_data *v_data =
			mm_addr(mm, mm_list_entry(ptr, struct vlan_data, lh));

		if (v_data->vlan_id == vlan) {
			mm_unlock(mm);
			return 1;
		}
	}
	mm_unlock(mm);

	return 0;
}

/* Returns 1 if the switch has this VLAN interface, 0 otherwise */
int has_vlan_if(int vlan)
{
	mm_ptr_t ptr;

	mm_lock(mm);

	mm_list_for_each(mm, ptr, mm_ptr(mm, &SHM->if_data)) {
		struct if_data *vif_data =
			mm_addr(mm, mm_list_entry(ptr, struct if_data, lh));

		if (vif_data->device.type == IF_TYPE_VIF &&
			vif_data->device.vlan == vlan) {
			mm_unlock(mm);
			return 1;
		}
	}

	mm_unlock(mm);

	return 0;
}

