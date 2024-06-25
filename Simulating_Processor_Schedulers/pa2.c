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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;

/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];

/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;

/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;

/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_BLOCKED;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_BLOCKED);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FCFS scheduler
 ***********************************************************************/
static int fcfs_initialize(void)
{
	return 0;
}

static void fcfs_finalize(void)
{
}

static struct process *fcfs_schedule(void)
{
	struct process *next = NULL;
	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, the current process can be blocked
	 * while acquiring a resource. In this case just pick the next as well.
	 */
	if (!current || current->status == PROCESS_BLOCKED) {
		goto pick_next;
	}
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	if (!list_empty(&readyqueue)) {
		next = list_first_entry(&readyqueue, struct process, list);
		/**
		 * Detach the process from the ready queue. Note that we use 
		 * list_del_init() over list_del() to maintain the list head tidy.
		 * Otherwise, the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}
	return next;
}

struct scheduler fcfs_scheduler = {
	.name = "FCFS",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fcfs_initialize,
	.finalize = fcfs_finalize,
	.schedule = fcfs_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/

static struct process *sjf_schedule(void)
{
    struct process* next = NULL;
    struct process* test;
    if(current == NULL){
        goto select;
    }
    if (current->age < current->lifespan) {
        return current;
    }
    select:
    if(!list_empty(&readyqueue)){
        next = list_first_entry(&readyqueue, struct process, list);
        unsigned int shortest = next->lifespan;
        list_for_each_entry(test, &readyqueue, list){
            if(test->lifespan < shortest){
                next = test;
                shortest = next->lifespan;
            }
        }
        list_del_init(&next->list);
    }
    return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.schedule = sjf_schedule,
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/
static struct process *stcf_schedule(void){
    struct process* next = NULL;
    struct process* test;
    if(current == NULL){ /// current 가 없음
        goto select_non_current;
    }
    else{ /// current 가 있음
        if(!list_empty(&readyqueue)){
            if(current->lifespan - current->age > 0){
                list_add_tail(&current->list, &readyqueue);
            }
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int shortest = next->lifespan - next->age;
            list_for_each_entry(test, &readyqueue, list){
                if((test->lifespan - test->age) < shortest){
                    next = test;
                    shortest = next->lifespan - next->age;
                }
            }
            list_del_init(&next->list);
            return next;
        }
        else{
            if (current->age < current->lifespan) {
                return current;
            }
            else return next;
        }
    }
    select_non_current:
    if(!list_empty(&readyqueue)){
        next = list_first_entry(&readyqueue, struct process, list);
        unsigned int shortest = next->lifespan;
        list_for_each_entry(test, &readyqueue, list){
            if(test->lifespan < shortest){
                next = test;
                shortest = next->lifespan;
            }
        }
        list_del_init(&next->list);
    }
    return next;
}
struct scheduler stcf_scheduler = {
	.name = "Shortest Time-to-Complete First",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
    .schedule = stcf_schedule,
};

/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/

static struct process *rr_schedule(void){
    struct process* next = NULL;
    if(current == NULL){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            list_del_init(&next->list);
        }
        return next;
    }
    else{
        if(current->age < current->lifespan){
            list_add_tail(&current->list, &readyqueue);
        }
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            list_del_init(&next->list);
            return next;
        }
        else{
            return next;
        }
    }
}
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
    .schedule = rr_schedule,
};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static bool prio_acquire(int resource_id){
    struct resource *r = resources + resource_id;
    if (!r->owner) {
        r->owner = current;
        return true;
    }
    current->status = PROCESS_BLOCKED;
    list_add_tail(&current->list, &r->waitqueue);
    return false;
}
static void prio_release(int resource_id){
    struct resource *r = resources + resource_id;
    assert(r->owner == current);
    r->owner = NULL;
    if (!list_empty(&r->waitqueue)) {
        struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
        unsigned int highest = waiter->prio;
        struct process* test;
        list_for_each_entry(test, &r->waitqueue, list){
            if(test->prio > highest){
                waiter = test;
                highest = waiter->prio;
            }
        }
        assert(waiter->status == PROCESS_BLOCKED);
        list_del_init(&waiter->list);
        waiter->status = PROCESS_READY;
        list_add_tail(&waiter->list, &readyqueue);
    }
}

static struct process *prio_schedule(void){
    struct process* next = NULL;
    struct process* test;
    if(current == NULL || current->status == PROCESS_BLOCKED){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
        }
        return next;
    }
    else{
        int check = 0;
        unsigned int prior = current->prio;
        list_for_each_entry(test, &readyqueue, list){
            if(test->prio == prior){
                check = 1;
                break;
            }
        }

        if(check == 0){ /// 같은 priority 을 가진 process 는 없음
            if(current->age < current->lifespan){
                list_add_tail(&current->list, &readyqueue);
            }
            if(!list_empty(&readyqueue)){
                next = list_first_entry(&readyqueue, struct process, list);
                unsigned int highest = next->prio;
                list_for_each_entry(test, &readyqueue, list){
                    if(test->prio > highest){
                        next = test;
                        highest = next->prio;
                    }
                }
                list_del_init(&next->list);
                return next;
            }
            return next;
        }
        else{ /// 같은 priority 을 가진 process 가 있음
            if(current->age < current->lifespan){
                list_add_tail(&current->list, &readyqueue);
            }
            if(!list_empty(&readyqueue)){
                unsigned int highest = current->prio;
                list_for_each_entry(test, &readyqueue, list){
                    if(test->prio == highest){
                        next = test;
                        break;
                    }
                }
                list_del_init(&next->list);
                return next;
            }
            else{
                return next;
            }
        }
    }
}

struct scheduler prio_scheduler = {
	.name = "Priority",
    .acquire = prio_acquire,
    .release = prio_release,
    .schedule = prio_schedule,
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/

static struct process *pa_schedule(void){
    struct process* next = NULL;
    struct process* test;
    if(current == NULL || current->status == PROCESS_BLOCKED){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
        }
        return next;
    }
    current->prio = current->prio_orig;
    if(!list_empty(&readyqueue)){
        list_for_each_entry(test, &readyqueue, list){
            test->prio++;
        }
    }
    if(current->age < current->lifespan){
        list_add_tail(&current->list, &readyqueue);
    }
    if(!list_empty(&readyqueue)){
        next = list_first_entry(&readyqueue, struct process, list);
        unsigned int highest = next->prio;
        list_for_each_entry(test, &readyqueue, list){
            if(test->prio > highest){
                next = test;
                highest = next->prio;
            }
        }
        list_del_init(&next->list);
        return next;
    }
    return next;
}

struct scheduler pa_scheduler = {
	.name = "Priority + aging",
    .acquire = prio_acquire,
    .release = prio_release,
    .schedule = pa_schedule,
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/

static bool pcp_acquire(int resource_id){
    struct resource *r = resources + resource_id;
    if (!r->owner) {
        r->owner = current;
        r->owner->prio = MAX_PRIO;
        return true;
    }
    current->status = PROCESS_BLOCKED;
    list_add_tail(&current->list, &r->waitqueue);
    return false;
}
static void pcp_release(int resource_id){
    struct resource *r = resources + resource_id;
    assert(r->owner == current);
    r->owner->prio = r->owner->prio_orig;
    r->owner = NULL;
    if (!list_empty(&r->waitqueue)) {
        struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
        unsigned int highest = waiter->prio;
        struct process* test;
        list_for_each_entry(test, &r->waitqueue, list){
            if(test->prio > highest){
                waiter = test;
                highest = waiter->prio;
            }
        }
        assert(waiter->status == PROCESS_BLOCKED);
        list_del_init(&waiter->list);
        waiter->status = PROCESS_READY;
        list_add_tail(&waiter->list, &readyqueue);
    }
}
static struct process *pcp_schedule(void){
    struct process* next = NULL;
    struct process* test;
    if(current == NULL){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
        }
        return next;
    }
    if(current->status == PROCESS_BLOCKED){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
            next->prio = MAX_PRIO;
        }
        return next;
    }
    if(current->age < current->lifespan){
        list_add_tail(&current->list, &readyqueue);
    }
    if(!list_empty(&readyqueue)){
        next = list_first_entry(&readyqueue, struct process, list);
        unsigned int highest = next->prio;
        list_for_each_entry(test, &readyqueue, list){
            if(test->prio > highest){
                next = test;
                highest = next->prio;
            }
        }
        list_del_init(&next->list);
        return next;
    }
    return next;
}
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
    .release = pcp_release,
    .schedule = pcp_schedule,
};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
static bool pip_acquire(int resource_id){
    struct resource *r = resources + resource_id;
    if (!r->owner) {
        r->owner = current;
        return true;
    }
    current->status = PROCESS_BLOCKED;
    if(r->owner->prio < current->prio)
        r->owner->prio = current->prio;
    list_add_tail(&current->list, &r->waitqueue);
    return false;
}
static void pip_release(int resource_id){
    struct resource *r = resources + resource_id;
    assert(r->owner == current);
    r->owner->prio = r->owner->prio_orig;
    r->owner = NULL;
    if (!list_empty(&r->waitqueue)) {
        struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
        unsigned int highest = waiter->prio;
        struct process* test;
        list_for_each_entry(test, &r->waitqueue, list){
            if(test->prio > highest){
                waiter = test;
                highest = waiter->prio;
            }
        }
        assert(waiter->status == PROCESS_BLOCKED);
        list_del_init(&waiter->list);
        waiter->status = PROCESS_READY;
        list_add_tail(&waiter->list, &readyqueue);
    }
}
static struct process *pip_schedule(void){
    struct process* next = NULL;
    struct process* test;
    if(current == NULL){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
        }
        return next;
    }
    if(current->status == PROCESS_BLOCKED){
        if(!list_empty(&readyqueue)){
            next = list_first_entry(&readyqueue, struct process, list);
            unsigned int highest = next->prio;
            list_for_each_entry(test, &readyqueue, list){
                if(test->prio > highest){
                    next = test;
                    highest = next->prio;
                }
            }
            list_del_init(&next->list);
        }
        return next;
    }
    if(current->age < current->lifespan){
        list_add_tail(&current->list, &readyqueue);
    }
    if(!list_empty(&readyqueue)){
        next = list_first_entry(&readyqueue, struct process, list);
        unsigned int highest = next->prio;
        list_for_each_entry(test, &readyqueue, list){
            if(test->prio > highest){
                next = test;
                highest = next->prio;
            }
        }
        list_del_init(&next->list);
        return next;
    }
    return next;
}
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
    .acquire = pip_acquire,
    .release = pip_release,
    .schedule = pip_schedule,
};
