/**********************************************************************
 * Copyright (c) 2020-2024
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
#include <getopt.h>
#include <ctype.h>
#include <inttypes.h>
#include <strings.h>

#include "parser.h"

#include "list_head.h"
#include "vm.h"

static bool verbose = true;

static bool print_tlb_result = false;

/**
 * Initial process
 */
static struct process init = {
	.pid = 0,
	.list = LIST_HEAD_INIT(init.list),
	.pagetable = {
		.pdes = { NULL },
	},
};

/**
 * Current process. Should not be listed in the @processes
 */
struct process *current = &init;

/**
 * Ready queue. Put @current process to the tail of this list on
 * switch_process(). Don't forget to remove the switched process from the list.
 */
LIST_HEAD(processes);

/**
 * Page table base register
 */
struct pagetable *ptbr = NULL;

/**
 * Map count for each page frame
 */
unsigned int mapcounts[NR_PAGEFRAMES] = { 0 };

/**
 * TLB of the system
 */
struct tlb_entry tlb[NR_TLB_ENTRIES] = {
	{false, 0, 0},
};

extern unsigned int alloc_page(unsigned int vpn, unsigned int rw);
extern void free_page(unsigned int vpn);
extern bool handle_page_fault(unsigned int vpn, unsigned int rw);
extern void switch_process(unsigned int pid);

extern bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int *pfn);
extern void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn);

/**
 * __translate()
 *
 * DESCRIPTION
 *   This function simulates the address translation in MMU.
 *   It translates @vpn to @pfn using the page table pointed by @ptbr.
 *
 * RETURN
 *   @true on successful translation
 *   @false if unable to translate. This includes the case when the page access
 *   is for write (indicated in @rw), but @pte->rw indicates it's read-only.
 */
static bool __translate(unsigned int rw, unsigned int vpn, unsigned int *pfn, bool *from_tlb)
{
    /// 첫 번째 page table 에서 인덱스 번호 찾기
    /// 이거는 16 배수로 배정 받음
	int pd_index = vpn / NR_PTES_PER_PAGE;
    /// 이거는 16개 중에 pte 몇번 인지 찾기
	int pte_index = vpn % NR_PTES_PER_PAGE;

    /// process 에 따른 page table
	struct pagetable *pt = ptbr;
    /// level 1 page index
	struct pte_directory *pd;
    /// level 2 pte number
	struct pte *pte;

	/* Lookup the mapping from TLB */
    /// tlb hit 을 기대 하면서 cpu 레지 스터인 TLB look up
	if (print_tlb_result && lookup_tlb(vpn, rw, pfn)) {
		*from_tlb = true;
		return true;
	}

	/* Nah, TLB miss */
    /// TLB 미스
	*from_tlb = false;

	/* Page table is invalid */
	if (!pt) return false;

	pd = pt->pdes[pd_index];

	/* Page directory does not exist */
	if (!pd) return false;

    /// 두 번째 레벨의 페이지 데이블. pte 구조체. 자기에 해당 하는 pte 찾음.
	pte = &pd->ptes[pte_index];

	/* PTE is invalid */
    /// invalid 는 두종류. 하나는 디스크 에 있을 경우, 다른 하나는 아직 안쓰고 있음
	if (!pte->valid) return false;

	/* Unable to handle the write access */
	if (rw & ACCESS_WRITE) {
		if (!(pte->rw & ACCESS_WRITE)) return false;
	}
	*pfn = pte->pfn;

	/* Insert the mapping into TLB */
	if (print_tlb_result) {
		insert_tlb(vpn, pte->rw, *pfn);
	}

	return true;
}

/**
 * __access_memory
 *
 * DESCRIPTION
 *   Simulate the MMU in the processor and call page fault handler
 *   if necessary.
 *
 * RETURN
 *   @true on successful access
 *   @false if unable to access @vpn for @rw
 */
static bool __access_memory(unsigned int vpn, unsigned int rw)
{
	unsigned int pfn;
	int ret;
	int nr_retries = 0;

	/* Cannot read nor write at the same time!! */
	assert((rw & ACCESS_READ) ^ (rw & ACCESS_WRITE));

	/**
	 * We have NR_PDES_PER_PAGE entries in the outer tables and
	 * NR_PTES_PER_PAGE in inner tables.
	 */
	assert(vpn < NR_PDES_PER_PAGE * NR_PTES_PER_PAGE);

	do {
		bool from_tlb;
		/* Ask MMU to translate VPN */
		if (__translate(rw, vpn, &pfn, &from_tlb)) {
			/* Success on address translation */
			if (print_tlb_result) {
				fprintf(stderr, "%c |", from_tlb ? 'o' : 'x');
			}
			fprintf(stderr, " %3u --> %-3u\n", vpn, pfn);
			return true;
		}

		/**
		 * Failed to translate the address. So, call OS through the page fault
		 * and restart the translation if the fault is successfully handled.
		 * Count the number of retries to prevent buggy translation.
		 */
		 /// 최대 두번 까지 다시 시도
		 /// page fault 처리
		nr_retries++;
	} while ((ret = handle_page_fault(vpn, rw)) == true && nr_retries < 2);

	if (ret == false) {
		fprintf(stderr, "Unable to access %u\n", vpn);
	}
	return ret;
}

static unsigned int __make_rwflag(const char *rw)
{
	int len = strlen(rw);
	unsigned int rwflag = ACCESS_READ;

	for (int i = 0; i < len; i++) {
		if (rw[i] == 'r' || rw[i] == 'R') {
			rwflag |= ACCESS_READ;
		}
		if (rw[i] == 'w' || rw[i] == 'W') {
			rwflag |= ACCESS_WRITE;
		}
	}
	return rwflag;
}

static bool __alloc_page(unsigned int vpn, unsigned int rw)
{
	unsigned int pfn;
	bool from_tlb;

	assert(rw);
	assert(rw & ACCESS_READ);

	/* Check whether the requested VPN is already allocated */
	if (__translate(ACCESS_READ, vpn, &pfn, &from_tlb)) {
		fprintf(stderr, "%u is already allocated to %u\n", vpn, pfn);
		return false;
	}

    /// alloc_page 함수 사용됨.
	pfn = alloc_page(vpn, rw);
	if (pfn == -1) {
		fprintf(stderr, "memory is full\n");
		return false;
	}
	fprintf(stderr, "alloc %3u --> %-3u\n", vpn, pfn);
	return true;
}

static bool __free_page(unsigned int vpn)
{
	unsigned int pfn;
	bool from_tlb;

	if (!__translate(ACCESS_READ, vpn, &pfn, &from_tlb)) {
		fprintf(stderr, "%u is not allocated\n", vpn);
		return false;
	}
	fprintf(stderr, "free %u (pfn %u)\n", vpn, pfn);
	free_page(vpn);

	return true;
}

static void __init_system(void)
{
	ptbr = &init.pagetable;
}

static void __show_pageframes(void)
{
	for (unsigned int i = 0; i < NR_PAGEFRAMES; i++) {
		if (!mapcounts[i]) continue;
		fprintf(stderr, "%3u: %d\n", i, mapcounts[i]);
	}
	fprintf(stderr, "\n");
}

static void __show_pagetable(void)
{
	fprintf(stderr, "\n*** PID %u ***\n", current->pid);

	for (int i = 0; i < NR_PDES_PER_PAGE; i++) {
		struct pte_directory *pd = current->pagetable.pdes[i];

		if (!pd) continue;

		for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
			struct pte *pte = &pd->ptes[j];

			if (!verbose && !pte->valid) continue;
			fprintf(stderr, "%02d:%02d | %c %c%c | %-3d\n", i, j,
				pte->valid ? 'v' : ' ',
				pte->valid ? (pte->rw & ACCESS_READ ? 'r' : ' ') : ' ',
				pte->rw & ACCESS_WRITE ? 'w' : ' ',
				pte->pfn);
		}
		printf("\n");
	}
}

static void __show_tlb(void)
{
	for (int i = 0; i < sizeof(tlb) / sizeof(*tlb); i++) {
		struct tlb_entry *t = tlb + i;

		if (!t->valid) continue;

		fprintf(stderr, "%c%c | %3d -> %-3d\n",
				t->rw & ACCESS_READ ? 'r' : ' ',
				t->rw & ACCESS_WRITE ? 'w' : ' ',
				t->vpn, t->pfn);
	}
}

static void __print_help(void)
{
	printf("  help | ?     : Print out this help message \n");
	printf("  exit         : Exit the simulation\n");
	printf("\n");
	printf("  switch [pid] : Do context switch to pid @pid\n");
	printf("                 Fork @pid if there is no process with the pid\n");
	printf("  show         : Show the page table of the current process\n");
	printf("  frames       : Show the status for each page frame\n");
	printf("  tlb          : Show TLB entries\n");
	printf("\n");
	printf("  alloc [vpn] r|w  : Allocate a page according to the rw flag\n");
	printf("  free [vpn]       : Deallocate the page at VPN @vpn\n");
	printf("  access [vpn] r|w : Access VPN @vpn for read or write\n");
	printf("  read [vpn]       : Equivalent to access @vpn r\n");
	printf("  write [vpn]      : Equivalent to access @vpn w\n");
	printf("\n");
}

static bool strmatch(char * const str, const char *expect)
{
	return (strlen(str) == strlen(expect)) &&
			(strncmp(str, expect, strlen(expect)) == 0);
}

static void __do_simulation(FILE *input)
{
	char command[MAX_COMMAND_LEN] = { 0 };

	__init_system();

	while (fgets(command, sizeof(command), input)) {
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;

		/* Make the command lowercase */
		for (size_t i = 0; i < strlen(command); i++) {
			command[i] = tolower(command[i]);
		}

		nr_tokens = parse_command(command, tokens);
		if (nr_tokens == 0) continue;

		if (nr_tokens == 1) {
			if (strmatch(tokens[0], "exit")) break;
			if (strmatch(tokens[0], "show")) {
				__show_pagetable();
			} else if (strmatch(tokens[0], "frames")) {
				__show_pageframes();
			} else if (strmatch(tokens[0], "tlb")) {
				__show_tlb();
			} else if (strmatch(tokens[0], "help") || strmatch(tokens[0], "?")) {
				__print_help();
			} else {
				printf("Unknown command %s\n", tokens[0]);
			}
		} else if (nr_tokens == 2) {
			unsigned int arg = strtoimax(tokens[1], NULL, 0);

			if (strmatch(tokens[0], "switch") || strmatch(tokens[0], "s")) {
				switch_process(arg);
			} else if (strmatch(tokens[0], "free") || strmatch(tokens[0], "f")) {
				__free_page(arg);
			} else if (strmatch(tokens[0], "read") || strmatch(tokens[0], "r")) {
				__access_memory(arg, ACCESS_READ);
			} else if (strmatch(tokens[0], "write") || strmatch(tokens[0], "w")) {
				__access_memory(arg, ACCESS_WRITE);
			} else {
				printf("Unknown command %s\n", tokens[0]);
			}
		} else if (nr_tokens == 3) {
			unsigned int vpn = strtoimax(tokens[1], NULL, 0);
			unsigned int rw = __make_rwflag(tokens[2]);

			if (strmatch(tokens[0], "alloc") || strmatch(tokens[0], "a")) {
				if (!__alloc_page(vpn, rw)) break;
			} else if (strmatch(tokens[0], "access")) {
				__access_memory(vpn, rw);
			} else {
				printf("Unknown command %s\n", tokens[0]);
			}
		} else {
			assert(!"Unknown command in trace");
		}

		if (verbose) printf("%d >> ", current->pid);
	}
}

static void __print_usage(const char * name)
{
	printf("Usage: %s {-q} {-f [workload file]}\n", name);
	printf("\n");
	printf("  -t: Show TLB result\n");
	printf("  -q: Run quietly\n\n");
}

int main(int argc, char * argv[])
{
	int opt;
	FILE *input = stdin;

	while ((opt = getopt(argc, argv, "qht")) != -1) {
		switch (opt) {
		case 'q':
			verbose = false;
			break;
		case 't':
			print_tlb_result = true;
			break;
		case 'h':
		default:
			__print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (verbose && !argv[optind]) {
		printf("*******************************************************\n");
		printf("            V M     S I M U L A T O R\n");
		printf("\n");
		printf("                                   >> 2024 Spring <<\n");
		printf("\n");
		printf("********************************************************\n");
	}

	if (argv[optind]) {
		if (verbose) printf("Use file \"%s\" for input.\n", argv[optind]);

		input = fopen(argv[optind], "r");
		if (!input) {
			fprintf(stderr, "No input file %s\n", argv[optind]);
			return EXIT_FAILURE;
		}
		verbose = false;
	} else {
		if (verbose) printf("Use stdin for input.\n");
	}

	if (verbose) {
		printf("Type 'help' or '?' for help.\n\n");
		printf("%d >> ", current->pid);
	}

	__do_simulation(input);

	if (input != stdin) fclose(input);

	return EXIT_SUCCESS;
}
