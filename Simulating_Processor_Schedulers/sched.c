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
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

#include "list_head.h"

#include "parser.h"
#include "process.h"
#include "resource.h"

#include "sched.h"

/**
 * List head to hold the processes ready to run
 */
LIST_HEAD(readyqueue);

/**
 * The process that is currently running
 */
struct process *current = NULL;

/**
 * Number of generated ticks since the simulator was started
 */
unsigned int ticks = 0;

/**
 * Resources in the system.
 */
struct resource resources[NR_RESOURCES];

/**
 * Following code is to maintain the simulator itself.
 */
struct resource_schedule {
	unsigned int resource_id;
	unsigned int at;
	unsigned int duration;
	struct list_head list;
};

static LIST_HEAD(__forkqueue);

bool quiet = false;

static const char *__process_status_sz[] = {
	"RDY",
	"RUN",
	"BLK",
	"EXT",
};

/**
 * Assorted schedulers
 */
extern struct scheduler fcfs_scheduler;
extern struct scheduler sjf_scheduler;
extern struct scheduler stcf_scheduler;
extern struct scheduler rr_scheduler;
extern struct scheduler prio_scheduler;
extern struct scheduler pa_scheduler;
extern struct scheduler pcp_scheduler;
extern struct scheduler pip_scheduler;

static struct scheduler *sched = &fcfs_scheduler;

void dump_status(void)
{
	struct process *p;

	printf("***** CURRENT *********\n");
	if (current) {
		printf("%2d (%s): %d + %d/%d at %d\n", current->pid,
		       __process_status_sz[current->status], current->__starts_at, current->age,
		       current->lifespan, current->prio);
	}

	printf("***** READY QUEUE *****\n");
	list_for_each_entry(p, &readyqueue, list) {
		printf("%2d (%s): %d + %d/%d at %d\n", p->pid, __process_status_sz[p->status],
		       p->__starts_at, p->age, p->lifespan, p->prio);
	}

	printf("***** RESOURCES *******\n");
	for (int i = 0; i < NR_RESOURCES; i++) {
		struct resource *r = resources + i;

		if (r->owner || !list_empty(&r->waitqueue)) {
			printf("%2d: owned by ", i);
			if (r->owner) {
				printf("%d\n", r->owner->pid);
			} else {
				printf("no one\n");
			}

			list_for_each_entry(p, &r->waitqueue, list) {
				printf("    %d is waiting\n", p->pid);
			}
		}
	}
	printf("\n\n");

	return;
}

#define __print_event(pid, string, args...)   \
	do {                                      \
		fprintf(stderr, "%3d: ", ticks);      \
		for (unsigned int i = 0; i < pid; i++) {       \
			fprintf(stderr, "    ");          \
		}                                     \
		fprintf(stderr, string "\n", ##args); \
	} while (0);

static inline bool strmatch(char *const str, const char *expect)
{
	return (strlen(str) == strlen(expect)) && (strncmp(str, expect, strlen(expect)) == 0);
}

static void __briefing_schedule(struct process *p)
{
	struct resource_schedule *rs;

	if (quiet)
		return;

	printf("- Process %d: Forked at tick %d and run for %d tick%s with initial priority %d\n",
	       p->pid, p->__starts_at, p->lifespan, p->lifespan >= 2 ? "s" : "", p->prio);

	list_for_each_entry(rs, &p->__resources_to_acquire, list) {
		printf("    Acquire resource [%d] at %d for %d\n", rs->resource_id, rs->at,
		       rs->duration);
	}
}

static int __load_script(char *const filename)
{
	char line[MAX_COMMAND_LEN];
	struct process *p = NULL;

	FILE *file = fopen(filename, "r");
	while (fgets(line, sizeof(line), file)) {
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = parse_command(line, tokens);

		if (nr_tokens == 0)
			continue;

		if (strmatch(tokens[0], "process")) {
			assert(nr_tokens == 2);
			/* Start processor description */
			p = malloc(sizeof(*p));
			memset(p, 0x00, sizeof(*p));

			p->pid = atoi(tokens[1]);

			INIT_LIST_HEAD(&p->list);
			INIT_LIST_HEAD(&p->__resources_to_acquire);
			INIT_LIST_HEAD(&p->__resources_holding);

			continue;
		} else if (strmatch(tokens[0], "end")) {
			/* End of process description */
			assert(p);

			list_add_tail(&p->list, &__forkqueue);

			__briefing_schedule(p);
			p = NULL;

			continue;
		}

		if (strmatch(tokens[0], "lifespan")) {
			assert(nr_tokens == 2);
			p->lifespan = atoi(tokens[1]);
		} else if (strmatch(tokens[0], "prio")) {
			assert(nr_tokens == 2);
			p->prio = p->prio_orig = atoi(tokens[1]);
		} else if (strmatch(tokens[0], "start")) {
			assert(nr_tokens == 2);
			p->__starts_at = atoi(tokens[1]);
		} else if (strmatch(tokens[0], "acquire")) {
			struct resource_schedule *rs;
			assert(nr_tokens == 4);

			rs = malloc(sizeof(*rs));

			*rs = (struct resource_schedule) {
				.resource_id = atoi(tokens[1]),
				.at = atoi(tokens[2]),
				.duration = atoi(tokens[3]),
			};

			list_add_tail(&rs->list, &p->__resources_to_acquire);
		} else {
			fprintf(stderr, "Unknown property %s\n", tokens[0]);
			return false;
		}
	}
	fclose(file);
	if (!quiet)
		printf("\n");
	return true;
}

/**
 * Fork process on schedule
 */
static int __fork_on_schedule()
{
	int nr_forked = 0;
	struct process *p, *tmp;
	list_for_each_entry_safe(p, tmp, &__forkqueue, list) {
		if (p->__starts_at <= ticks) {
			list_move_tail(&p->list, &readyqueue);
			p->status = PROCESS_READY;
			__print_event(p->pid, "N");
			if (sched->forked)
				sched->forked(p);
			nr_forked++;
		}
	}
	return nr_forked;
}

/**
 * Exit the process
 */
static void __exit_process(struct process *p)
{
	/* Make sure the process is not attached to some list head */
	assert(list_empty(&p->list));

	/* Make sure the process is not holding any resource */
	assert(list_empty(&p->__resources_holding));

	/* Make sure there is no pending resource to acquire */
	assert(list_empty(&p->__resources_to_acquire));

	if (sched->exiting)
		sched->exiting(p);

	__print_event(p->pid, "X");

	free(p);
}

/**
 * Process resource acqutision
 */
static bool __run_current_acquire()
{
	struct resource_schedule *rs, *tmp;

	list_for_each_entry_safe(rs, tmp, &current->__resources_to_acquire, list) {
		if (rs->at == current->age) {
			assert(sched->acquire && "scheduler.acquire() not implemented");

			/* Callback to acquire the resource */
			if (!sched->acquire(rs->resource_id)) {
				__print_event(current->pid, "=[%d]", rs->resource_id);
				return false;
			}

			list_move_tail(&rs->list, &current->__resources_holding);

			__print_event(current->pid, "+[%d]", rs->resource_id);
		}
	}

	return true;
}

/**
 * Process resource release
 */
static void __run_current_release()
{
	struct resource_schedule *rs, *tmp;

	list_for_each_entry_safe(rs, tmp, &current->__resources_holding, list) {
		if (--rs->duration != 0) {
			continue;
		}
		assert(sched->release && "scheduler.release() not implemented");

		/* Callback the release() */
		sched->release(rs->resource_id);

		__print_event(current->pid, "-[%d]", rs->resource_id);

		list_del(&rs->list);
		free(rs);
	}
}

/***********************************************************************
 * The main loop for the scheduler simulation
 */
static void __do_simulation(void)
{
	assert(sched->schedule && "scheduler.schedule() not implemented");

	while (true) {
		struct process *prev;

		/* Fork processes on schedule */
		__fork_on_schedule();

		/* Ask scheduler to pick the next process to run */
		prev = current;
		current = sched->schedule(); /// 여기서 current 선택

		/* If the system has run a process in the previous tick */
		if (prev) {
			/* Update the process status */
			if (prev->status == PROCESS_RUNNING) {
				prev->status = PROCESS_READY;
			} /// 전 process ready que 에 넣기

			/* Decommission it if completed */
			if (prev->age == prev->lifespan) {
				prev->status = PROCESS_EXIT;
				__exit_process(prev); /// 전 process 가 끝났 으면 해당 process 종료
			}
		}

		/* No process is ready to run at this moment */
		if (!current) { /// next == NULL
			/* Quit simulation if no pending process exists */
			if (list_empty(&readyqueue) && list_empty(&__forkqueue)) {
				break;
			}

			/* Idle temporarily */
			fprintf(stderr, "%3d: idle\n", ticks);
		} else { /// next 가 선택 되면
			/* Execute the current process */
			current->status = PROCESS_RUNNING;

			/* Ensure that @current is detached from any list */
			assert(list_empty(&current->list));

			/* Try acquiring scheduled resources */
			if (__run_current_acquire()) {
				/* Succesfully acquired all the resources to make a progress */
				__print_event(current->pid, "%d", current->pid);

				/* So, it ages by one tick */
				current->age++;

				/* And performs scheduled releases */
				__run_current_release();
			} else {
				/**
				 * The current is blocked while acquiring resource(s).
				 * In this case, @current could not make a progress in this tick.
				 * Thus, it does not get aged nor is unable to perform releases
				 */
			}
		}

		/* Increase the tick counter */
		ticks++;
	}
}

static void __initialize(void)
{
	INIT_LIST_HEAD(&readyqueue);

	for (int i = 0; i < NR_RESOURCES; i++) {
		resources[i].owner = NULL;
		INIT_LIST_HEAD(&(resources[i].waitqueue));
	}

	INIT_LIST_HEAD(&__forkqueue);

	if (quiet)
		return;
	printf("               _              _ \n");
	printf("              | |            | |\n");
	printf("      ___  ___| |__   ___  __| |\n");
	printf("     / __|/ __| '_ \\ / _ \\/ _` |\n");
	printf("     \\__ \\ (__| | | |  __/ (_| |\n");
	printf("     |___/\\___|_| |_|\\___|\\__,_|\n");
	printf("\n");
	printf("                                 2024 Spring\n");
	printf("      Simulating %s scheduler\n", sched->name);
	printf("\n");
	printf("****************************************************\n");
	printf("   N: Forked\n");
	printf("   X: Finished\n");
	printf("   =: Blocked\n");
	printf("  +n: Acquire resource n\n");
	printf("  -n: Release resource n\n");
	printf("\n");
}

static void __print_usage(char *const name)
{
	printf("Usage: %s {-q} -[f|s|S|r|a|p|i] [process script file]\n", name);
	printf("\n");
	printf("  -q: Run quietly\n\n");
	printf("  -f: Use FCFS scheduler (default)\n");
	printf("  -s: Use SJF scheduler\n");
	printf("  -S: Use STCF scheduler\n");
	printf("  -r: Use Round-robin scheduler\n");
	printf("  -p: Use Priority scheduler\n");
	printf("  -a: Use Priority scheduler with aging\n");
	printf("  -c: Use Priority scheduler with PCP\n");
	printf("  -i: Use Priority scheduler with PIP\n");
	printf("\n");
}

int main(int argc, char *const argv[])
{
	int opt;
	char *scriptfile;

	while ((opt = getopt(argc, argv, "qfsSrpaich")) != -1) {
		switch (opt) {
		case 'q':
			quiet = true;
			break;

		case 'f':
			sched = &fcfs_scheduler;
			break;
		case 's':
			sched = &sjf_scheduler;
			break;
		case 'S':
			sched = &stcf_scheduler;
			break;
		case 'r':
			sched = &rr_scheduler;
			break;
		case 'p':
			sched = &prio_scheduler;
			break;
		case 'a':
			sched = &pa_scheduler;
			break;
		case 'i':
			sched = &pip_scheduler;
			break;
		case 'c':
			sched = &pcp_scheduler;
			break;
		case 'h':
		default:
			__print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		__print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	scriptfile = argv[optind];

	__initialize();

	if (!__load_script(scriptfile)) {
		return EXIT_FAILURE;
	}

	if (sched->initialize && sched->initialize()) {
		return EXIT_FAILURE;
	}

	__do_simulation();

	if (sched->finalize) {
		sched->finalize();
	}

	return EXIT_SUCCESS;
}
