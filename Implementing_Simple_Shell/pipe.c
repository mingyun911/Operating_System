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

const char *msg = "Hello world!\n";

void sighandler(int signal)
{
	fprintf(stderr, "%d: SIGPIPE\n", signal);
}

int main(int argc, const char *argv[])
{
	/*
	struct sigaction sa = {
		.sa_handler = sighandler,
		.sa_flags = 0,
	}, old_sa;
	sigaction(SIGPIPE, &sa, &old_sa);
	*/

	fprintf(stderr, ">> Start\n");
	sleep(1);
	fprintf(stderr, ">> Printing to the pipe\n");

	/*
	printf(msg);
	fflush(stdout);
	*/
	write(STDOUT_FILENO, msg, strlen(msg));

	fprintf(stderr, ">> Done\n");

	return EXIT_SUCCESS;
}
