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

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_NR_TOKENS	32		/* Maximum length of tokens in a command */
#define MAX_TOKEN_LEN	128		/* Maximum length of single token */
#define MAX_COMMAND_LEN	1024	/* Maximum length of assembly string */

int parse_command(char *command, char *tokens[]);

#endif
