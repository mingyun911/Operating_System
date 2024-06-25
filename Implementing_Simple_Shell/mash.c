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
#include <getopt.h>
#include <errno.h>

#include "parser.h"

extern int run_command(int nr_tokens, char *tokens[]);
extern int initialize(int argc, char * const argv[]);
extern void finalize(int argc, char * const argv[]);

static bool __verbose = true;

static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *cwd = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, cwd, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;
		__print_prompt();
	
		if (!fgets(command, sizeof(command), stdin)) break;

		nr_tokens = parse_command(command, tokens);
		if (nr_tokens == 0) continue;
/*
        for(int x = 0; x < nr_tokens; x++){
            printf("command[%d]: %s\n", x, tokens[x]);
        }*/
		ret = run_command(nr_tokens, tokens);
		if (ret < 0) {
			fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		}

		free_command_tokens(tokens);

		if (ret == 0 || ret == -EINVAL) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
