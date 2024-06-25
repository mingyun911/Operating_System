/**********************************************************************
 * Copyright (c) 2019-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "list_head.h"

struct process;

/**
 * Resources in the system.
 */
struct resource {
	/**
	 * The owner process of this resource. NULL implies the resource is free
	 * whereas non-NULL implies @owner process owns this resource
	 */
	struct process *owner;

	/**
	 * list head to list processes that are wanting for the resource
	 */
	struct list_head waitqueue;
};

/**
 * This system has 16 different resources. It is defined in sched.c as an array of
 * struct resource (i.e., struct resource resources[NR_RESOURCES];)
 */
#define NR_RESOURCES 16

#endif
