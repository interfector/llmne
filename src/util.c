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

	if(ctx->argc < 2)
		if(strcmp(ctx->instr,"EXIT") && 
	        strcmp(ctx->instr,"NOP") && 
		   strcmp(ctx->instr,"SET") && 
		   strcmp(ctx->instr,"RET") && 
		   ctx->instr[strlen(ctx->instr)-1] != ':') 
			die("* Syntax error, too few operands after instruction at line: %d.\n",nline);
}

struct llmne_instr*
getInstrFromSymbol(char* name,char* name2,int *size)
{
	int i = 0,pos = 0;
	char* line = xmalloc(256);
	TokenCtx ctx;
	struct llmne_instr * instr = xmalloc(sizeof(struct llmne_instr));

	fgetpos(i_stream,(fpos_t*)&pos);

	fseek(i_stream,0,SEEK_SET);

	while(fgets(line,256,i_stream))
	{
		if(!line[0])
			continue;

		line[strlen(line)-1] = '\0';

		if(line[strlen(line)-1] == ':')
		{
			line[strlen(line)-1] = '\0';

			if(!strcmp(line,name))
			{
				if(!line[0])
					continue;

				printf("%s:%d\n",name,i);

				while(fgets(line,256,i_stream))
				{
					line[strlen(line)-1] = '\0';

					if(line[strlen(line)-1] == ':')
					{
						line[strlen(line)-1] = '\0';

						if(!strcmp(line,name2))
						{
							fseek(i_stream,pos,SEEK_CUR);

							return instr;
						}
					} else {
						instr = (struct llmne_instr*)realloc(instr,++i * sizeof(struct llmne_instr));
						TokenParse(&ctx,line);
						instr[i-1] = InstrParse(&ctx);

						printf("%d:%s\n",i-1,instr[i-1].instr);
					}
				}
			}
		}
	}

	fseek(i_stream,pos,SEEK_CUR);

	return NULL;
}

struct llmne_sym*
resolveSymbols(int* len)
{
	int i = 0,offs = 0;
	char* line = xmalloc(256);
	struct llmne_sym * syms = xmalloc(sizeof(struct llmne_sym));
	struct llmne_instr * instr;

	while(fgets(line,256,i_stream))
	{
		offs++;

		if(!line || line[0] == '\0' || line[0] == '#')
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
				/*else {
					instr = getInstrFromSymbol(syms[i-1].name,syms[i].name,&offs);
					syms[i].offset = syms[i-1].offset + offs;

					i++;

					free(instr);
				}*/
			}
		}
	}

	*len = i;

	fseek(i_stream,0,SEEK_SET);

	free(line);

	return syms;
}

struct llmne_var*
resolveVarSymbols(int* len)
{
	int i = 0,offs = 0;
	char* line = xmalloc(256);
	TokenCtx ctx;
	struct llmne_var * vars = xmalloc(sizeof(struct llmne_var));
	
	while(fgets(line,256,i_stream))
	{
		offs++;

		line[strlen(line)-1] = '\0';

		if(!line || line[0] == '#' || line[0] == '\0')
			continue;

		TokenParse(&ctx,line);
		
		if(!strcmp(ctx.instr,"SET"))
		{
			vars = (struct llmne_var*)realloc(vars,++i * sizeof(struct llmne_var));

			vars[i-1].name = strdup(ctx.args[0]);
			vars[i-1].offset = -1;
			vars[i-1].value = atoi(ctx.args[1]);
		}	
	}

	*len = i;

	fseek(i_stream,0,SEEK_SET);

	free(line);

	return vars;
}

struct llmne_var*
CreateTempVar(int val,char* name)
{
	int i = 0,pos;
	char* line = xmalloc(256);
	TokenCtx ctx;
	struct llmne_var * var = xmalloc(sizeof(struct llmne_var));

	fgetpos(i_stream,(fpos_t*)&pos);

	fseek(i_stream,0,SEEK_SET);
	
	while(fgets(line,256,i_stream))
	{
		line[strlen(line)-1] = '\0';
		if(!line || line[0] == '#' || line[0] == '\0')
			continue;
		TokenParse(&ctx,line);
		i++;
	}

	var->offset = i;
	var->name = strdup(name);
	var->value = val;

	llmne.vars = realloc(llmne.vars,++llmne.vars_len * sizeof(struct llmne_var));

	llmne.vars[llmne.vars_len-1] = *var;

	fseek(i_stream,pos,SEEK_CUR);

	free(line);

	return var;
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

struct llmne_var*
searchVar(char* name)
{
	int i;
	
	if(!strcmp(name,"AX"))
		return &llmne.ax;

	for(i = 0;i < llmne.vars_len;i++)
		if(!strcmp(llmne.vars[i].name,name))
			return &llmne.vars[i];

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

void
AddInstrTo(TokenCtx * ctx, int op, int code)
{
	llmne.instr = realloc(llmne.instr,++llmne.instr_len * sizeof(struct llmne_instr));
	llmne.instr[llmne.instr_len-1] = newInstr(ctx,op,code);
}

struct llmne_instr
InstrParse(TokenCtx* ctx)
{ /* TODO adds STORE,PRINT,SET and fix 
	the multiplce action instruction and 
	the address calculation */

	int offset = 0;
	char* ptr = NULL;
	struct llmne_var * var;
//	struct llmne_var * tmp = CreateTempVar(0);

	if(!strcmp(ctx->instr,"READ")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,10,var->value);
			else if (!strcmp(ptr,"AX"))
			{
				printf("AX found.\n");

				struct llmne_var * read_var;
				if(searchVar("READVAR"))
					read_var = searchVar("READVAR");
				else
					read_var = CreateTempVar(0,"READVAR");

				AddInstrTo(ctx,10,read_var->offset);
				return newInstr(ctx,13,read_var->offset);
			}
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,10,var->offset);

		return newInstr(ctx,10,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"WRITE")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,11,var->value);
			else if(!strcmp(ptr,"AX"))
			{
//				AddInstrTo(ctx,12,tmp->offset);
//				return newInstr(ctx,11,tmp->offset);
			}
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,11,var->offset);

		return newInstr(ctx,11,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"POP")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,12,var->value);
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,12,var->offset);

		return newInstr(ctx,12,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"PUSH")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,13,var->value);
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,13,var->offset);

		return newInstr(ctx,13,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"ADD")) {
		if((var = searchVar(ctx->args[0])))
		{
			if(ctx->args[1][0] == '(' && ctx->args[1][strlen(ctx->args[1])-1] == ')')
			{
				ptr = xmalloc(strlen(ctx->args[1]) - 1);
				strcpy(ptr,ctx->args[1] + 1);
				ptr[strlen(ptr)-1] = '\0';
	
				if(searchVar(ptr))
				{
					AddInstrTo(ctx,13,var->offset);
					AddInstrTo(ctx,14,(searchVar(ptr))->offset);

					return newInstr(ctx,12,var->offset);
				} else if(!strcmp(ptr,"AX"))
				{
					AddInstrTo(ctx,14,var->offset);
					return newInstr(ctx,12,var->offset);
				}
			} else { 
				struct llmne_var * pushed;

				if(searchVar("ADDVAR"))
					pushed = searchVar("ADDVAR");
				else
					pushed = CreateTempVar(atoi(ctx->args[1]),"ADDVAR");

				AddInstrTo(ctx,13,var->offset);
				AddInstrTo(ctx,14,pushed->offset);

				return newInstr(ctx,12,var->offset);
			}
		} else if(!strcmp(ctx->args[0],"AX"))
		{
			if(ctx->args[1][0] == '(' && ctx->args[1][strlen(ctx->args[1])-1] == ')')
			{
				ptr = xmalloc(strlen(ctx->args[1]) - 1);
				strcpy(ptr,ctx->args[1] + 1);
				ptr[strlen(ptr)-1] = '\0';

				if((var = searchVar(ptr)))
					return newInstr(ctx,14,var->offset);
			} else {
				struct llmne_var * addtoax;
				
				if(searchVar("ADDTOAX"))
					addtoax = searchVar("ADDTOAX");
				else
					addtoax = CreateTempVar(atoi(ctx->args[1]),"ADDTOAX");

				return newInstr(ctx,14,addtoax->offset);
			}
		}

		return newInstr(ctx,10,atoi(ctx->args[0])); 
	} else if (!strcmp(ctx->instr,"SUB")) {
		return newInstr(ctx,15,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MUL")) {
		return newInstr(ctx,16,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DIV")) {
		return newInstr(ctx,17,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"MOD")) {
		return newInstr(ctx,18,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"AND")) {
		return newInstr(ctx,19,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"OR")) {
		return newInstr(ctx,20,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"XOR")) {
		return newInstr(ctx,21,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"NOT")) {
		return newInstr(ctx,22,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHL")) {
		return newInstr(ctx,23,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"SHR")) {
		return newInstr(ctx,24,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEL")) {
		return newInstr(ctx,25,atoi(ctx->args[0]));
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

		return newInstr(ctx,34,offset);
	} else if (!strcmp(ctx->instr,"INC")) {
		return newInstr(ctx,35,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"DEC")) {
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
		return newInstr(ctx,38,0);
	} else if (!strcmp(ctx->instr,"STPUSH")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,39,var->value);
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,39,var->offset);

		return newInstr(ctx,39,atoi(ctx->args[0]));
	} else if (!strcmp(ctx->instr,"STPOP")) {
		if(ctx->args[0][0] == '(' && ctx->args[0][strlen(ctx->args[0])-1] == ')')
		{
			ptr = xmalloc(strlen(ctx->args[0]) - 1);

			strcpy(ptr,ctx->args[0] + 1);
			ptr[strlen(ptr)-1] = '\0';

			if((var = searchVar(ptr)))
				return newInstr(ctx,40,var->value);
		}

		if((var = searchVar(ctx->args[0])))
			return newInstr(ctx,40,var->offset);

		return newInstr(ctx,40,atoi(ctx->args[0]));
	}

	return (struct llmne_instr) { 0,{ 0 },0,0,0 };
}

void
printInstr()
{
	struct llmne_sym * sym;
	
	if(llmne.instr[llmne.instr_len-1].instr)
	{/*
		if(llmne.instr[llmne.instr_len-1].instr_code == 27)
		{
			if((sym = searchSymbols(llmne.instr[llmne.instr_len-1].ctx.args[0])))
			{
				if(sym->offset != llmne.instr[llmne.instr_len-1].code)
				{
					printf("%d  %s %s+%d\n",llmne.instr[llmne.instr_len-1].opcode,
									    llmne.instr[llmne.instr_len-1].instr,
									    llmne.instr[llmne.instr_len-1].ctx.args[0],
									    llmne.instr[llmne.instr_len-1].code - sym->offset);
				} else {
					printf("%d  %s %s\n",llmne.instr[llmne.instr_len-1].opcode,
									 llmne.instr[llmne.instr_len-1].instr,
									 llmne.instr[llmne.instr_len-1].ctx.args[0]);
				}
			} else {
				printf("%d  %s %s\n",llmne.instr[llmne.instr_len-1].opcode,
								 llmne.instr[llmne.instr_len-1].instr,
								 llmne.instr[llmne.instr_len-1].ctx.args[0]);
			}
		} else {
		printf("%d  %s %s,%s\n",llmne.instr[llmne.instr_len-1].opcode,
						    llmne.instr[llmne.instr_len-1].instr,
						    llmne.instr[llmne.instr_len-1].ctx.args[0],
						    llmne.instr[llmne.instr_len-1].ctx.args[1]);
		}*/

		printf("%d  %s\n",llmne.instr[llmne.instr_len-1].opcode,
					   llmne.instr[llmne.instr_len-1].ctx.line);
	}
}

void
dump_vars()
{
	int i;

	printf("\nVariables:\n");

	for(i = 0;i < llmne.vars_len;i++)
		printf("name: %s\tvalue: %d\n",llmne.vars[i].name,llmne.vars[i].value);
}

void
dump_symbols()
{
	int i;

	printf("\nSymbols:\n");

	for(i = 0;i < llmne.syms_len;i++)
		printf("name: %s\toffset: %d\n",llmne.symbols[i].name,llmne.symbols[i].offset);
}

void
llmne_parse_all(char* line)
{
	TokenCtx tokens;
	
	TokenParse(&tokens,line);

	if(tokens.instr[strlen(tokens.instr)-1] != ':' && strcmp(tokens.instr,"SET")) {
		llmne.instr = realloc(llmne.instr,++llmne.instr_len * sizeof(struct llmne_instr));
		llmne.instr[llmne.instr_len-1] = InstrParse(&tokens);

		printInstr();
	}

	free(tokens.args);
	free(tokens.instr);
}
