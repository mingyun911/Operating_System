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
#include <string.h>

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

typedef unsigned char bool;
#define true	1
#define false	0

#include "list_head.h"
#include "parser.h"
#include <errno.h>

struct entry{
    char* alias;
    char* sentence;
    struct list_head list;
};


LIST_HEAD(stack);


int run_command(int nr_tokens, char *tokens[])
{

    if(strcmp(tokens[0], "exit") == 0)
        return 0;
    if(strcmp(tokens[0], "cd") == 0){
        char* path = tokens[1];
        if(path == NULL || strcmp(path, "~")==0){
            path = getenv("HOME");
            chdir(path);
        }
        else{
            chdir(path);
        }
        return 1;
    }
    if(strcmp(tokens[0], "alias") == 0){
        if(nr_tokens > 1){
            struct entry* new = (struct entry*)malloc(sizeof(struct entry));
            new->alias = strdup(tokens[1]);
            char* string = (char*)malloc(1000 * sizeof(char));
            for(int x = 2; x < nr_tokens; x++) {
                strcat(string, tokens[x]);
                if(x < nr_tokens - 1)
                    strcat(string, " ");
            }
            new->sentence = strdup(string);
            list_add_tail(&new->list, &stack);
            return 1;
        }
        else{
            struct entry *cur;
            list_for_each_entry(cur, &stack, list) {
                fprintf(stderr, "%s: %s\n", cur->alias, cur->sentence);
            }
            return 1;
        }
    }
    /// 문자 대치 ///
    int temp = 0;
    for(int x = 0; x < nr_tokens; x++){
        int result = 0;
        struct entry *cur;
        list_for_each_entry(cur, &stack, list) {
            if(strcmp(tokens[x], cur->alias) == 0) {
                tokens[temp] = strdup(cur->sentence);
                temp++;
                result++;
                break;
            }
        }
        if (result == 0) {
            tokens[temp] = strdup(tokens[x]);
            temp++;
        }
    }

    /// 다시 파싱 ///
    char *new_tokens[MAX_COMMAND_LEN];
    int new_nr_tokens = 0;

    for(int x = 0; x < nr_tokens; x++) {
        char *temporary = strdup(tokens[x]);
        char *token = strtok(temporary, " ");

        while(token != NULL) {
            new_tokens[new_nr_tokens++] = strdup(token);
            token = strtok(NULL, " ");
        }
        free(temporary);
    }
    new_tokens[new_nr_tokens] = NULL;


    /// 파이프 있는 지 없는 지 체크 하기 ///
    int result = 0;
    for(int x = 0; x < new_nr_tokens; x++){
        if(strcmp(new_tokens[x], "|") == 0){
            result = 1;
            break;
        }
    }

    /// 파이프 없다 ///
    if(result == 0){
        pid_t pid = fork();
        if(pid == -1){
            return -1;
        }
        else if(pid == 0){
            execvp(new_tokens[0], new_tokens);
            return -1;
        }
        else{
            int status;
            wait(&status);
            return 1;
        }
    }

        /// 파이프 있다 ///
    else {

        int count = 0;
        nr_tokens = 0;
        for (int x = 0; x < new_nr_tokens; x++) {
            if (strcmp(new_tokens[x], "|") == 0) {
                count++;
                break;
            }
            tokens[nr_tokens++] = strdup(new_tokens[x]);
            count++;
        }
        tokens[nr_tokens] = NULL;

        pid_t pid = fork();
        if (pid == -1) {
            return -1;
        }
        else if (pid == 0) {
            int pipe_fd[2];
            pipe(pipe_fd);
            pid_t c_pid = fork();
            if(c_pid == -1){
                return -1;
            }
            else if(c_pid == 0){ /// echo 자식 ///
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
                execvp(tokens[0], tokens);
                return -EINVAL;
            }
            else{ /// world 부모 ///
                nr_tokens = 0;
                for (int x = count; x < new_nr_tokens; x++) {
                    tokens[nr_tokens++] = strdup(new_tokens[x]);
                }
                tokens[nr_tokens] = NULL;
                close(pipe_fd[1]);
                dup2(pipe_fd[0], STDIN_FILENO);
                close(pipe_fd[0]);
                execvp(tokens[0], tokens);
                return -EINVAL;
            }
        }
        else {
            int status;
            wait(&status);
            return 1;
        }
    }
}



/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{
}

////
