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
#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

/* The number of physical page frames of the system */
#define NR_PAGEFRAMES	128

/* The number of PTEs in a page */
#define PTES_PER_PAGE_SHIFT	4
#define NR_PTES_PER_PAGE    (1 << PTES_PER_PAGE_SHIFT)

#define PDES_PER_PAGE_SHIFT 2
#define NR_PDES_PER_PAGE	(1 << PDES_PER_PAGE_SHIFT)

/* Protection bits for read and write */
#define ACCESS_NONE  0x00
#define ACCESS_READ  0x01
#define ACCESS_WRITE 0x02

/**
 * 2-level page table abstraction
 */
struct pte {
	bool valid;
	unsigned int rw;
	unsigned int pfn;
	unsigned int private;	/* May use to backup something ;-) */
};

struct pte_directory {
	struct pte ptes[NR_PTES_PER_PAGE];
};

struct pagetable {
	struct pte_directory *pdes[NR_PDES_PER_PAGE];
};


/**
 * Simplified PCB
 */
struct process {
	unsigned int pid;

	struct pagetable pagetable;

	struct list_head list;  /* List head to chain processes on the system */
};

struct tlb_entry {
	bool valid;
	int rw;
	unsigned int vpn;
	unsigned int pfn;
	unsigned int private;
};

#define NR_TLB_ENTRIES	(1 << (PTES_PER_PAGE_SHIFT * 2))
#endif
