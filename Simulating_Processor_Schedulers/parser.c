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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

int parse_command(char *command, char *tokens[])
{
	char *curr = command;
	int token_started = false;
	int nr_tokens = 0;

	while (*curr != '\0') {
		if (isspace(*curr)) {
			*curr = '\0';
			token_started = false;
		} else {
			if (!token_started) {
				tokens[nr_tokens] = curr;
				nr_tokens += 1;
				token_started = true;
			}
		}

		curr++;
	}

	/* Remove comments */
	for (int i = 0; i < nr_tokens; i++) {
		if (strncmp(tokens[i], "#", strlen("#")) == 0) {
			nr_tokens = i;
			tokens[i] = NULL;
			break;
		}
	}

	return nr_tokens;
}
