/*
* LLMNE, last lol mnemonic
* Copyright (C) 2010 nex
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LLMNE_UTIL_
#define _LLMNE_UTIL_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define xdie(x) do{ perror(x); exit(1); }while(1)

typedef struct {
	char* line;
	char* instr;

	char** args;
	int    argc;
} TokenCtx;

void  handle(int,siginfo_t*,void*);
void* xmalloc(int);
void  die(char*,...);
void  banner(void);
void  init_signal(void);
void  TokenParse(TokenCtx*,char*);

struct llmne_sym*   searchSymbols(char*);
struct llmne_instr  InstrParse(TokenCtx*);
struct llmne_instr  newInstr(TokenCtx*,int,int);

void handle_symbol(char*,int);
void resolveSymbols(FILE*);
void relocateAllSymbols(void);

void llmne_parse_all(char*);
void llmne_preprocess(char*);
void printInstr();
void lxs_execute();
void dump_symbols();

extern int suppress_error;
extern int nline;

extern struct llmne_file llmne;

#endif
