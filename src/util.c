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

	ctx->line = strdup(line);

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

	if(ctx->line[strlen(ctx->line)-1] == ':')
		handle_symbol(ctx->line,nline);
	else
		nline++;
}

void
handle_symbol(char* line,int offset)
{
	char* dup;

	if(!line)
		return;

	dup = strdup(line);
	dup[strlen(dup)-1] = '\0';
/*
	if(searchSymbols(dup))
		die("* Syntax error, duplicate symbols declaration at line %d.\n",offset);
*/
	llmne.symbols = realloc(llmne.symbols,sizeof(struct llmne_sym) * ++llmne.syms_len);
	llmne.symbols[llmne.syms_len-1].name = strdup(dup);
	llmne.symbols[llmne.syms_len-1].offset = offset;

	free(dup);
}

void
resolveSymbols(void)
{
	char* line = xmalloc(256);
	TokenCtx ctx;

	while(fgets(line,256,i_stream))
	{
		line[strlen(line)-1] = '\0';

		if(line[0] == '#' || line[0] == '\0')
			continue;

		TokenParse(&ctx,line);
	}

	free(line);

	nline = 0;

	fseek(i_stream,0,SEEK_SET);
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
	char* ptr = NULL;

	if(!strcmp(ctx->instr,"READ")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,10,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,10,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"WRITE")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,11,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,11,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"POP")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,12,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,12,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"PUSH")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,13,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,13,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"ADD")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,14,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,14,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SUB")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,15,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,15,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MUL")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,16,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,16,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DIV")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,17,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,17,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MOD")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,18,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,18,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"AND")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,19,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,19,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"OR")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,20,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,20,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"XOR")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,21,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,21,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOT")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,22,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,22,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHL")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,23,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,23,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHR")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,24,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,24,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEL")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,25,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,25,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOP")) {
		return newInstr(ctx,26,26);
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
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,28,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,28,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JN")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,29,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,29,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JZ")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,30,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,30,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JM")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,31,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,31,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"JG")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,32,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,32,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"EXIT")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,33,(searchSymbols(ctx->args[0]))->offset + offset);
		else
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

		return newInstr(ctx,34,offset);
	} else if (!strcmp(ctx->instr,"INC")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,35,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,35,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEC")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,36,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,36,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"CALL")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,37,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,37,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"RET")) {
		return newInstr(ctx,38,00);
	} else if (!strcmp(ctx->instr,"STPUSH")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,39,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,39,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"STPOP")) {
		if((ptr = strchr(ctx->args[0],'+')))
		{
			offset = atoi(ptr+1);
			ptr[0] = '\0';
		}

		if(searchSymbols(ctx->args[0]))
			return newInstr(ctx,40,(searchSymbols(ctx->args[0]))->offset + offset);
		else
			return newInstr(ctx,40,atoi(ctx->args[0]));
	}

	return (struct llmne_instr) { "VAR" ,{ "VAR", 0,0,0 },0,0, atoi(ctx->instr) };
}

void
printInstr()
{
	int i;
	
	for(i = 0;i < llmne.instr_len;i++)
		printf("%04d  %s\n",llmne.instr[i].opcode,
					   llmne.instr[i].ctx.line);
}

void
lxs_execute()
{
	FILE *fp;
	int i;

	if(!(fp = fopen("/tmp/llmne.o","w")))
		xdie("tmpfile");

	for(i = 0;i < llmne.instr_len;i++)
		fprintf(fp,"%04d  %s\n",llmne.instr[i].opcode,
				llmne.instr[i].ctx.line);

	fclose(fp);

	putchar('\n');

	i = system("lxs -s /tmp/llmne.o");

	unlink("/tmp/llmne.o");
}

void
dump_symbols()
{
	int i;

	for(i = 0;i < llmne.syms_len;i++)
		printf("[DEBUG] |-- @ Name: %s.\n"
			  "[DEBUG] |-> @ Offset: %02d.\n",llmne.symbols[i].name,llmne.symbols[i].offset);
}

void
llmne_parse_all(char* line)
{
	TokenCtx tokens;
	
	TokenParse(&tokens,line);

	if(tokens.instr[strlen(tokens.instr)-1] != ':') {
		llmne.instr = realloc(llmne.instr,++llmne.instr_len * sizeof(struct llmne_instr));
		llmne.instr[llmne.instr_len-1] = InstrParse(&tokens);
	}

	free(tokens.args);
	free(tokens.instr);
}
