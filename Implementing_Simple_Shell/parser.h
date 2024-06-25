/**********************************************************************
 * Copyright (c) 2020-2023
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

#define MAX_NR_TOKENS	32	/* Maximum length of tokens in a command */
#define MAX_TOKEN_LEN	128	/* Maximum length of single token */
#define MAX_COMMAND_LEN	4096 /* Maximum length of assembly string */


/***********************************************************************
 * parse_command()
 *
 * DESCRIPTION
 *  Parse @command, and put each command token into @tokens[].
 *
 *  A command token is defined as a string without any whitespace (i.e., *space*
 *  and *tab* in this programming assignment). For exmaple,
 *   command = "  cp  -pr /home/sslab   /path/to/dest  "
 *
 *  then, nr_tokens = 4, and tokens is
 *    tokens[0] = "cp"
 *    tokens[1] = "-pr"
 *    tokens[2] = "/home/sslab"
 *    tokens[3] = "/path/to/dest"
 *    tokens[>=4] = NULL
 *
 *  Each token is allocated from the heap, so you need to deallcate them by 
 *  calling @free_command_tokens after use.
 *
 * RETURN VALUE
 *  Return the number of @tokens[]
 *
 */
int parse_command(char *command, char *tokens[]);


/***********************************************************************
 * free_commmand_tokens()
 *
 * DESCRIPTION
 *  Deallocate @tokens.
 *
 * HINT
 *  Actually this function does not care whether the entries in @tokens are
 *  allocated from @parse_command() or somewhere else ;-)
 */
void free_command_tokens(char *tokens[]);

#endif
