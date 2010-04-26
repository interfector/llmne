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


#include <llmne.h>
#include <util.h>

void
handle(int sig,siginfo_t *si, void* unused)
{
	printf("\nSIGSEGV at <0x%lx>.\n",(long)si->si_addr);
	
	exit(1);
}

void
init_signal(void)
{
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handle;

	if(sigaction(SIGSEGV,&sa,NULL) == -1)
		xdie("sigaction");
}

void*
xmalloc(int size)
{
	void* p = malloc(size);

	if(!p)
		xdie("malloc");

	return p;
}

void
die(char* fmt,...)
{
	va_list v;

	va_start(v,fmt);

	vfprintf(stderr,fmt,v);

	va_end(v);

	exit(1);
}

void
banner(void)
{
	printf(" _ _                      \n"
		  "| | |_ __ ___  _ __   ___ \n"
		  "| | | '_ ` _ \\| '_ \\ / _ \\\n"
		  "| | | | | | | | | | |  __/\n"
		  "|_|_|_| |_| |_|_| |_|\\___|\n\n");
}
			                     
char*
trim(char* string)
{
	char* trim = malloc(strlen(string));
	int i,k;

	for(i = 0,k = 0;i < strlen(string);i++)
		if(string[i] != ' ' && 
		   string[i] != '\r' &&
 		   string[i] != '\n' &&
		   string[i] != '\t' &&
		   string[i] != '\x0B')
		trim[k++] = string[i];

	return trim;
}

void
TokenParse(TokenCtx *ctx,char* line)
{

	char* token;

	token = strtok(line, " \t");

	if(!token)
	{
		if(suppress_error)
		{
			printf("* Syntax error, missing operand after instruction at line: %d.\n",nline);

			return;
		} else
			die("* Syntax error, missing operand after instruction at line: %d.\n",nline);
	}

	ctx->instr = strdup(token);
	ctx->args = (char**) malloc(sizeof(char*));
	ctx->argc = 1;

	while((token = strtok(NULL,",")))
	{
		ctx->args = (char**) realloc(ctx->args,sizeof(char*) * ctx->argc);
		ctx->args[(ctx->argc++)-1] = strdup(token);
	}

	if(ctx->argc < 2)
		if(strcmp(ctx->instr,"EXIT") && strcmp(ctx->instr,"NOP") && ctx->instr[strlen(ctx->instr)-1] != ':') 
			die("* Syntax error, too few operands after instruction at line: %d.\n",nline);
}



struct llmne_sym*
resolveSymbols(int* len)
{
	int i = 0,offs = 0;
	char* line = xmalloc(256);
	struct llmne_sym * syms = xmalloc(sizeof(struct llmne_sym));

	while(fgets(line,256,i_stream))
	{
		offs++;

		if(!line)
			continue;

		line[strlen(line)-1] = '\0';

		if(line[strlen(line)-1] == ':')
		{
			line[strlen(line)-1] = '\0';

			if(line[0] != '\0')
			{
				syms = (struct llmne_sym*)realloc(syms,(i+1) * sizeof(struct llmne_sym));

				syms[i].name = strdup(line);
				//if(!i)
					syms[i++].offset = /*0*/offs;
				//else
				//	syms[i++].offset = syms[i-1].offset + getInstrFromSymbol(syms[i-1].name,syms[i].name);
			}
		}
	}

	*len = i;

	fseek(i_stream,0,SEEK_SET);

	free(line);

	return syms;
}

struct llmne_sym*
searchSymbols(char* name)
{
	int i;

	for(i = 0;i < llmne.syms_len;i++)
		if(!strcmp(llmne.symbols[i].name,name))
			return &llmne.symbols[i];

	return NULL;
}

struct llmne_instr
newInstr(TokenCtx *ctx,int op,int code)
{
	struct llmne_instr instr;
	int i;

	instr.instr = strdup(ctx->instr);

	memcpy(&instr.ctx,ctx,sizeof(TokenCtx));
	
	instr.instr_code = op;
	instr.code = code;
	instr.opcode = op * 100 + code;

	return instr;
}

struct llmne_instr
InstrParse(TokenCtx* ctx)
{ /* TODO adds STORE,PRINT,SET and fix 
	the multiplce action instruction and 
	the address calculation */

	int offset = 0;
	char* ptr;

	if(!strcmp(ctx->instr,"READ")) {
		return newInstr(ctx,10,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"WRITE")) {
		return newInstr(ctx,11,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"POP")) {
		return newInstr(ctx,12,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"PUSH")) {
		return newInstr(ctx,14,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"ADD")) {
		return newInstr(ctx,15,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SUB")) {
		return newInstr(ctx,16,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MUL")) {
		return newInstr(ctx,17,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DIV")) {
		return newInstr(ctx,18,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MOD")) {
		return newInstr(ctx,19,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"AND")) {
		return newInstr(ctx,20,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"OR")) {
		return newInstr(ctx,21,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"XOR")) {
		return newInstr(ctx,22,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOT")) {
		return newInstr(ctx,23,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHL")) {
		return newInstr(ctx,24,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHR")) {
		return newInstr(ctx,25,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEL")) {
		return newInstr(ctx,26,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOP")) {
		return newInstr(ctx,26,0);
	} else if (!strcmp(ctx->instr,"JMP")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,27,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,27,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"CMP")) {
		return newInstr(ctx,28,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JN")) {
		return newInstr(ctx,29,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JZ")) {
		return newInstr(ctx,30,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JM")) {
		return newInstr(ctx,31,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JG")) {
		return newInstr(ctx,32,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"EXIT")) {
		return newInstr(ctx,33,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DISPLAY")) {
		if(!strcmp(ctx->args[0],"INT"))
			offset = 0;
		else if(!strcmp(ctx->args[0],"HEX"))
			offset = 1;
		else if(!strcmp(ctx->args[0],"BIN"))
			offset = 2;
		else if(!strcmp(ctx->args[0],"CHAR"))
			offset = 3;
		else if(!strcmp(ctx->args[0],"STRING"))
			offset = 4;
		else
			offset = 0;

		return newInstr(ctx,34,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"INC")) {
		return newInstr(ctx,35,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEC")) {
		return newInstr(ctx,36,atoi(ctx->args[0]));
	}	

	return (struct llmne_instr) { 0,{ 0 },0,0,0 };
}
