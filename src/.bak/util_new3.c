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
	printf(
		" _ _                \n"
		"| | |_ __  _ _  ___ \n"
		"| | | '  \\| ' \\/ -_)\n"
		"|_|_|_|_|_|_||_\\___|\n"
		"                    \n");
}
			                     
char*
trim(char* string)
{
	char* trim = malloc(strlen(string));
	int i,k;

	memset(trim,0,strlen(string));

	for(i = 0,k = 0;i < strlen(string);i++)
		if(string[i] != ' ' && 
		   string[i] != '\r' &&
 		   string[i] != '\n' &&
		   string[i] != '\t')
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
	int i,k;
	char* ptr;

	if(strchr(name,' ') || strchr(name,'\t'))
	{
		ptr = malloc(strlen(name));

		memset(ptr,0,strlen(name));

		for(i = 0,k = 0;i < strlen(name);i++)
			if(name[i] != ' ' && name[i] != '\t')
				ptr[k++] = name[i];
	} else
		ptr = strdup(name);

#ifdef _DEBUG
	printf("[DEBUG] Searched symbols: %s.\n",ptr);
#endif

	for(i = 0;i < llmne.syms_len;i++)
		if(!strcmp(llmne.symbols[i].name,ptr))
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

	int i;
	int offset = 0;
	char *ptr = NULL;
	
	int size = sizeof(instruction_set) / sizeof(struct lxs_mne);

	if( !strcmp( ctx->instr, "DISPLAY" ) )
	{
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

		return newInstr(ctx, 34, offset);
	}

	for(i = 0;i < size;i++)
	{
		if( !strcmp( ctx->instr, instruction_set[i].mne ) )
		{
			if((ptr = strchr(ctx->args[0],'+')))
			{
				offset = atoi( ptr + 1);
				ptr[0] = '\0';
			}

			if(searchSymbols(ctx->args[0]))
				return newInstr(ctx, instruction_set[i].instr, (searchSymbols(ctx->args[0]))->offset + offset);
			else
				return newInstr(ctx, instruction_set[i].instr , atoi(ctx->args[0]));
		}
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
