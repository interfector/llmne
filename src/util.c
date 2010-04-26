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
					syms[i++].offset = 0;
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
newInstr(char* name,char** args,int op,int code)
{
	struct llmne_instr instr;
	int i;
/*
	instr.instr = strdup(ctx->name);
	instr.args = malloc(ctx->argc);

	for(i = 0;i < ctx->argc;i++)
		instr.args[i] = strdup(ctx->args[i]);

	instr.argc = ctx->argc;
*/
	instr.instr_code = op;
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
		printf("10%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,10,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"WRITE")) {
		printf("11%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,11,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"POP")) {
		printf("12%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,12,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"PUSH")) {
		printf("13%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,14,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"ADD")) {
		printf("14%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,15,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SUB")) {
		printf("15%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,16,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MUL")) {
		printf("16%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,17,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DIV")) {
		printf("17%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,18,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MOD")) {
		printf("18%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,19,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"AND")) {
		printf("19%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,20,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"OR")) {
		printf("20%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,21,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"XOR")) {
		printf("21%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,22,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOT")) {
		printf("22%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,23,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHL")) {
		printf("23%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,24,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHR")) {
		printf("24%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,25,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEL")) {
		printf("25%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,26,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOP")) {
		printf("2600\n");

		return newInstr(ctx->instr,ctx->args,26,0);
	} else if (!strcmp(ctx->instr,"JMP")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
		{
			printf("27%02d\n",(searchSymbols(ctx->args[0]))->offset + offset);

			return newInstr(ctx->instr,ctx->args,27,(searchSymbols(ctx->args[0]))->offset + offset);
		}
		else
			printf("27%02d\n",atoi(ctx->args[0]));

			return newInstr(ctx->instr,ctx->args,27,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"CMP")) {
		printf("28%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,28,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JN")) {
		printf("29%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,29,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JZ")) {
		printf("30%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,30,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JM")) {
		printf("31%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,31,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JG")) {
		printf("32%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,32,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"EXIT")) {
		printf("33%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,33,atoi(ctx->args[0]));
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

		printf("34%02d\n",offset);

		return newInstr(ctx->instr,ctx->args,34,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"INC")) {
		printf("35%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,35,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEC")) {
		printf("36%02d\n",atoi(ctx->args[0]));

		return newInstr(ctx->instr,ctx->args,36,atoi(ctx->args[0]));
	}
		
}
