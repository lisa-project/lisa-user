/*
  *    This file is part of LiSA Multi-engine implementation
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
#ifndef _MULTIENGINE_H
#define _MULTIENGINE_H

#include "cJSON.h"
#include "backend_api.h"

#define MAX_NAME_SIZE		128
#define CONFIG_FILENAME 	"backend_implementations.json"
#define JSON_SWITCH_NODE	"backend_objects"
#define SW_LOCAL		"local"


struct switch_interface {
	char if_name[MAX_NAME_SIZE];
	struct list_head lh;
};

struct sw_ops_entries {
	int sw_index;
	char port[MAX_NAME_SIZE];
	char ip[MAX_NAME_SIZE];
	char type[MAX_NAME_SIZE];
	char locality[MAX_NAME_SIZE];
	struct switch_operations *sw_ops;
	struct list_head if_names_lh;
	struct list_head lh;
};

struct list_head head_sw_ops;

void print_lists(struct list_head head_sw_ops);
void multiengine_init(void)__attribute__((constructor));
#endif
