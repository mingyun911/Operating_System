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

#ifndef __PROCESS_H__
#define __PROCESS_H__

struct list_head;

enum process_status {
	PROCESS_READY,		/* Process is ready to run */
	PROCESS_RUNNING,	/* The process is now running */
	PROCESS_BLOCKED,		/* The process is waiting for some resource */
	PROCESS_EXIT,		/* The process is exited */
};

struct process {
	unsigned int pid;		/* Process ID */

	enum process_status status;
							/* The status of the process */

	unsigned int age;		/* # of ticks the process was scheduled in */
	unsigned int lifespan;	/* The lifespan of the process. The process will
							   be exited when age == lifespan */

	unsigned int prio;		/* Currently effective priority of the process.
							   0 by default, and the larger, the more important
							   process it is */

	struct list_head list;	/* list head for listing processes */

	unsigned int prio_orig;	/* The original priority of the process. You might
							   need it to implement dynamic priority features
							   such as aging, PIP and PCP. */


	/** DO NOT ACCESS FOLLOWING VARIABLES. THESE ARE USED FOR SIMULATOR IMPLEMENTATION **/
	unsigned int __starts_at;	/* When to fork the process */

	struct list_head __resources_to_acquire;
								/* Schedule to acquire resources */

	struct list_head __resources_holding;
								/* Resources that the process is currently holding */
};

/**
 * Support function to dump the process and resource status
 */
void dump_status(void);

#define MAX_PRIO	64	/* Maximum value for priority */

#endif
