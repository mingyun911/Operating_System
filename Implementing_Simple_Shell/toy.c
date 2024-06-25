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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int main(int argc, const char *argv[])
{
	fprintf(stderr, "pid  = %d\n", getpid());
	fprintf(stderr, "argc = %d\n", argc);

	for (int i = 0; i < argc; i++) {
		fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
	}

	if (argc > 2 && strcmp(argv[1], "zzz") == 0) {
		sleep(atoi(argv[2]));
	}
	fprintf(stderr, "done!\n");

	return EXIT_SUCCESS;
}
