/*
  *    this file is part of lisa multi-engine implementation
  *
  *    lisa command line interface is free software; you can redistribute it 
  *    and/or modify it under the terms of the gnu general public license 
  *    as published by the free software foundation; either version 2 of the 
  *    license, or (at your option) any later version.
  *
  *    lisa command line interface is distributed in the hope that it will be 
  *    useful, but without any warranty; without even the implied warranty of
  *    merchantability or fitness for a particular purpose.  see the
  *    gnu general public license for more details.
  *
  *    you should have received a copy of the gnu general public license
  *    along with lisa command line interface; if not, write to the free 
  *    software foundation, inc., 59 temple place, suite 330, boston, 
  *    ma  02111-1307  usa
  */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "multiengine.h"

LIST_HEAD(head_sw_ops);

int register_switch(struct switch_operations *ops)
{
	struct sw_ops_entries *entry;

	entry = malloc(sizeof(struct sw_ops_entries));
	if (NULL == entry) {
		printf("Registering switch \n");
		return -1;
	}

	entry->sw_ops =  ops;

	list_add_tail(&(entry->lh), &head_sw_ops);
		
	return 0;
}

int multiengine_init(void)
{
	void *handle, *handle_current;

	INIT_LIST_HEAD(&head_sw_ops);

	handle_current = dlopen(NULL,RTLD_NOW|RTLD_GLOBAL);
	if (NULL == handle_current) {
		return -1;
	}

	/* open switch libraries */
	handle = dlopen("libswitch.so", RTLD_LAZY);
	if (NULL == handle) {
		printf("Error opening library %s\n", dlerror());
		return -1;
	}

	return 0;
}


int main()
{
	multiengine_init();
	return 0;
}

