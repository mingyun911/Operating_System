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

#ifndef __SCHED_H__
#define __SCHED_H__

/***********************************************************************
 * struct scheduler
 *
 * DESCRIPTION
 *   This structure is a collection of callback functions for a scheduler..
 *   Apply your scheduling policy by assigining appropriate functions to
 *   the function pointers.
 */
struct scheduler {
	const char *name;

	/***********************************************************************
	 * int initialize(void)
	 *
	 * DESCRIPTION
	 *   Callback function for your own initialization code. It is OK to
	 *   leave this field NULL if you don't need any initialization.
	 *
	 * RETURN VALUE
	 *   Return 0 on successful initialization.
	 *   Return other value on error, which leads the program to exit.
	 */
	int (*initialize)(void);


	/***********************************************************************
	 * void finalize(void)
	 *
	 * DESCRIPTION
	 *   Callback function for finalizing your code. Like @initialize(),
	 *   you may leave this function NULL.
	 */
	void (*finalize)(void);


	/***********************************************************************
	 * void forked(struct process *process)
	 *
	 * DESCRIPTION
	 *   Called when @process is newly forked. You may do per-process
	 *   initialization work in this function. You may leave this function
	 *   NULL if you don't need it.
	 */
	void (*forked)(struct process *);


	/***********************************************************************
	 * void exiting(struct process *process)
	 *
	 * DESCRIPTION
	 *   Called when @process is about to exit. You may do per-process
	 *   finalization work in this function. You may leave this function NULL
	 *   if you don't need it.
	 */
	void (*exiting)(struct process *);


	/***********************************************************************
	 * struct process *schedule(void)
	 *
	 * DESCRIPTION
	 *   Pick a process to run next. @current points to the current process
	 *   which has been running on the processor. You may put the current
	 *   into the ready queue and pick a process to run next if the current is
	 *   in the ready status. When the current is blocked (i.e., waiting for
	 *   some resources), however, you should not put it back into the ready
	 *   queue since it is not ready but is waiting for the resource!!
	 *
	 * RETURN
	 *   process to run next
	 *   NULL if there is no available process to schedule
	 */
	struct process *(*schedule)(void);


	/***********************************************************************
	 * bool acquire(int resource_id)
	 *
	 * DESCRIPTION
	 *   Callback function to acquire the resource @resource_id.
	 *
	 * RETURN
	 *   true on successful acquision
	 *   false if the resource is already held by others or unavailable
	 */
	bool (*acquire)(int);


	/***********************************************************************
	 * void release(int resource_id)
	 *
	 * DESCRIPTION
	 *   Callbacked to release the resource @resource_id
	 */
	void (*release)(int);
};

#endif
